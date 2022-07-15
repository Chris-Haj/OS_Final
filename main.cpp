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
        //@TODO MUST ADD MORE FUNCTIONS
    }

    int getfile_size() {
        return file_size;
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

    // MainDir - "file" (FsFile) vector, store all the files in the disk.
    // map that links the file name to its FsFile
    map<string, FileDescriptor*> MainDir;

    vector<FileDescriptor*> OpenFileDescriptors;
    // (6) OpenFileDescriptors --
    //  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
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

    // ------------------------------------------------------------------------
    void listAll() {
        int i = 0;
        for (auto curFile = MainDir.begin(); curFile != MainDir.end(); curFile++) {
            cout << i << ": " << curFile->first << endl;
            cout << "index: " << i << ": FileName: " << curFile->first
                 << " , isInUse: " << curFile->second->getInUse() << endl;
            i++;
        }


        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++) {
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread(&bufy, 1, 1, sim_disk_fd);
            cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }


    // ------------------------------------------------------------------------
    void fsFormat(int blockSize = 4) {
        BitVectorSize = DISK_SIZE / blockSize;
        BitVector = new int[BitVectorSize];
        for (int i = 0; i < BitVectorSize; i++) {
            BitVector[i] = 0;
        }
        is_formated = true;
    }

    /*Function to create a new file (fsFile) and update OpenFileDescriptors and MainDir and also keeps file Open
     * returns the file_descriptor
     * */
    int CreateFile(string fileName) { //@TODO IF DISK IS NOT CREATED, RETURN -1
        if (!is_formated)
            return -1;
        int BlockSize = DISK_SIZE / BitVectorSize;
        FsFile *file = new FsFile{BlockSize};
        FileDescriptor *fd= new FileDescriptor {fileName,file};
        OpenFileDescriptors.push_back(fd);
        MainDir.insert({move(fileName), fd});
        return OpenFileDescriptors.size()-1;
    }

    /*If file is not open then open and return fileDescriptor
     *else */
    int OpenFile(string fileName) {


    }

    // ------------------------------------------------------------------------
    string CloseFile(int fd) {

    }

    /*
     * 1) File must be open
     2) Disk must have space
     3) File must have space
     4) Disk must be initialized
     */
    int WriteToFile(int fd, char *buf, int len) {

        write(fd, buf, len);

    }

    // ------------------------------------------------------------------------
    int DelFile(string FileName) {

    }

    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len) {


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