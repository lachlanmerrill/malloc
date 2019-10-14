#include "myalloc.h"
#include <stdio.h>

// A simple test program to show the  use of the library
// By default, myalloc() simply calls malloc() so this will
// work.
// Kasim Terzic, Sep 2017

void main()
{

    printf("Test 1\n");
    printf("Allocating 1024 * sizeof(int)... \n");

    int* ptr = myalloc(1024*sizeof(int));
    ptr[0] = 42;
    ptr[1023] = 25;

    int* ptr2 = myalloc(128*sizeof(int));
    ptr2[0] = 69;
    ptr2[127] = 420;
    printf("In first array:\nValue 0 is %d\nValue 1023 is %d\n", ptr[0], ptr[1023]);


    printf("In second array:\nValue 0 is %d\nValue 127 is %d\n", ptr2[0], ptr2[127]);

    printf("Freeing the allocated memory... \n");
    myfree(ptr);
    printf("First pointer freed\n");
    myfree(ptr2);
    printf("Second pointer freed\n");

    printf("Test 1 passed.\n");
    printf("Test 2\n");

    ptr = myalloc(1024*sizeof(int));
    printf("First pointer allocated.\n");
    ptr2 = myalloc(512*sizeof(int));

    printf("Memory allocated.\n");

    ptr[0] = 100;
    ptr[1023] = 25;
    ptr2[0] = 400;
    ptr2[511] = 50;

    printf("Values assigned.\n");

    myfree(ptr2);
    myfree(ptr);

    printf("Test 2 passed.\n");

    printf("Done.\n");
}
