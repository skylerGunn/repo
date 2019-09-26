#ifndef _S19FS_H__
#define _S19FS_H__

#include <sys/types.h>
#include <dyn_array.h>
#include <block_store.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>		// for size_t
#include <inttypes.h>	// for uint16_t
#include <string.h>
#include <bitmap.h>

// seek_t is for fs_seek
typedef enum { FS_SEEK_SET, FS_SEEK_CUR, FS_SEEK_END } seek_t;

typedef enum { FS_REGULAR, FS_DIRECTORY } file_t;

#define FS_FNAME_MAX (32)
// INCLUDING null terminator

typedef struct {
    // You can add more if you want
    // vvv just don't remove or rename these vvv
    uint8_t inodeID;
    char name[FS_FNAME_MAX];
    file_t type;
} file_record_t;

/*typedef struct {
//needs a block_store instance for FBM and for generally managing the files
//needs a dyn_array thing but for now let's just use block_store
//also needs a path I guess?
//char *path;
block_store_t *bStore;
inode_t *nodeTable; //256 of these
//anything else?
} S19FS_t; */

typedef struct {
    //for the inode
    uint16_t directPointers[6]; //6 direct pointers, each 2 bytes
    uint16_t indirectPointer; //1 2 byte that points to block of direct pointers
    uint16_t doubleIndirect; //1 2byte that points to block of indirect pointers
    uint8_t inodeID; //id of the inode for inode table
    uint8_t subID;
    uint8_t isFile;
    uint16_t endBlock; //address of the ending block, acts like a bitmap
    uint16_t bytesLeft; //how many bytes are left in the current last block. Combinined with endBlock and starting block can get total number of bytes for file
    uint8_t isOpen; //1 if file is open/in use, 0 if file is not open/in use, 2 if second fd is in use
    uint16_t fd1Block; //which block the file descriptor is on
    uint16_t fd1Loc; //byte pos of the current block
    uint16_t fd2Block;
    uint16_t fd2Loc;

    uint16_t fd3Block; //has 3 internal file descriptors
    uint16_t fd3Loc; //first inode is root directory
    //currently at 33 bytes, with supposed to be 64 blocks. use rest as placeholder atm, used for directories and stuff
    //uint8_t isFile; //0 if not made yet, 1 for file, 2 for directory
    //uint16_t numBlocks; //total blocks, can be up to
    //uint8_t subID; //the inode id of what directory its in: 0 by default (not made yet or root directory); for example, if was inside subdirectory with id 3, would be 3, and the subdirectory would have subID of root or 0
    //so it only shows up when user is inside that directory
    uint8_t placeholder[28]; //used for fileName, 0 if empty
} inode_t;

/*typedef struct {
  uint8_t bytes[1024];
  } blocks_t; //bytes in each block
  */
typedef struct S19FS {
    //block_store_t *bStore;
    //bitmap_t *fbm; //8 blocks
    inode_t nodeTable[256]; //256 inodes, 16 blocks
    //uint8_t blocks[65512][1024]; //65536 - 8 - 16 = 65512 blocks for other data
    block_store_t *blocks;
    //bitmap_t *bMap;
} S19FS_t;


///
/// Formats (and mounts) an S19FS file for use
/// \param fname The file to format
/// \return Mounted S19FS object, NULL on error
///
S19FS_t *fs_format(const char *path);

///
/// Mounts an S19FS object and prepares it for use
/// \param fname The file to mount

/// \return Mounted S19FS object, NULL on error

///
S19FS_t *fs_mount(const char *path);

///
/// Unmounts the given object and frees all related resources
/// \param fs The S19FS object to unmount
/// \return 0 on success, < 0 on failure
///
int fs_unmount(S19FS_t *fs);

///
/// Creates a new file at the specified location
///   Directories along the path that do not exist are not created
/// \param fs The S19FS containing the file
/// \param path Absolute path to file to create
/// \param type Type of file to create (regular/directory)
/// \return 0 on success, < 0 on failure
///
int fs_create(S19FS_t *fs, const char *path, file_t type);

///
/// Opens the specified file for use
///   R/W position is set to the beginning of the file (BOF)
///   Directories cannot be opened
/// \param fs The S19FS containing the file
/// \param path path to the requested file
/// \return file descriptor to the requested file, < 0 on error
///
int fs_open(S19FS_t *fs, const char *path);

///
/// Closes the given file descriptor
/// \param fs The S19FS containing the file
/// \param fd The file to close
/// \return 0 on success, < 0 on failure
///
int fs_close(S19FS_t *fs, int fd);

///
/// Moves the R/W position of the given descriptor to the given location
///   Files cannot be seeked past EOF or before BOF (beginning of file)
///   Seeking past EOF will seek to EOF, seeking before BOF will seek to BOF
/// \param fs The S19FS containing the file
/// \param fd The descriptor to seek
/// \param offset Desired offset relative to whence
/// \param whence Position from which offset is applied
/// \return offset from BOF, < 0 on error
///
off_t fs_seek(S19FS_t *fs, int fd, off_t offset, seek_t whence);

///
/// Reads data from the file linked to the given descriptor
///   Reading past EOF returns data up to EOF
///   R/W position in incremented by the number of bytes read
/// \param fs The S19FS containing the file
/// \param fd The file to read from
/// \param dst The buffer to write to
/// \param nbyte The number of bytes to read
/// \return number of bytes read (< nbyte IFF read passes EOF), < 0 on error
///
ssize_t fs_read(S19FS_t *fs, int fd, void *dst, size_t nbyte);

///
/// Writes data from given buffer to the file linked to the descriptor
///   Writing past EOF extends the file
///   Writing inside a file overwrites existing data
///   R/W position in incremented by the number of bytes written
/// \param fs The S19FS containing the file
/// \param fd The file to write to
/// \param dst The buffer to read from
/// \param nbyte The number of bytes to write
/// \return number of bytes written (< nbyte IFF out of space), < 0 on error
///
ssize_t fs_write(S19FS_t *fs, int fd, const void *src, size_t nbyte);

///
/// Deletes the specified file and closes all open descriptors to the file
///   Directories can only be removed when empty
/// \param fs The S19FS containing the file
/// \param path Absolute path to file to remove
/// \return 0 on success, < 0 on error
///
int fs_remove(S19FS_t *fs, const char *path);

///
/// Populates a dyn_array with information about the files in a directory
///   Array contains up to 15 file_record_t structures
/// \param fs The S19FS containing the file
/// \param path Absolute path to the directory to inspect
/// \return dyn_array of file records, NULL on error
///
dyn_array_t *fs_get_dir(S19FS_t *fs, const char *path);

/// Moves the file from one location to the other
///   Moving files does not affect open descriptors
/// \param fs The S19FS containing the file
/// \param src Absolute path of the file to move
/// \param dst Absolute path to move the file to
/// \return 0 on success, < 0 on error
///
int fs_move(S19FS_t *fs, const char *src, const char *dst);

/// Link the dst with the src
/// dst and src should be in the same File type, say, both are files or both are directories
/// \param fs The F18FS containing the file
/// \param src Absolute path of the source file
/// \param dst Absolute path to link the source to
/// \return 0 on success, < 0 on error
///
int fs_link(S19FS_t *fs, const char *src, const char *dst);

#endif
