#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 256

// ============================================================================
void decToBinary(int n, char &c) {
    // array to store binary number
    int binaryNum[8];

    // counter for binary array
    int i = 0;
    while (n > 0) {
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }

    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j] == 1)
            c = c | 1u << j;
    }
}



// ============================================================================

class FsFile {
    int file_size;
    int block_in_use;
    int index_block;
    int block_size;
public:
    FsFile(int _block_size) {
        file_size = 0;
        block_in_use = 0;
        block_size = _block_size;
        index_block = -1;
    }

    int getFileSize() const {
        return file_size;
    }

    void setFileSize(int fileSize) {
        file_size = fileSize;
    }

    int getBlockInUse() const {
        return block_in_use;
    }

    void setBlockInUse(int blockInUse) {
        block_in_use = blockInUse;
    }

    int getIndexBlock() const {
        return index_block;
    }

    void setIndexBlock(int indexBlock) {
        index_block = indexBlock;
    }

    int getBlockSize() const {
        return block_size;
    }
};

// ============================================================================

class FileDescriptor {
    string file_name;
    FsFile *fs_file;
    bool inUse;
    int fd;

public:

    FileDescriptor(string FileName, FsFile *fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
        fd=-1;
    }

    string getFileName() {
        return file_name;
    }

    bool getInUse() const {
        return inUse;
    }

    void setInUse(bool use) {
        inUse = use;
    }
    FsFile *getFile(){
        return fs_file;
    }
    int getFd(){
        return fd;
    }

    void setFd(int FileD){
        this->fd=FileD;
    }

};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

// ============================================================================

class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int *BitVector;
    int EmptyBlocks;

    // MainDir - "file" (FsFile) map, store all the files in the disk.
    // map that links the file name to its FsFile
    map<string, FileDescriptor *> MainDir;
    map<int, FileDescriptor *> OpenFileDescriptors;

    // OpenFileDescriptors --
    // when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    int FindEmptyIndex(map<int, FileDescriptor *> ma) {
        if (ma.size() == 0)
            return 0;
        int first = 0;
        for (auto it = ma.begin(); it != ma.end(); it++) {
            if(it->first==first)
                first++;
            else
                return first;
        }
        return first;
    }


public:
    fsDisk() {
        sim_disk_fd = fopen(DISK_SIM_FILE, "r+");
        assert(sim_disk_fd);
        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        is_formated = false;
    }
    ~fsDisk(){
        for(auto i = MainDir.begin(); i!=MainDir.end();i++){
            delete i->second->getFile();
            delete i->second;
        }
        delete[] BitVector;
        fclose(sim_disk_fd);

    }

    // ------------------------------------------------------------------------
    void listAll() {//fun1
        if(!is_formated){
            cout <<"Disk is not formatted yet!"<<endl;
            return;
        }
        int i = 0;
        for (auto curFile = MainDir.begin(); curFile != MainDir.end(); curFile++) {
            cout << i << ": " << curFile->first << endl;
            cout << "index: " << i << ": FileName: " << curFile->first
                 << " , isInUse: " << curFile->second->getInUse() << endl;
            i++;
        }
        char bufy;
        cout << "Disk content: '"<<endl;
        for (i = 0; i < DISK_SIZE; i++) {
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread(&bufy, 1, 1, sim_disk_fd);
            cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }

    void fsFormat(int blockSize = 4) {
        if(is_formated){//If Disk was formatted, and we need to format again, we need to clear out, all data structures that were used earlier
            for(auto i = MainDir.begin(); i!=MainDir.end();i++){
                delete i->second->getFile();
                delete i->second;
            }
            MainDir.clear();
            OpenFileDescriptors.clear();
            delete[] BitVector;
            for (int i = 0; i < DISK_SIZE; i++) {
                int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
                ret_val = fwrite("\0", 1, 1, sim_disk_fd);
                assert(ret_val == 1);
            }
        }
        BitVectorSize = DISK_SIZE / blockSize;
        EmptyBlocks=BitVectorSize;
        cout << "FORMAT DISK: number of blocks: " << BitVectorSize << endl;
        BitVector = new int[BitVectorSize];
        is_formated = true;
        for (int i = 0; i < BitVectorSize; i++)
            BitVector[i] = 0;
    }

    /*Function to create a new file (fsFile) and update OpenFileDescriptors and MainDir and also keeps file Open
     * returns the file_descriptor
     * */
    int CreateFile(string fileName) { //fun3
        if (!is_formated){
            cout << "Disk needs to be formatted!" <<endl;
            return -1;
        }
        if(MainDir.find(fileName)!=MainDir.end()){
            cout << "File already exists!"<<endl;
            return -1;
        }
        int BlockSize = DISK_SIZE / BitVectorSize;
        FsFile *file = new FsFile{BlockSize};
        FileDescriptor *fd = new FileDescriptor{fileName, file};
        int FileDescIndex = FindEmptyIndex(OpenFileDescriptors);
        fd->setFd(FileDescIndex);
        OpenFileDescriptors.insert({FileDescIndex, fd});
        MainDir.insert({move(fileName), fd});
        return FileDescIndex;
    }

    int OpenFile(string fileName) {//fun4
        if (MainDir.find(fileName) == MainDir.end()) {
            cout << "File does not exist!" << endl;
            return -1;
        }
        if (MainDir[fileName]->getInUse()) {
            cout << "File is already open!" << endl;
            return -1;
        }
        FileDescriptor *fileDescriptor = MainDir[fileName];
        if(fileDescriptor->getFd()!=-1){
            OpenFileDescriptors[fileDescriptor->getFd()]=fileDescriptor;
            return fileDescriptor->getFd();
        }
        int FileDescIndex = FindEmptyIndex(OpenFileDescriptors);
        OpenFileDescriptors.insert({FileDescIndex, fileDescriptor});
        fileDescriptor->setInUse(true);
        fileDescriptor->setFd(FileDescIndex);
        return FileDescIndex;
    }


    string CloseFile(int fd) {
        if (OpenFileDescriptors.find(fd) == OpenFileDescriptors.end()) {
            cout << "FileDescriptor does not exist!" << endl;
            return "-1";
        }
        if(OpenFileDescriptors[fd] == nullptr){
            cout << "File is not open!"<<endl;
            return "-1";
        }
        string FileName = OpenFileDescriptors[fd]->getFileName();
        OpenFileDescriptors[fd]->setInUse(false);
        OpenFileDescriptors[fd]= nullptr;
        return FileName;
    }

    int WriteToFile(int fd, char *buf, int len) {//fun6
        if (!is_formated) { //If disk is not formatted
            cout << "Disk is not formatted!" << endl;
            return -1;
        }
        if (OpenFileDescriptors.find(fd) == OpenFileDescriptors.end()) { // If File is not open
            cout << "File is not open!" << endl;
            return -1;
        }
        FsFile *file = OpenFileDescriptors[fd]->getFile();
        int blockSize = file->getBlockSize();
        int FileSize = file->getFileSize();
        int IndexBlock = file->getIndexBlock();
        if(FileSize==(blockSize*blockSize)){
            cout << "File is full!"<<endl;
            return -1;
        }
        int SpaceLeft = (blockSize*blockSize) - file->getFileSize(), return_value=len;
        if(len>SpaceLeft){
            len=SpaceLeft;
            buf[len]='\0';
            return_value=-1;
        }
        int BlocksNeeded = ceil((double)len/blockSize);
        if(FileSize==0){
            for(int i=0;i<BitVectorSize;i++){
                if(BitVector[i]==0){
                    IndexBlock=i;
                    BitVector[i]=1;
                    break;
                }
            }
        }
        char *positions = new char[blockSize];
        fseek(sim_disk_fd,IndexBlock*blockSize,SEEK_SET);
        fread(positions,1,blockSize,sim_disk_fd);





    }

    // ------------------------------------------------------------------------
    int DelFile(string FileName) {//fun7
        if (MainDir.find(FileName) == MainDir.end()) {
            cout << "File does not exist" << endl;
            return -1;
        }
        FsFile *file = MainDir[FileName]->getFile();
        if (MainDir[FileName]->getInUse()) {
            cout << "Cannot delete a file that is open!" << endl;
            return -1;
        }
        char *ToRead = new char[file->getBlockSize()];
        int IndexesBlockPos = file->getIndexBlock() * file->getBlockSize() ;
        fseek(sim_disk_fd,IndexesBlockPos,SEEK_SET);
        fread((void*)ToRead, file->getBlockSize(), 1, sim_disk_fd);
        fseek(sim_disk_fd,IndexesBlockPos,SEEK_SET);
        for(int i=0;i<file->getBlockSize();i++){
            fwrite("\0",1,1,sim_disk_fd);
        }
        BitVector[file->getIndexBlock()]=0;

        for(int i=0;i<strlen(ToRead);i++){
            int index = (int)(ToRead[i]) - '0';
            fseek(sim_disk_fd,index*file->getBlockSize(),SEEK_SET);
            for(int j=0;j<file->getBlockSize();j++){
                fwrite("\0",1,1,sim_disk_fd);
            }
            BitVector[index]=0;
        }
        int fd = MainDir[FileName]->getFd();
        OpenFileDescriptors.erase(fd);
        delete MainDir[FileName]->getFile();
        delete MainDir[FileName];
        MainDir.erase(FileName);
        delete [] ToRead;
        return fd;
    }

    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len) {//fund8
        if (!is_formated) { //If disk is not formatted
            cout << "Disk is not formatted!" << endl;
            return -1;
        }
        strcpy(buf,""); //Clear buffer
        if (OpenFileDescriptors.find(fd) == OpenFileDescriptors.end()) { // If File is not open
            cout << "File is not open!" << endl;
            return -1;
        }
        FsFile *file = OpenFileDescriptors[fd]->getFile();
        char *ToRead = new char[file->getBlockSize()];
        int IndexesBlockPos = file->getIndexBlock() * file->getBlockSize();
        fseek(sim_disk_fd,IndexesBlockPos,SEEK_SET);
        //read from sim_disk_fd to positions 4 chars
        fread((void*)ToRead, 1, file->getBlockSize(), sim_disk_fd);
        int BlocksToRead = ceil((double) len/file->getBlockSize());
        int i=0;
        char *ReadFromblock = new char[file->getBlockSize()];
        for(;i<BlocksToRead;i++){
            int index = (int)(ToRead[i]) - '0';
            fseek(sim_disk_fd,index*file->getBlockSize(),SEEK_SET);
            fread((void*)ReadFromblock, file->getBlockSize(), 1, sim_disk_fd);
            strncpy(&buf[i*file->getBlockSize()],ReadFromblock,file->getBlockSize());
            strcpy(ReadFromblock,"");
        }
        buf[len]='\0';
        delete[] ReadFromblock;
        delete[] ToRead;
    }
};

int main() {

    int blockSize;
    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;

    while (true) {
        cin >> cmd_;
        switch (cmd_) {
            case 0:   // exit
                delete fs;
                exit(0);
                break;

            case 1:  // list-file
                fs->listAll();
                break;

            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile(_fd, str_to_write, strlen(str_to_write));
                break;

            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read;
                fs->ReadFromFile(_fd, str_to_read, size_to_read);
                cout << "ReadFromFile: " << str_to_read << endl;
                break;

            case 8:   // delete file
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }

}