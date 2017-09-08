#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>
#include <assert.h>

void __attribute__ ((constructor)) setup(void);

#define PAGESIZE 4096
#define PAGEHEADER 36
#define BLOCKHEADER 8

void* pageBuilder(int listIndex, int size);

int fd;
void *pages[10];

void setup()
{
  fd = open("/dev/zero", O_RDWR);
  int i;
  for (i = 0; i < 10; i++)
    pages[i] = NULL;
}

void* malloc(size_t size)
{
  if (size == 0)
    return NULL;
  if (size == 1)
    size = 2;

  //find the index of the appropriate size
  int listIndex = (int) ceil(log2(size));

  if (listIndex <= 10)
    return pageBuilder((listIndex - 1), (int) pow(2.0, (double) listIndex));

  //if none of the indices found, make a really big page
  //and give it a special header
  void *page = mmap(NULL, size+PAGEHEADER+BLOCKHEADER, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);

  *((int *)page) = size;
  *((int *)(page + 4)) = 0;
  *((int *)(page + 8)) = size;

  *((void **)(page + 12)) = NULL;
  *((void **)(page + 20)) = NULL;
  *((void **)(page + 28)) = NULL;
  *((void **)(page + PAGEHEADER)) = page;

  return page + PAGEHEADER + BLOCKHEADER;
}

void* pageBuilder(int listIndex, int size)
{
  int numOfBlocks = (PAGESIZE - PAGEHEADER) / (BLOCKHEADER + size);

  void *page = pages[listIndex];
  while ((page != NULL) && (*((int *)(page)) == 0))
    page = *((void **)(page + 12));

  if (page == NULL)
  {
    //initializing a page...
    page = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);

    //setting current num of free blocks, num of possible blocks, size of blocks
    *((int *)(page)) = numOfBlocks;
    *((int *)(page + 4)) = numOfBlocks;
    *((int *)(page + 8)) = size;

    *((void **)(page + 12)) = pages[listIndex];

    //if there's already a page, move it to the back
    if (pages[listIndex] != NULL)
      *((void **)(pages[listIndex] + 20)) = page;
    *((void **)(page + 20)) = NULL;
    *((void **)(page + 28)) = page + PAGEHEADER;

    //hold beginning of free list
    void *bookmark = *((void **)(page + 28));

    int i;
    //move down appropriate # of blocks
    for (i = 0; i < (numOfBlocks - 1); i++)
    {
      *((void **)bookmark) = bookmark + BLOCKHEADER + size;
      bookmark = *((void **)bookmark);
    }

    //null terminate set of blocks
    *((void **)bookmark) = NULL;

    pages[listIndex] = page;
  }

  //get the head of the free list
  void *freeBlock = *((void **)(page + 28));

  *((void **)(page + 28)) = *((void **)freeBlock);

  //set block header
  *((void **)freeBlock) = page;

  //decrement number of free spaces
  *((int *)(page)) = *((int *)(page)) - 1;

  return freeBlock + BLOCKHEADER;
}

void* calloc(size_t num, size_t size)
{
  void *ptr = malloc((num * size));

  //clear memory
  memset(ptr, 0, (num * size));

  return ptr;
}

void* realloc(void *ptr, size_t size)
{
  int origSize = 0;

  if (size == 0)
  {
    free(ptr);
    return NULL;
  }

  if (ptr == NULL)
    return malloc(size);

  //move to block header to get page pointer
  void *block = ptr - BLOCKHEADER;

  void *page = *((void **)block);

  //check the size
  origSize = *((int *)(page + 8));

  void *temp = malloc(size);
  if (size < origSize)
    memcpy(temp, ptr, size);
  else
    memcpy(temp, ptr, origSize);

  free(ptr);

  return temp;
}

void free(void *ptr)
{
  if (ptr == NULL)
    return;

  void *block = ptr - BLOCKHEADER;
  void *page = *((void **)block);

  //if page is completely full, unmap it
  if (*((int *)(page + 4)) == 0)
  {
    munmap(page, *((int *)page) + PAGEHEADER + BLOCKHEADER);
    return;
  }

  int sizeOfBlocks = *((int *)(page + 8));

  //move the allocated block to the free list
  *((void **)block) = *((void **)(page + 28));
  *((void **)(page + 28)) = block;

  //increment number of free spaces
  *((int *)(page)) = *((int *)(page)) + 1;

  //for clearing out an empty page
  if (*((int *)page) == *((int *)(page + 4)))
  {
    void *previousPage = *((void **)(page + 20));
    void *nextPage = *((void **)(page + 12));

    if (previousPage != NULL)
      *((void **)(previousPage + 12)) = nextPage;
    else
    {
      int listIndex = (int) floor(log2(sizeOfBlocks));
      if (listIndex <= 10)
        pages[listIndex - 1] = nextPage;
    }

    if (nextPage != NULL)
      *((void **)(nextPage + 20)) = previousPage;

    munmap(page, PAGESIZE);
  }
}
