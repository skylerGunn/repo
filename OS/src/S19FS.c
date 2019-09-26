#include "dyn_array.h"
#include "bitmap.h"
#include "block_store.c"
#include "S19FS.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BLOCK_STORE_NUM_BLOCKS 65536   // 2^16 blocks.
#define BLOCK_STORE_AVAIL_BLOCKS 65528 // Last 8 blocks consumed by the FBM
#define BLOCK_SIZE_BITS 8192         // 2^10 BYTES per block *2^3 BITS per BYTES
#define BLOCK_SIZE_BYTES 1024         // 2^10 BYTES per block
#define BLOCK_STORE_NUM_BYTES (BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES)  // 2^16 blocks of 2^10 bytes.

S19FS_t* fs_format(const char* path);
S19FS_t* fs_mount(const char* path);
int fs_unmount(S19FS_t *fs);
void writeEntry(S19FS_t *fs, char *fname, uint8_t, uint8_t*, uint8_t);
int searchDirEntry(char*, uint8_t*);
size_t inodeMake(S19FS_t *fs, file_t type, uint8_t dir, uint8_t *idp);

char lastS19[32];

S19FS_t* fs_format(const char* path) { //I think fs_format happens once and it creates the actual file, and mount updates the file and puts out all the actual files and directories and stuff
    //path = NULL;
    if (path == NULL) {
        printf("Cannot have null file path \n");
        return NULL;
    }
    if (strstr(path, ".S19FS") == NULL) {
        printf("Not a S19FS file \n");
        return NULL;
    }
    //S19FS_t *mainSys = fs_mount(path);
    /*int fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
      if (fd < 0) { //file opening failed somehow
      return 0;
      }
      close(fd);*/
    //bitmap_t *fbm = (bitmap_t*) malloc(sizeof(bitmap_t));
    //*fbm->data = malloc(sizeof(uint8_t) * BLOCK_STORE_AVAIL_BLOCKS); //first 8 blocks consumed by FBM
    S19FS_t *mainObj = (S19FS_t*) malloc(sizeof(S19FS_t));
    mainObj->blocks = block_store_create(path);
    //printf("%zu total set \n", bitmap_total_set(mainObj->blocks->b));
    //mainObj->bMap = bitmap_create(65512);
    //mainObj->fbm = bitmap_create(65512);
    /*if (mainObj->fbm == NULL) {
      printf("hey \n");
      }*/
    if (mainObj->nodeTable == NULL) {
        printf("hey again \n");
    }
    if (mainObj->blocks == NULL) {
        printf("hellow \n");
    }

    int i = 0;
    int j = 0;
    int k = 0;
    uint8_t blockers[16][1024];
    //printf("%zu \n", sizeof(mainObj->nodeTable));
    int h = 0;
    for (i = 0; i < 256; i++) {
        //printf("%d \n", i);
        h = i % 16;
        //printf("h is %d i is %d \n", h, i);
        for (j = 0; j < 6; j++) {
            (mainObj->nodeTable+i)->directPointers[j] = 0;
            //printf("%p test \n", (mainObj->nodeTable+i));
            //printf("%zu \n", (mainObj->nodeTable+i)->directPointers[j]);
            //printf("%p t2 \n", (mainObj->nodeTable+i));
            //blockers[i+1 / 16][((i % 16) * 64) + (j*2)] = (mainObj->nodeTable+i)->directPointers[j];
            blockers[i / 16][((h) * 64) + (j)] = 0;
            blockers[i / 16][((h) * 64) + j + 6] = 0; //second part of address
            //printf("%zu val \n", (mainObj->nodeTable+i)->directPointers[j]);
            //printf("%p i is: %d j is: %d \n", (mainObj->nodeTable+i), i, j);
        }
        //printf("%d \n", i);
        //printf("%p \n", (mainObj->nodeTable+i));

        //printf("%d htest \n", h);
        (mainObj->nodeTable+i)->indirectPointer = 0;
        blockers[i / 16][((h) * 64) + 12] = 0;
        //printf("%d i here \n", i);
        blockers[i / 16][((h) * 64) + 13] = 0;
        (mainObj->nodeTable+i)->doubleIndirect = 0;
        blockers[i / 16][((h) * 64) + 14] = 0;
        blockers[i / 16][((h) * 64) + 15] = 0;
        (mainObj->nodeTable+i)->inodeID = (uint8_t) i;
        blockers[i / 16][((h) * 64) + 16] = (mainObj->nodeTable+i)->inodeID;
        (mainObj->nodeTable+i)->subID = 0;
        blockers[i / 16][((h) * 64) + 17] = (mainObj->nodeTable+i)->subID;
        //(mainObj->nodeTable+i)->isFile = 0;
        blockers[i / 16][((h) * 64) + 18] = (mainObj->nodeTable+i)->isFile = 0;
        if (i == 0) {
            blockers[i / 16][((h) * 64) + 18] = (mainObj->nodeTable+i)->isFile = 2;
            uint8_t v = 16;
            bool ttt = block_store_request(mainObj->blocks, (size_t) v);
            if (ttt == false) {
                return NULL;
            }
            //blockers[0][0] = (v & 0xff);
            blockers[0][1] = v;
            mainObj->nodeTable->directPointers[0] = v;
        }
        blockers[i / 16][((h) * 64) + 19] = 0;
        blockers[i / 16][((h) * 64) + 20] = 0; //the endBlock

        uint16_t val = 1024;
        blockers[i / 16][((h) * 64) + 21] = (val & 0xff);
        blockers[i / 16][((h) * 64) + 22] = (val >> 8); //taken from stack overflow, should store 1024

        //printf("%p htest2 \n", mainObj);
        //blockers[i+1 / 16][((h) * 64) + 23] = (mainObj->nodeTable+i)->isOpen = 0;
        //blockers[i+1 / 16][((h) * 64) + 24] = (mainObj->nodeTable+i)->fd1Block = 0;
        for (k = 23; k < 36; k++) {
            //printf("k is %d h is %d i is %d at location %d %d %p \n", k, h, i, (i+1/16), (((h) * 64) + k), mainObj);
            blockers[i/ 16][((h) * 64) + k] = 0; //fds and isOpen
            // printf("%p \n", mainObj);
        }
        //printf("%d i mid \n", i);
        (mainObj->nodeTable+i)->endBlock = 0;
        (mainObj->nodeTable+i)->bytesLeft = 1024;
        (mainObj->nodeTable+i)->isOpen = 0;
        (mainObj->nodeTable+i)->fd1Block = 0;
        (mainObj->nodeTable+i)->fd1Loc = 0;
        (mainObj->nodeTable+i)->fd2Block = 0;
        (mainObj->nodeTable+i)->fd2Loc = 0;
        (mainObj->nodeTable+i)->fd3Block = 0;
        (mainObj->nodeTable+i)->fd3Loc = 0;

        //(mainObj->nodeTable+i)->fd3Block = 0;
        //(mainObj->nodeTable+i)->fd3Loc = 0;
        //printf("%p htest3 \n", mainObj);
        //printf("%d i mid \n", i);
        for (k = 0; k < 28; k++) {
            //printf("i is %d k is %d %d \n", i, k, (((h) * 64) + 36 + k));
            blockers[i / 16][(((h) * 64) + 36 + k)] = 0;
            (mainObj->nodeTable+i)->placeholder[k] = 0;
            //(mainObj->nodeTable+i)->placeholder[k] = 0;
        }
        /*for (k = 0; k < 28; k++) {
          (mainObj->nodeTable+i)->placeholder[k] = 0;
          }
          (mainObj->nodeTable+i)->isFile = 0;
          if (i == 0) {
          (mainObj->nodeTable+i)->isFile = 2;
          }
          */
        //(mainObj->nodeTable+i)->inodeID = (uint8_t) i;
        //(mainObj->nodeTable+i)->doubleIndirect = 0;
        //(mainObj->nodeTable+i)->indirectPointer = 0;
        //(mainObj->nodeTable+i)->endBlock = 0;
        //(mainObj->nodeTable+i)->bytesLeft = 1024;
        /*(mainObj->nodeTable+i)->fd1Block = 0;
          (mainObj->nodeTable+i)->fd1Loc = 0;
          (mainObj->nodeTable+i)->fd2Block = 0;
          (mainObj->nodeTable+i)->fd2Loc = 0;
          (mainObj->nodeTable+i)->fd3Block = 0;
          (mainObj->nodeTable+i)->fd3Loc = 0;*/
        //(mainObj->nodeTable+i)->subID = 0;
        //(mainObj->nodeTable+i)->isOpen = 0;
        //printf("%d i at end \n", i);
    }
    //S19FS_t *mainObj = malloc(sizeof(S19FS_t*));
    //printf("made it here ok \n");
    size_t catchT;
    //printf("%zu used 1\n", block_store_get_used_blocks(mainObj->blocks));
    //printf("%p \n", mainObj->blocks->b);
    for (i = 0; i < 16; i++) {
        if (block_store_request(mainObj->blocks, (size_t) i) == false) {
            return NULL;
        }

        catchT = block_store_write(mainObj->blocks, (size_t) i, blockers[i]);
        if (catchT == 0) {
            printf("something happened \n");
            return NULL;
        }

        //printf("%zu block \n", *(mainObj->blocks->data_blocks + i)); 
        //printf("%zu \n", block_store_get_used_blocks(mainObj->blocks));
    }
    //printf("%p \n", mainObj->blocks);
    catchT = block_store_serialize(mainObj->blocks, path);
    //printf("%zu used 2\n",block_store_get_used_blocks(mainObj->blocks));
    //printf("%zu \n", catchT);
    //printf("%zu \n", (mainObj->nodeTable+37)->indirectPointer);
    //printf("%zu \n", block_store_get_free_blocks(mainObj->blocks));

    //16 inodes per block
    //S19FS_t *main2 = (S19FS_t*) malloc(sizeof(S19FS_t));
    //main2->blocks = block_store_deserialize(path);
    //printf("%zu total set i \n", block_store_get_used_blocks(main2->blocks));
    strcpy(lastS19, path);
    return mainObj;
}






S19FS_t* fs_mount(const char* path) {
    if (path == NULL) {
        printf("cannot have null file path \n");
        return NULL;
    }
    if (strcmp(path, "") == 0) {
        return NULL; 
    }

    //int fd = creat(path, 0666);
    S19FS_t *mainObj = (S19FS_t*) malloc(sizeof(S19FS_t));
    mainObj->blocks = block_store_deserialize(path);
    //printf("%zu total set \n", bitmap_total_set(mainObj->blocks->b));
    if (mainObj->blocks == NULL) {
        return NULL;
    }
    //printf("%zu total set \n", bitmap_total_set(mainObj->blocks->b));
    //printf("%zu usd after mount \n", block_store_get_used_blocks(mainObj->blocks));
    uint8_t blockers[16][1024];
    int i = 0;
    size_t catchT;
    for (i = 0; i < 16; i++) {
        catchT = block_store_read(mainObj->blocks, i, blockers[i]);
        if (catchT == 0) {
            return NULL;
        }
    }
    int h = 0;
    int j = 0;
    //int k = 0;
    uint16_t temp;
    for (i = 0; i < 256; i++) {
        h = i % 16;
        for (j = 0; j < 6; j++) {
            temp = (uint16_t) blockers[i/16][((h) * 64) + (j*2)] << 8 | blockers[i/16][((h) * 64) + (j*2) + 1];
            (mainObj->nodeTable+i)->directPointers[j] = temp;
            /*if (i == 0 && j == 0) {
              printf("%u direct pointer \n", temp);
              }*/

        }
        temp = (uint16_t) blockers[i/16][((h) * 64) + 12] << 8 | blockers[i/16][((h) * 64) + 13];
        (mainObj->nodeTable+i)->indirectPointer = temp;
        temp = (uint16_t) blockers[i/16][((h) * 64) + 14] << 8 | blockers[i/16][((h) * 64) + 15];
        (mainObj->nodeTable+i)->doubleIndirect = temp;
        (mainObj->nodeTable+i)->inodeID = blockers[i/16][((h) * 64) + 16];
        (mainObj->nodeTable+i)->subID = blockers[i/16][((h) * 64) + 17];
        (mainObj->nodeTable+i)->isFile = blockers[i/16][((h) * 64) + 18]; 
        temp = (uint16_t) blockers[i/16][((h) * 64) + 19] << 8 | blockers[i/16][((h) * 64) + 20];
        (mainObj->nodeTable+i)->endBlock = temp;
        temp = (uint16_t) blockers[i/16][((h) * 64) + 21] << 8 | blockers[i/16][((h) * 64) + 22];
        (mainObj->nodeTable+i)->bytesLeft = temp;
        (mainObj->nodeTable+i)->isOpen = blockers[i/16][((h) * 64) + 23];
        temp = (uint16_t) blockers[i/16][((h) * 64) + 24] << 8 | blockers[i/16][((h) * 64) + 25];
        (mainObj->nodeTable+i)->fd1Block = temp;
        temp = (uint16_t) blockers[i/16][((h) * 64) + 26] << 8 | blockers[i/16][((h) * 64) + 27];
        (mainObj->nodeTable+i)->fd1Loc = temp;
        temp = (uint16_t) blockers[i/16][((h) * 64) + 28] << 8 | blockers[i/16][((h) * 64) + 29];
        (mainObj->nodeTable+i)->fd2Block = temp;
        temp = (uint16_t) blockers[i/16][((h) * 64) + 30] << 8 | blockers[i/16][((h) * 64) + 31];
        (mainObj->nodeTable+i)->fd2Loc = temp;
        temp = (uint16_t) blockers[i/16][((h) * 64) + 32] << 8 | blockers[i/16][((h) * 64) + 33];
        (mainObj->nodeTable+i)->fd3Block = temp;
        temp = (uint16_t) blockers[i/16][((h) * 64) + 34] << 8 | blockers[i/16][((h) * 64) + 35];
        (mainObj->nodeTable+i)->fd3Loc = temp;
        //temp = (uint16_t) blockers[i/16][((h) * 64) + 32] << 8 | blockers[i/16][((h) * 64) + 33];
        //(mainObj->nodeTable+i)->fd3Block = temp;
        //temp = (uint16_t) blockers[i/16][((h) * 64) + 34] << 8 | blockers[i/16][((h) * 64) + 35];
        //(mainObj->nodeTable+i)->fd3Loc = temp;
        for (j = 36; j < 63; j++) {
            (mainObj->nodeTable+i)->placeholder[j-36] = blockers[i/16][((h) * 64) + j];
        }
    }
    strcpy(lastS19, path);
    //printf("%zu used after mount\n", block_store_get_used_blocks(mainObj->blocks));
    //printf("%p \n", mainObj->blocks->b);
    return mainObj;
}

int fs_unmount(S19FS_t *fs) {
    //fs = NULL;
    //char path[] = "a.S19FS";
    //int i = 0;
    //int h;
    //int j= 0;
    if (fs == NULL) {
        return -1;
    }

    //int i = 0;


    //size_t catchT;
    //catchT = block_store_serialize(fs->blocks, path);
    /*if (catchT == 0) {
      return -1;
      }
      */
    /*if (fs->nodeTable != NULL) {
      free(fs->nodeTable);
      }*/
    //time to do it
    if (lastS19 == NULL) {
        printf("somethings wrong \n");
        return -1;
    }
    uint8_t blockers[16][1024];
    int i = 0;
    size_t catchT;
    int h = 0;
    int j = 0;
    uint16_t temp;
    uint8_t temp1;
    uint8_t temp2;
    for (i = 0; i < 256; i++) {//copy the contents of nodeTable to first 16 blocks, then serialize with block_store_serialize
        h = i % 16;
        for (j = 0; j < 6; j++) {
            temp = (fs->nodeTable+i)->directPointers[j];
            temp1 = (temp & 0xff);
            temp2 = (temp >> 8);
            blockers[i/16][((h) * 64) + (j*2)] = temp2;
            blockers[i/16][((h) * 64) + (j*2) + 1] = temp1;
        }
        temp = (fs->nodeTable+i)->indirectPointer;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 12] = temp2;
        blockers[i/16][((h) * 64) + 13] = temp1;
        temp = (fs->nodeTable+i)->doubleIndirect;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 14] = temp2;
        blockers[i/16][((h) * 64) + 15] = temp1;
        blockers[i/16][((h) * 64) + 16] = (fs->nodeTable+i)->inodeID;
        blockers[i/16][((h) * 64) + 17] = (fs->nodeTable+i)->subID;
        blockers[i/16][((h) * 64) + 18] = (fs->nodeTable+i)->isFile;
        temp = (fs->nodeTable+i)->endBlock;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 19] = temp2;
        blockers[i/16][((h) * 64) + 20] = temp1;
        temp = (fs->nodeTable+i)->bytesLeft;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 21] = temp2;
        blockers[i/16][((h) * 64) + 22] = temp1;
        blockers[i/16][((h) * 64) + 23] = (fs->nodeTable+i)->isOpen;
        temp = (fs->nodeTable+i)->fd1Block;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 24] = temp2;
        blockers[i/16][((h) * 64) + 25] = temp1;
        temp = (fs->nodeTable+i)->fd1Loc;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 26] = temp2;
        blockers[i/16][((h) * 64) + 27] = temp1;
        temp = (fs->nodeTable+i)->fd2Block;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 28] = temp2;
        blockers[i/16][((h) * 64) + 29] = temp1;
        temp = (fs->nodeTable+i)->fd2Loc;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 30] = temp2;
        blockers[i/16][((h) * 64) + 31] = temp1;
        temp = (fs->nodeTable+i)->fd3Block;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 32] = temp2;
        blockers[i/16][((h) * 64) + 33] = temp1;
        temp = (fs->nodeTable+i)->fd3Loc;
        temp1 = (temp & 0xff);
        temp2 = (temp >> 8);
        blockers[i/16][((h) * 64) + 34] = temp2;
        blockers[i/16][((h) * 64) + 35] = temp1;
        for (j = 36; j < 64; j++) { 
            blockers[i/16][((h) * 64) + j] = 0;
        }
    }//now we have everything copied in data
    for (i = 0; i < 16; i++) {
        catchT = block_store_write(fs->blocks, (size_t) i, blockers[i]);
        if (catchT == 0) {
            printf("something went wrong \n");
            return -1;
        }
    }
    catchT = block_store_serialize(fs->blocks, lastS19);
    if (catchT == 0) {
        printf("something went wrong \n");
        return -1;
    }

    block_store_destroy(fs->blocks);
    free(fs->nodeTable);

    /*uint8_t blockers[16][256];
      for (i = 0; i < 256; i++) {
      h = i % 16;
      for (j = 0; j < 6; j++) {
      blockers[i/16][((h) * 64) + 
      }
      }
      */
    return 0;
}

int fs_create(S19FS_t *fs, const char *path, file_t type) {
    if (path == NULL) {
        return -1;
    }
    if (fs == NULL) {
        return -1;
    }
    if (type != FS_DIRECTORY && type != FS_REGULAR) {
        return -1;
    }
    if (strcmp(path, "/") == 0) {
        printf("just a slash \n");
        return -1;
    }

    //if (type == FS_REGULAR)
    //note: have built in limit of 8 subdirectories, i.e. "/sub1/sub2/sub3/sub4/sub5/sub6/sub7"
    //char list[8][32];
    //char **list = malloc(sizeof(char) * 32 * 8);
    char list[8][32];
    int i = 0;
    //char *p = path;
    //const char *delim = "/";
    int j = 0;
    int k = 0;
    //printf("here \n");
    //printf("%s the path \n", path);
    //printf("%c \n", path[0]);
    while (path[i] != '\0') {
        if (path[i] == '/') {
            //list[j][k] = '/';
            list[j][k] = '\0';
            j++;
            k = 0;
        } else {
            list[j][k] = path[i];
            k++;
            if (k >= 31) {
                printf("sorry, one of those params were too long \n");
                //free(list);
                return -1;
            }

        }
        i++;
        //printf("%d \n", i);
    }
    list[j][k] = '\0';
    if (j < 1) {
        printf("empty \n");
        //free(list);
        return -1;
    }

    if (strlen(list[0]) != 0) {
        printf("missing slash or something \n");
        //free(list);
        return -1;
    }
    int lastFile = j;
    int mFlag = 0;
    int entryID = 0;
    size_t ifError;
    uint16_t tempPointer = 0;
    uint8_t *tempBlock = malloc(1024);
    uint8_t *inodeIDP = malloc(1);
    uint16_t lastPointer = 0;
    //int toLook = 0;
    //printf("list[1] is %s \n", list[1]);
    //printf("path again is %s \n", path);
    //size_t tempID;
    //printf("%d last file \n", lastFile);
    //printf("%u root direct \n", fs->nodeTable->directPointers[0]);
    for (i = 1; i <= lastFile; i++) {
        if (mFlag == -1) {
            break;
        }

        for (k = 0; k < 256; k++) {
            /*if (mFlag == 2) {
              k = toLook;
              printf("%d k is now \n", k);
              mFlag = 0;
              }*/
            if (k < mFlag) {
                k = mFlag;
                //printf("jumped to dir %d \n", k);
            }

            if (k >= 255 &&  i < lastFile) {
                //printf("%d i %s \n", i, list[i]);
                printf("couldn't find the requested directory \n");
                free(tempBlock);
                free(inodeIDP);
                return -1;//couldn't find it
            }

            if ((fs->nodeTable+k)->isFile == 2) {
                //printf("test %d \n", k);
                tempPointer = (fs->nodeTable+k)->directPointers[0];
                //printf("at k is %u \n", tempPointer); 
                if (i != lastFile) {
                    ifError = block_store_read(fs->blocks, tempPointer, tempBlock);
                    entryID = searchDirEntry(list[i], tempBlock);
                    //printf("entryID %d \n", entryID);
                    if (entryID == -1) {
                        //keep looking
                    } else {
                        //printf("isFile G %u of %s %d \n", (fs->nodeTable + entryID)->isFile, list[i], entryID);
                        if ((fs->nodeTable + entryID)->isFile != 2) {
                            printf("bad request, folder is already a file \n");
                            free(tempBlock);
                            free(inodeIDP);
                            return -1;
                        }

                        lastPointer = tempPointer;
                        mFlag = entryID;
                        //printf("%d mFlag \n", mFlag);
                        //toLook = k;
                        break; //found it so no need to keep looking
                    }
                } else if (i == lastFile) {
                    lastPointer = (fs->nodeTable+k)->directPointers[0];
                    /*if (mFlag == 2) {
                      mFlag = 0;
                      }*/

                    ifError = block_store_read(fs->blocks, lastPointer, tempBlock);
                    //printf("%s list[i], %u lastPointer \n", list[i], lastPointer);
                    entryID = searchDirEntry(list[i], tempBlock);

                    if (entryID == -1) {
                        ifError = inodeMake(fs, type, (fs->nodeTable+k)->subID, inodeIDP);
                        if (ifError == 0) { //we are full! 
                            printf("somehow we have too many files, directory full \n");
                            free(tempBlock);
                            free(inodeIDP);
                            return -1;
                        }
                        //printf("%s list[i] %u inodeIDP %u inodeID %u tempblock\n", list[i], *inodeIDP, (uint8_t) k, tempBlock[0]);
                        writeEntry(fs, list[i], *inodeIDP, tempBlock, (uint8_t) k);
                        size_t t5 = block_store_write(fs->blocks, lastPointer, tempBlock);
                        if (t5 == 0) {
                            printf("something messed up \n");
                        }

                        //printf("we wrote it! \n");
                        //uint8_t t3 = *inodeIDP;
                        //printf("%u isFile %d \n", (fs->nodeTable+t3)->isFile, k);
                        mFlag = -1;
                        break;
                    } else {
                        //got a match
                        //printf("%u subID, %d inodeID, %d k, %d i \n", (fs->nodeTable+entryID)->subID, entryID, k, i);
                        //if ((fs->nodeTable+entryID)->subID == (uint8_t) i) {
                        //printf("file has already been made in that location %d entryID %d %u %u pointing to %u last %u \n", k, entryID, *tempBlock, (fs->nodeTable+entryID)->subID, (fs->nodeTable+entryID)->directPointers[0], lastPointer);
                        free(tempBlock); //file has already been made in that location
                        free(inodeIDP);
                        return -1;
                        // }

                    }
                }
            }
        }
    }
    free(tempBlock);
    free(inodeIDP);
    return 0;
}



//helper function that writes entry to the directory file its in
void writeEntry(S19FS_t *fs, char *fname, uint8_t id, uint8_t *block, uint8_t inodeID) {//the fs, name of the file to write on the dir block, the id of file to write on dir block, the dir block, the id of the dir on the inodeTable
    int i = 0;
    int t = 0;
    for (i = 0; i < 31; i++) {
        if (block[i*33] == 0) {
            t = i;
            break;
        }
    }
    //printf("value of t is %d \n", t);
    block[t*33] = id;
    int k = strlen(fname);
    for (i = 1; i <= k; i++) {
        block[(t*33) + i] = (uint8_t) fname[i-1];
    }
    (fs->nodeTable+inodeID)->bytesLeft -= 33;
    //done?          
}



//helper function that looks for a match inside a block directory file
int searchDirEntry(char *li, uint8_t *block) {
    int i = 0;
    uint8_t id;
    int j = 0;
    char holder[32];
    for (i = 0; i < 31; i++) { //style is inodeID(1byte), fileName(32byte)
        id = block[i*33];
        if (id == 0) {
            //didn't find entry
            return -1;
        }
        for (j = 1; j < 33; j++) {
            holder[j-1] = (char) block[(i*33) + j]; //will it convert uint8_t to char ok?
        }
        //printf("%s found \n", holder);
        if (strcmp(li, holder) == 0) {
            //found it
            return (int) id;
        }
    }
    return -1;
}

//helper function that creates a new inode and updates nodeTable
size_t inodeMake(S19FS_t *fs, file_t type, uint8_t directoryID, uint8_t *idp) {
    int i = 0;
    size_t temp;
    for (i = 1; i < 256; i++) {
        if ((fs->nodeTable+i)->isFile == 0) {//look for inode that hasn't been made yet
            if (type == FS_DIRECTORY) {
                (fs->nodeTable+i)->isFile = 2;
                temp = block_store_allocate(fs->blocks);
                (fs->nodeTable+i)->directPointers[0] = (uint16_t) temp;
                (fs->nodeTable+i)->endBlock = (uint16_t) temp;
                (fs->nodeTable+i)->subID = directoryID;
                idp[0] = (uint8_t) i; //return inodeID for write to block
                return temp;
            }
            if (type == FS_REGULAR) {
                (fs->nodeTable+i)->isFile = 1;
                temp = block_store_allocate(fs->blocks);
                (fs->nodeTable+i)->directPointers[0] = (uint16_t) temp;
                (fs->nodeTable+i)->endBlock = (uint16_t) temp;
                (fs->nodeTable+i)->subID = directoryID;
                idp[0] = (uint8_t) i;
                return temp;
            }
        }
    }
    return 0;
}

int fs_open(S19FS_t *fs, const char *path) {
    if (fs == NULL) {
        return -1;
    }
    if (path == NULL) {
        return -1;
    }
    if (strcmp(path, "") == 0) {
        return -1;
    }
    //return 0;
    //printf("path is %s \n", path);
    char list[8][32];
    int i = 0;
    int j = 0;
    int k = 0;
    while (path[i] != '\0') {
        if (path[i] == '/') {
            list[j][k] = '\0';
            j++;
            k = 0;
        } else {
            list[j][k] = path[i];
            k++;
            if (k >= 31) {
                printf("one of the params too long \n");
                return -1;
            }
        }
        i++;
    }
    list[j][k] = '\0';
    if (j < 1) {
        printf("empty \n");
        return -1;
    }
    if (strlen(list[0]) != 0) {
        return -1;
    }
    int lastFile = j;
    int mFlag = 0;
    int entryID = 0;
    size_t ifError;
    uint16_t tempPointer = 0;
    uint8_t *tempBlock = malloc(1024);
    uint16_t lastPointer = 0;
    for (i = 1; i <= lastFile; i++) {
        for (k = 0; k < 256; k++) {
            if (k < mFlag) {
                k = mFlag;
            }
            if (k >= 255 && i < lastFile) {
                free(tempBlock);
                //couldn't find requested file
                return -1;
            }

            if ((fs->nodeTable+k)->isFile == 2) {
                tempPointer = (fs->nodeTable+k)->directPointers[0];
                if (i != lastFile) {
                    ifError = block_store_read(fs->blocks, tempPointer, tempBlock);
                    if (ifError == 0) {
                        printf("somethings wrong \n");
                        free(tempBlock);
                        return -1;
                    }


                    entryID = searchDirEntry(list[i], tempBlock);
                    if (entryID == -1) {
                        //keep looking
                    } else {
                        if ((fs->nodeTable + entryID)->isFile != 2) {
                            //badRequest
                            free(tempBlock);
                            return -1;
                        }

                        lastPointer = tempPointer;
                        mFlag = entryID;
                        break;
                    }
                } else if (i == lastFile) {
                    lastPointer = (fs->nodeTable+k)->directPointers[0];
                    ifError = block_store_read(fs->blocks, lastPointer, tempBlock);
                    entryID = searchDirEntry(list[i], tempBlock);
                    if (entryID == -1) {
                        //couldn't find it
                        free(tempBlock);
                        return -1;
                    } else {
                        //found file?
                        if ((fs->nodeTable+entryID)->isFile == 1) {
                            if ((fs->nodeTable+entryID)->isOpen == 3) {
                                //printf("already open thrice \n"); //instead of a limit of 256 file descriptors, each inode has 3 internal file descriptors which point to the block and the byte location inside that block
                                free(tempBlock);
                                return -1;
                            } else {
                                if ((fs->nodeTable+entryID)->isOpen == 0) { //could probably refactor this but whatever
                                    (fs->nodeTable+entryID)->isOpen = 1;
                                    (fs->nodeTable+entryID)->fd1Block = (fs->nodeTable+entryID)->directPointers[0];
                                    (fs->nodeTable+entryID)->fd1Loc = 0;
                                    free(tempBlock);
                                    uint16_t fd1Block = (fs->nodeTable+entryID)->fd1Block;
                                    uint16_t fd1Loc = (fs->nodeTable+entryID)->fd1Loc;
                                    uint32_t fd = (fd1Block << 16) + fd1Loc;
                                    return (int) fd;
                                }
                                if ((fs->nodeTable+entryID)->isOpen == 1) {
                                    (fs->nodeTable+entryID)->isOpen = 2;
                                    (fs->nodeTable+entryID)->fd2Block = (fs->nodeTable+entryID)->directPointers[0];
                                    (fs->nodeTable+entryID)->fd2Loc = 0;
                                    free(tempBlock);
                                    uint16_t fd2Block = (fs->nodeTable+entryID)->fd2Block;
                                    uint16_t fd2Loc = (fs->nodeTable+entryID)->fd2Loc;
                                    uint32_t fd = (fd2Block << 16) + fd2Loc;
                                    return (int) fd;
                                }
                                if ((fs->nodeTable+entryID)->isOpen == 2) {
                                    (fs->nodeTable+entryID)->isOpen = 3;
                                    (fs->nodeTable+entryID)->fd3Block = (fs->nodeTable+entryID)->directPointers[0];
                                    free(tempBlock);
                                    uint16_t fd3Block = (fs->nodeTable+entryID)->fd3Block;
                                    uint16_t fd3Loc = 0;
                                    uint32_t fd = (fd3Block << 16) + fd3Loc;
                                    return (int) fd;
                                }
                            }
                        }
                        //wasn't a file, terminate
                        free(tempBlock);
                        return -1;
                    }
                }
            }
        }
    }
    free(tempBlock);

    //uint8_t *tBlock = malloc(1024);

    //free(tBlock);
    return 0;
}

int fs_close(S19FS_t *fs, int fd) {
    if (fs == NULL) {
        return -1;
    }
    if (fd <= 0) {
        return -1;
    }
    uint32_t point = (uint32_t) fd;
    //printf("point: %u \n", point);
    uint16_t fdBlock = (point >> 16);
    uint16_t fdLoc = (uint16_t) point;
    //printf("fdBlock: %u \n", fdBlock);
    //printf("fdLoc: %u \n", fdLoc);
    int i = 0;
    //int j = 0;
    for (i = 0; i < 256; i++) {
        if ((fs->nodeTable+i)->isFile == 1) {
            //now check to make sure it has the same fdBlock and fdLoc
            //since fdBlock and fdLoc is unique for each file
            if ((fs->nodeTable+i)->fd3Block == fdBlock && (fs->nodeTable+i)->fd3Loc == fdLoc) {
                //found the descriptor!
                (fs->nodeTable+i)->fd3Block = 0;
                (fs->nodeTable+i)->fd3Loc = 0;
                (fs->nodeTable+i)->isOpen = 2;
                return 0;
            }
            if ((fs->nodeTable+i)->fd2Block == fdBlock && (fs->nodeTable+i)->fd2Loc == fdLoc) {
                (fs->nodeTable+i)->fd2Block = 0;
                (fs->nodeTable+i)->fd2Loc = 0;
                (fs->nodeTable+i)->isOpen = 1;
                return 0;
            }
            if ((fs->nodeTable+i)->fd1Block == fdBlock && (fs->nodeTable+i)->fd1Loc == fdLoc) {
                (fs->nodeTable+i)->fd1Block = 0;
                (fs->nodeTable+i)->fd1Loc = 0;
                (fs->nodeTable+i)->isOpen = 0;
                return 0;
            } //easy!
        }
    }
    return -1;
}

dyn_array_t *fs_get_dir(S19FS_t *fs, const char *path) {
    if (fs == NULL) {
        return NULL;
    }
    if (path == NULL) {
        return NULL;
    }
    if (strcmp(path, "") == 0) {
        return NULL;
    }
    int i = 0;
    int j = 0;
    int k = 0;
    char list[8][32];
    while (path[i] != '\0') { //get each of the folders 1 by 1
        if (path[i] == '/') {
            list[j][k] = '\0';
            j++;
            k = 0;
        } else {
            list[j][k] = path[i];
            k++;
            if (k >= 31) {
                printf("params too long \n");
                return NULL;
            }
        }
        i++;
    }
    list[j][k] = '\0';
    if (j < 1) {
        printf("empty \n");
        return NULL;
    }
    //printf("path %s \n", path);
    //printf("j: %d is %s \n", j, list[j]);
    if (strcmp(path, "/") == 0) {
        dyn_array_t *dynArr = dyn_array_create(0, sizeof(file_record_t), NULL);
        file_record_t *rec = (file_record_t*) malloc(sizeof(file_record_t) * 31);
        int x = 0;
        int x2 = 0;
        uint8_t tID =0;
        uint8_t *bbb = malloc(1024);
        block_store_read(fs->blocks, 16, bbb);
        bool tester;
        for (x = 0; x < 31; x++) {
            tID = bbb[x*33];
            if (tID == 0) {
                free(rec);
                free(bbb);
                return dynArr;
            }
            for (x2 = 1; x2 < 33; x2++) {
                (rec+x)->name[x2-1] = (char) bbb[(x*33) + x2];
            }
            if ((fs->nodeTable+tID)->isFile == 2) {
                (rec+x)->type = FS_DIRECTORY;
            } else {
                (rec+x)->type = FS_REGULAR;
            }
            tester = dyn_array_push_front(dynArr, (rec+x));
            if (tester == false) {
                printf("something happened \n");
            }
        }
        free(rec);
        free(bbb);
        return dynArr;
    }
    //return NULL;
    int mFlag = 0;
    int lastFile = j;
    size_t ifError;
    uint8_t *tempBlock = malloc(1024);
    uint16_t lastPointer = 0;
    uint16_t tempPointer = 0;
    int entryID = 0;
    for (i = 1; i <= lastFile; i++) {//same process as before, just keep going through until find last entry and then quit
        for (k = 0; k < 256; k++) {
            if (k < mFlag) {
                k = mFlag;
                //printf("jumped to %d \n", k);
            }
            if (k >= 255 && i < lastFile) {
                printf("dir not found \n");
                //printf("i: %d list[i]: %s \n", i, list[i]);
                free(tempBlock);
                return NULL;
            }
            if ((fs->nodeTable+k)->isFile == 2) {
                tempPointer = (fs->nodeTable+k)->directPointers[0];
                if (i != lastFile) {
                    ifError = block_store_read(fs->blocks, tempPointer, tempBlock);
                    if (ifError == 0) {
                        printf("something went wrong \n");
                    }

                    entryID = searchDirEntry(list[i], tempBlock);
                    if (entryID == -1) {
                        //keep looking
                    } else {
                        if ((fs->nodeTable + entryID)->isFile != 2) {
                            printf("bad request \n");
                            free(tempBlock);
                            return NULL;
                        }
                        mFlag = entryID;
                        break;
                    }
                } else if (i == lastFile) {
                    //printf("k is %d i is %d list[i] %s temp %u \n", k, i, list[i], tempPointer);
                    lastPointer = (fs->nodeTable+k)->directPointers[0];
                    ifError = block_store_read(fs->blocks, lastPointer, tempBlock);
                    entryID = searchDirEntry(list[i], tempBlock);
                    if (entryID == -1) {
                        //dir not found
                        free(tempBlock);
                        return NULL;
                    } else {
                        dyn_array_t *dynArr = dyn_array_create(0, sizeof(file_record_t), NULL);
                        file_record_t *rec = (file_record_t*) malloc(sizeof(file_record_t) * 31);
                        int x = 0;
                        int x2 = 0;
                        uint8_t tID = 0;
                        bool tester;
                        uint16_t p = (fs->nodeTable+entryID)->directPointers[0];
                        if ((fs->nodeTable+entryID)->isFile != 2) {
                            free(tempBlock);
                            free(rec);
                            dyn_array_destroy(dynArr);
                            return NULL;
                        }

                        block_store_read(fs->blocks, (size_t) p, tempBlock);
                        for (x = 0; x < 31; x++) {
                            tID = tempBlock[x*33];
                            if (tID == 0) {
                                free(rec);
                                free(tempBlock);
                                return dynArr;
                            }
                            for (x2 = 1; x2 < 33; x2++) {
                                (rec+x)->name[x2-1] = (char) tempBlock[(x*33) + x2];
                            }
                            if ((fs->nodeTable+tID)->isFile == 2) {
                                (rec+x)->type = FS_DIRECTORY;
                            } else {
                                (rec+x)->type = FS_REGULAR;
                            }
                            tester = dyn_array_push_front(dynArr, (rec+x));
                            if (tester == false) {
                                printf("there was a problem \n");
                            }
                        }
                        free(tempBlock);
                        return dynArr;
                        //printf("entryID: %d k: %d i: %d lastPointer: %u list[i] %s \n", entryID, k, i, lastPointer, list[i]);
                        //found directory
                    }

                }
            }
        }
    }
    free(tempBlock);
    return NULL;


}

/*pseudocode: 
 * will have another function on teardown which saves changes to a mainObj to a buffer that copies changes to a file
 * fs_create will create a file by getting the first available inode and getting another block for the first direct pointer and sets metadata
 * fs_open will just open by changing the isOpen metadata for the right inode and getting the matching filename and inode
 * fs_close will do pretty much the same thing except closing instead of opening. Will also copy the blocks of the inode to the file data
 * fs_get_dir will use the path and the subdirectory metadata and seek out each inode with a matching subdirectory id and add it to the dyn_array
 * will also need to update fs_mount and fs_format, need to include file names in the placeholder and make other changes

*/

/*pseudocode2: 
 * for fs_seek, all I have to do is change the value of the fd once I find a match. Then change it's value based on offset and whence, making sure file is open too
 * for fs_write and fs_read, first find match within inode that has same fd 
 * then write/read byte by byte at file. when no more bytes, either move to next file if already allocated or allocate a new block for direct pointer, indirect pointer or double indirect depending on where we're at
 * for fs_remove, searches for dir/file within path that matches, same as before, then free the blocks it's associated with (backwards, from double indirect to indirect to direct for large files), and also set the bitmap to free using block_store_release on the specified byte. Also need to update inode so that the location is free
 */



