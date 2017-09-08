Earl Honeycutt (ehoneyc@g.clemson.edu)

This project includes a single .c file:

allocator.c

allocator.c compiles into a shared library (myliballoc.so) that contains user-level implementations for malloc(), free(), calloc() and realloc(), all using mmap for memory allocation through pages.

KNOWN PROBLEMS:

Page header is a bit chunky (36 bytes).

DESIGN:

My allocator makes use of two different headers: one for pages and one for individual blocks.
The page header contains: current # of blocks that can be allocated, max # of blocks that can 
potentially be created, the size of the page's blocks, and pointers to the next page, the previous
page, and the head of the free list. The block header contains a pointer to either the head of the
page (if the block is allocated) or to the next block in the free list (if unallocated).

http://man7.org/linux/man-pages/man2/mmap.2.html
https://linux.die.net/man/2/munmap
http://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html
https://www.cs.umd.edu/class/sum2003/cmsc311/Notes/BitOp/pointer.html

