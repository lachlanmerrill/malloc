#include "myalloc.h"
#include <stdio.h>

// A simple test program to show the  use of the library
// By default, myalloc() simply calls malloc() so this will
// work.
// Kasim Terzic, Sep 2017

void main()
{
    printf("Allocating 1024 * sizeof(int)... \n");

    int* ptr = myalloc(1024*sizeof(int));
    ptr[0] = 42;
    ptr[1023] = 25;

    printf("Value 0 is %d\nValue 1023 is %d\n", ptr[0], ptr[1023]);

    int* ptr2 = myalloc(128*sizeof(int));
    ptr2[0] = 69;
    ptr2[127] = 420;

    printf("In second array:\nValue 0 is %d\nValue 127 is %d\n", ptr2[0], ptr2[127]);

    printf("Freeing the allocated memory... \n");
    myfree(ptr);
    printf("First pointer freed\n");
    myfree(ptr2);
    printf("Second pointer freed\n");

    printf("Yay!\n");
}
