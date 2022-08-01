Disk Simulator
Authored by Christopher Haj
207824772

==Description==
This program simulates how a computer's hard disk works using the index allocation method with a BitVector.
It can do several operations on files, including creating, deleting, writing, reading, opening and closing them.
In the beginning of the program, the user must format the disk and choosing how many blocks each index has.



Program DATABASES:
DISK_SIM_MEM.txt is used as the hard disk main directory, which holds all information on files.

==Functions==
1)listAll() function which prints out information on all files in the disk. And contents of each file.
2)fsFormat() function which formats the disk, and sets the number of blocks each index has.
If disk was already formatted, and function is called again, it will erase all data on the disk.
3)CreateFile() function which creates a file on the disk, adds it to the main directory, and returns the file's FD.(File is left open)
4)OpenFile() function which opens a file on the disk, and returns the file's FD.
5)CloseFile() function which closes a file on the disk, and removes it from OpenFileDescriptors and sets inUse to 0.
6)WriteFile() function which writes to a file on the disk, and returns the number of bytes written.
7)ReadFile() function which reads from a file on the disk, and returns the number of bytes read.
8)DeleteFile() function which deletes a file on the disk, and all its contents, and removes it from the main directory.

==Program Files==
OS_Final

==How to compile?==
compile: gcc DiskSim.c -o DiskSim

==How to run?==
./DiskSim


==Input==
1) 0 to exit program
2) 1 to list all files on the disk
3) 2 to format the disk, user needs to pass in an integer which represents the size of a block in the disk.
4) 3 to create a file, user needs to pass in a string for the name of the new file.
5) 4 to open a file, user needs to pass in string, to open the file with the matching name.
6) 5 to close a file, user needs to pass in an int which is the fd of the open file.
7) 6 to write to a file, user needs to first pass an int, which is the fd of the file needed to write on,
then a string of what should be written.
8) 7 to read from a file, user needs to first pass in an int which is what fd is read from, and another int,
which represents how many bytes to read.
9) 8 to delete a file, user needs to pass in the name of the file that he wants to delete.


