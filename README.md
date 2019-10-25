# malloc
An implementation of stdlib's malloc function for C using mmap and munmap.

Supports coalescing of adjacent free regions, word and page alignment, and has optimisations to reduce memory overhead to two words per allocated region, as well as reduce the time taken to traverse the free list to find a block of suitable size for allocation. 

Can be imported as a library by including the #include "myalloc.h" header, and appending the -lmyalloc argument to gcc when compiling.
