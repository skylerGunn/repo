#include <stdio.h>
#include <stdint.h>
#include "bitmap.h"
#include "block_store.h"
#include "bitmap.c"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
// include more if you need

typedef struct block_store {
    bitmap_t *b; //the FBM, first block for metadata: stores if the rest of the 255 blocks are available
    uint8_t *data; //each block is 256 of these with 255 blocks 
}block_store_t;
//2048 bits per block, 256 bytes per block
//one blockstore has 256 blocks, with the first block being the FBM
//
extern int errno;
block_store_t *block_store_create() {
    const size_t nbits = 65536;
    block_store_t *blockStore = malloc(sizeof(block_store_t));
    blockStore->b = bitmap_create(nbits);
    int i = 0;
    for (i = 0; i < 8; i++) {
        bitmap_set(blockStore->b, (i + 65528));
    }

    (blockStore->data) = malloc(1024 * 65528); //256 bytes per block and 255 blocks
    for (i = 0; i < (1024 * 65528); i++) {
        *(blockStore->data+i) = 0;
    }

    return blockStore;
}


void block_store_destroy(block_store_t *const bs) {
    if (bs != NULL) {
        free(bs->data);
        bitmap_destroy(bs->b);
        free(bs);
    }
}


size_t block_store_allocate(block_store_t *const bs) {
    if (bs == NULL) {
        return SIZE_MAX;
    }
    size_t id = bitmap_ffz(bs->b);
    if (id != SIZE_MAX) {
        bitmap_set(bs->b, id);
    } else {
        return SIZE_MAX;
    }
    return id;
}


bool block_store_request(block_store_t *const bs, const size_t block_id) {
    if (bs == NULL) {
        return false;
    }
    if (bs->b == NULL) {
        return false;
    }
    /*if (block_id > 255) {
      return false;
      }*/
    if (bitmap_test(bs->b, block_id) == true) {
        return false;
    }
    /*if (block_id == 10 && bitmap_test(bs->b, 1) != false) {
      return false;
      }*/

    bitmap_set(bs->b, block_id);
    return true;
}

void block_store_release(block_store_t *const bs, const size_t block_id) {
    if (bs != NULL) {
        bitmap_reset(bs->b, block_id);
    }

}

size_t block_store_get_used_blocks(const block_store_t *const bs) { 
    if (bs == NULL) {
        return SIZE_MAX;
    }
    size_t s = bitmap_total_set(bs->b);
    return s;
}

size_t block_store_get_free_blocks(const block_store_t *const bs) {
    if (bs == NULL) {
        return SIZE_MAX;
    }
    size_t s = bitmap_total_set(bs->b);
    return 65536 - s;
}

size_t block_store_get_total_blocks() {
    return 65536; //do more here?
}

size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer) {
    if (bs == NULL) {
        return 0;
    }
    if (buffer == NULL) {
        return 0;
    }
    //memcpy(buffer, (bs->data + (block_id * 256)), 256);
    memcpy(buffer, (bs->data + (block_id * 1024)), 1024);
    return 1024;
}

size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer) {
    if (buffer == NULL) {
        return 0;
    }
    if (bs == NULL) {
        return 0;
    }
    memcpy((bs->data + (block_id * 1024)), buffer, 1024);
    return 1024;
}

block_store_t *block_store_deserialize(const char *const filename) {
    if (filename == NULL) {
        return NULL;
    }
    //block_store_t bst = block_store_create(); //create instance of empty block
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        //block_store_destroy(bst);
        printf("here \n");
        return NULL;
    }
    const size_t nbits = 65536;
    //bitmap_t *bStart = bitmap_create(nbits);
    uint8_t *dataBuffer = malloc(1024 * 8);
    //void *const bitmap_dataBuffer = malloc(sizeof(bStart));
    //bitmap_destory(bStart); //get rid of thing we made for the buffer
    size_t r = read(fd, dataBuffer, 1024 * 8); //first 8 blocks are fbm
    if (r == 0) {
        free(dataBuffer);
        //bitmap_destroy(bStart);
        return NULL;
    }
    //printf("%zu read \n", r);
    int i = 0;
    /*for (i = 0; i < 1024; i++) {
      printf("%u bit   ", dataBuffer[i]);
      }*/

    //bitmap_destroy(bStart);
    block_store_t *bst = malloc(sizeof(block_store_t));
    //bst->b = bitmap_import(nbits, dataBuffer);
    bst->b = bitmap_create(nbits);
    //int j = 0;
    //printf("%u \n", dataBuffer[0]);
    for (i = 0; i < 8192; i++) {
        /*if (i == 0) {
          printf("%d i %u \n", i, dataBuffer[i]);
          printf("%d \n", (dataBuffer[i] & 128));
          printf("%d \n", (dataBuffer[i] & 64));
          }*/

        //printf("%u ", dataBuffer[i]);
        if ((dataBuffer[i] & 128) > 0) {
            //printf("hi \n");
            bitmap_set(bst->b, (i*8) + 0);
        }
        //printf("%u ", dataBuffer[i]);

        if ((dataBuffer[i] & 64) > 0) {
            //printf("h \n");
            bitmap_set(bst->b, (i*8) + 1);

        }
        if ((dataBuffer[i] & 32) > 0) {
            bitmap_set(bst->b, (i*8) + 2);
        }
        if ((dataBuffer[i] & 16) > 0) {
            bitmap_set(bst->b, (i*8) + 3);
        }
        if ((dataBuffer[i] & 8) > 0) {
            bitmap_set(bst->b, (i*8) + 4);
        }
        if ((dataBuffer[i] & 4) > 0) {
            bitmap_set(bst->b, (i*8) + 5);
        }
        if ((dataBuffer[i] & 2) > 0) {
            bitmap_set(bst->b, (i*8) + 6);
            //printf("7 went through \n");
        }
        if ((dataBuffer[i] & 00000001) == 00000001) {
            //printf("worked once \n");
            bitmap_set(bst->b, (i*8) + 7);
        }
    }

    if (bst->b == NULL) {
        free(bst);
        free(dataBuffer);
        return NULL;//problem here
    }
    free(dataBuffer);
    //int i = 0;
    /*for (i = 0; i < 1024; i++) {
      printf("i %d is %d  ",i,  bitmap_test(bst->b, i));
      }*/
    bst->data = malloc(1024 * 65528);
    r = read(fd, bst->data, 65528 * 1024);
    if (r == 0) {
        free(bst->data);
        free(bst->b);
        return NULL;
    }
    close(fd);
    //printf("its %d \n", bitmap_test(bst->b, 10));
    //bitmap_set(bst->b, 10);
    return bst;
}

size_t block_store_serialize(const block_store_t *const bs, const char *const filename) {
    if (bs == NULL) {
        return 0;
    }
    if (filename == NULL) {
        return 0;
    }
    //char *buffer = malloc(sizeof(bs->b)); //may need sizeof(char) here
    int fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        printf("%d error is: %s \n",errno, strerror(errno));
        return 0; //should be impossible
    }
    //uint8_t *dataBuffer = malloc(1024 * 8);
    //printf("%zu total set \n", bitmap_total_set(bs->b));
    uint8_t *data = malloc(8192);
    memcpy(data, bs->b->data, 8192);
    //int i = 0;
    /*for (i = 0; i < 1024; i++) {
      printf("i %d is %u    ", i, data[i]);
      }*/

    size_t w1 = write(fd, data, 1024 * 8);
    //printf("%zu written \n", w1);
    //printf("%zu total set2 \n", bitmap_total_set(bs->b));
    if (w1 == 0) {
        printf("w1 wrong \n");
        return 0;
    }
    uint8_t *data2 = malloc(1024 * 65528);
    /*int i = 0;
      for (i = 0; i < (1024 * 65528); i++) {
      data2[i] = 0;
      }
      */
    //printf("size: %zu \n", sizeof(data2));
    memcpy(data2, bs->data, 1024 * 65528);
    //size_t w2 = write(fd, bs->data, 1024 * 65528);
    size_t w2 = write(fd, data2, 1024 * 65528);
    if (w2 == 0) {
        printf("w2 wrong \n");
        return 0;
    }
    close(fd);
    free(data);
    free(data2);
    //printf("%zu serialized \n", w1+w2);
    return (w1 + w2);
}

