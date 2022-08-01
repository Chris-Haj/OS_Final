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
public:

    FileDescriptor(string FileName, FsFile *fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
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

    FsFile *getFile() {
        return fs_file;
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

    map<string, FileDescriptor *> MainDir;
    // MainDir - "file" (FsFile) map, store all the files in the disk.
    // map that links the file name to its FsFile

    map<int, FileDescriptor *> OpenFileDescriptors;
    // OpenFileDescriptors --
    // when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.

    int FindEmptyIndex(map<int, FileDescriptor *> ma) { //This function searches for the lowest key possible that is available to be used as a fd
        if (ma.size() == 0)
            return 0;
        int first = 0;
        for (auto it = ma.begin(); it != ma.end(); it++) {
            if (it->first == first)
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

    ~fsDisk() {
        for (auto i = MainDir.begin(); i != MainDir.end(); i++) {
            delete i->second->getFile();
            delete i->second;
        }
        delete[] BitVector;
        fclose(sim_disk_fd);

    }

    // ------------------------------------------------------------------------
    void listAll() {//fun1
        if (!is_formated) {
            cout << "Disk is not formatted yet!" << endl;
            return;
        }
        int i = 0;
        for (auto curFile = MainDir.begin(); curFile != MainDir.end(); curFile++) {
            cout << i << ": " << curFile->first << endl;
            cout << "index: " << i << ": FileName: " << curFile->first
                 << " , isInUse: " << curFile->second->getInUse() <<" File size: " <<curFile->second->getFile()->getFileSize() << endl;
            i++;
        }
        char bufy;
        cout << "Disk content: '" << endl;
        for (i = 0; i < DISK_SIZE; i++) {
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread(&bufy, 1, 1, sim_disk_fd);
            if (bufy<32)
                cout <<'\0';
            else
                cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }

    void fsFormat(int blockSize = 4) {
        if (is_formated) {//If Disk was formatted, and we need to format again, we need to clear out, all data structures that were used earlier
            for (auto i = MainDir.begin(); i != MainDir.end(); i++) {
                delete i->second->getFile();
                delete i->second;
            }
            MainDir.clear();
            OpenFileDescriptors.clear();
            delete[] BitVector;
            for (int i = 0; i < DISK_SIZE; i++) { //Clear out the disk
                int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
                ret_val = fwrite("\0", 1, 1, sim_disk_fd);
                assert(ret_val == 1);
            }
        }
        BitVectorSize = DISK_SIZE / blockSize;
        EmptyBlocks = BitVectorSize;
        cout << "FORMAT DISK: number of blocks: " << BitVectorSize << endl;
        BitVector = new int[BitVectorSize];
        is_formated = true;
        for (int i = 0; i < BitVectorSize; i++)
            BitVector[i] = 0;
    }

    /*Function to create a new file (fsFile) and update OpenFileDescriptors and MainDir and also keeps file Open
     * returns the file_descriptor
     * */
    int CreateFile(string fileName) {
        if (!is_formated) {
            cout << "Disk needs to be formatted!" << endl;
            return -1;
        }
        if (MainDir.find(fileName) != MainDir.end()) {
            cout << "File already exists!" << endl;
            return -1;
        }
        int BlockSize = DISK_SIZE / BitVectorSize;
        FsFile *file = new FsFile{BlockSize};
        FileDescriptor *fd = new FileDescriptor{fileName, file};
        int FileDescIndex = FindEmptyIndex(OpenFileDescriptors); //Search for lowest fd in OpenFileDescriptors and give it to file
        OpenFileDescriptors.insert({FileDescIndex, fd}); //After creating the file, it is inserted into both MainDir, and OpenFileDescriptors (file is left open after creation)
        MainDir.insert({move(fileName), fd});
        return FileDescIndex;
    }

    int OpenFile(string fileName) {
        if (MainDir.find(fileName) == MainDir.end()) {
            cout << "File does not exist!" << endl;
            return -1;
        }
        if (MainDir[fileName]->getInUse()) {
            cout << "File is already open!" << endl;
            return -1;
        }
        int FileDescIndex = FindEmptyIndex(OpenFileDescriptors); //Find the lowest fd in OpenFileDescriptors and give it to file
        OpenFileDescriptors.insert({FileDescIndex, MainDir[fileName]});
        MainDir[fileName]->setInUse(true);
        return FileDescIndex;
    }

    string CloseFile(int fd) { //After receiving the fd of a file, it removes it from the OpenFileDescriptors, and its setInUse is set to false, indicating that is closed and not being used.
        if (OpenFileDescriptors.find(fd) == OpenFileDescriptors.end()) {
            cout << "FileDescriptor does not exist!" << endl;
            return "-1";
        }
        OpenFileDescriptors[fd]->setInUse(false);
        string FileName = OpenFileDescriptors[fd]->getFileName();
        OpenFileDescriptors.erase(fd);
        return FileName;
    }

    int WriteToFile(int fd, char *buf, int len) {
        if (!is_formated) {
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
        if (FileSize == (blockSize * blockSize)) {
            cout << "File is full!" << endl;
            return -1;
        }
        int SpaceLeft = (blockSize * blockSize) - file->getFileSize(), return_value = len;
        if (len > SpaceLeft) { //If the length of the buffer is larger than the space left in the file then it, we will only write according to how much space is left
            len = SpaceLeft;
            buf[len] = '\0';
            return_value = -1; //return_val is set to -1 to indicate that an error occurred.
        }
        for(int i=len;i<blockSize*blockSize;i++)
            buf[i]='\0';
        int BlocksNeeded = ceil((double) len / blockSize);
        char *positions = new char[blockSize]; // Array to store positions of the file's blocks
        for (int i=0;i< blockSize;i++)
            positions[i]='\0';
        int LettersWritten = 0; //Used to point where to continue writing from buffer
        if (FileSize == 0) {
            if (BlocksNeeded>EmptyBlocks){
                cout << "Not enough space"<<endl;
                return -1;
            }
            for (int i = 0; i < BitVectorSize; i++) { // Find location for index block and take it.
                if (BitVector[i] == 0) {
                    IndexBlock = i;
                    file->setIndexBlock(IndexBlock);
                    BitVector[i] = 1;
                    break;
                }
            }
            for(int i=0,j=0;i<BitVectorSize&&j<BlocksNeeded;i++){ //Get positions of blocks to write on and take them, while adding them to "positions"
                if(BitVector[i]==0){
                    BitVector[i]=1;
                    positions[j++] = (unsigned char) i;
                    fseek(sim_disk_fd,i*blockSize,SEEK_SET);
                    fwrite(&buf[LettersWritten],1,blockSize,sim_disk_fd);
                    LettersWritten+=blockSize;
                }
            }
            fseek(sim_disk_fd,IndexBlock*blockSize,SEEK_SET);
            fwrite(positions,1,blockSize,sim_disk_fd);//Write into index block the positions of the blocks to write on
        }
        else{
            char *oldPositions = new char[blockSize+1];
            oldPositions[blockSize]='\0';
            fseek(sim_disk_fd,IndexBlock*blockSize,SEEK_SET);
            fread(oldPositions,1,blockSize,sim_disk_fd);//Read previous positions of blocks to write on to update them later
            int offset = FileSize%blockSize;
            if(offset==0){ //if offset is 0, then we need to get a new block to write on
                for(int i=0,j=0;i<BitVectorSize&&j<BlocksNeeded;i++) { //Get positions of blocks to write on and take them, while adding them to "positions"
                    if (BitVector[i] == 0) {
                        BitVector[i] = 1;
                        positions[j++] = (unsigned char) i;
                        fseek(sim_disk_fd, i * blockSize, SEEK_SET);
                        fwrite(&buf[LettersWritten], 1, blockSize, sim_disk_fd);
                        LettersWritten += blockSize;
                    }
                }
                strcat(oldPositions,positions);
                fseek(sim_disk_fd,IndexBlock*blockSize,SEEK_SET);
                fwrite(oldPositions,1,blockSize,sim_disk_fd);
            }
            else{//if offset is not 0, continue writing on the same block then get a new block to write on if needed
                int lastBlock = (int) oldPositions[strlen(oldPositions)-1];
                fseek(sim_disk_fd,(lastBlock*blockSize)+offset,SEEK_SET);
                fwrite(buf,1,blockSize-offset,sim_disk_fd);
                LettersWritten+=blockSize-offset;
                BlocksNeeded = ceil((double)(len-LettersWritten)/blockSize);
                for(int i=0,j=0;i<BitVectorSize&&j<BlocksNeeded;i++) { //Get positions of blocks to write on and take them, while adding them to "positions"
                    if (BitVector[i] == 0) {
                        BitVector[i] = 1;
                        positions[j++] = (unsigned char) i;
                        fseek(sim_disk_fd, i * blockSize, SEEK_SET);
                        fwrite(&buf[LettersWritten], 1, blockSize, sim_disk_fd);
                        LettersWritten += blockSize;
                    }
                }
                strcat(oldPositions,positions);//Update positions of blocks to write on
                fseek(sim_disk_fd,IndexBlock*blockSize,SEEK_SET);
                fwrite(oldPositions,1,blockSize,sim_disk_fd);
            }
            delete[] oldPositions;
        }
        delete[] positions;
        file->setBlockInUse(strlen(positions));
        file->setFileSize(FileSize+len);
        return return_value;
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
        int blockSize = file->getBlockSize();
        int Index = file->getIndexBlock();
        char *ToRead = new char[blockSize];
        //ToRead will store the indexes that the file uses
        fseek(sim_disk_fd, Index*blockSize, SEEK_SET);
        fread((void *) ToRead, 1, blockSize, sim_disk_fd);
        fseek(sim_disk_fd, Index*blockSize, SEEK_SET);
        for (int i = 0; i < file->getBlockSize(); i++) { //Clear out the Indexes block
            fwrite("\0", 1, 1, sim_disk_fd);
        }
        BitVector[Index] = 0;
        for (int i = 0; i < strlen(ToRead); i++) {//Go over each block used by the file, and clear it, while setting its index in BitVector to 0, indicating that it is free.
            int index = (int) (ToRead[i]);
            fseek(sim_disk_fd, index * blockSize, SEEK_SET);
            for (int j = 0; j < blockSize; j++) {
                fwrite("\0", 1, 1, sim_disk_fd);
            }
            BitVector[index] = 0;
        }
        delete MainDir[FileName]->getFile(); //After deleting all contents of the file from the disk, we delete the file pointer, then delete the fileDescriptor pointer
        delete MainDir[FileName];
        MainDir.erase(FileName);//Remove the file's name from Main Directory
        delete[] ToRead;
        EmptyBlocks+=strlen(ToRead);
        return 1;
    }

    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len) {//fund8
        if (!is_formated) { //If disk is not formatted
            cout << "Disk is not formatted!" << endl;
            return -1;
        }
        strcpy(buf, ""); //Clear buffer
        if (OpenFileDescriptors.find(fd) == OpenFileDescriptors.end()) { // If File is not open
            cout << "File is not open!" << endl;
            return -1;
        }
        FsFile *file = OpenFileDescriptors[fd]->getFile();
        int blockSize = file->getBlockSize();
        char *ToRead = new char[blockSize];
        int Index = file->getIndexBlock();
        fseek(sim_disk_fd, Index*blockSize, SEEK_SET);
        fread((void *) ToRead, 1, blockSize, sim_disk_fd); // ToRead will store the indexes of each block of file
        int BlocksToRead = strlen(ToRead);
        int i = 0;
        char *ReadFromBlock = new char[blockSize];
        for (; i < BlocksToRead; i++) { //Go over each index in ToRead which is an index of a block of the current file, go to that block, read it all and add the result to buf
            int index = (int) (ToRead[i]);
            fseek(sim_disk_fd, index * blockSize, SEEK_SET);
            fread((void *) ReadFromBlock, blockSize, 1, sim_disk_fd);
            strncpy(&buf[i * blockSize], ReadFromBlock, blockSize);
            strcpy(ReadFromBlock, "");
        }
        buf[len] = '\0';
        delete[] ReadFromBlock;
        delete[] ToRead;
        return len>file->getFileSize() ? -1:len; //In-case we tried reading more than the file size, we return -1 to indicate that an error happened, and the requested length was not read.
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