#include "myalloc.h"
#include <stdio.h>
#include <stdbool.h>

// A simple test program to show the  use of the library
// By default, myalloc() simply calls malloc() so this will
// work.
// Kasim Terzic, Sep 2017

// Tests basic allocation and freeing.
bool test1() {
  printf("Test 1 - Basic allocation \t - ");

  int* ptr = myalloc(1024*sizeof(int));
  ptr[0] = 42;
  ptr[1023] = 25;

  int* ptr2 = myalloc(128*sizeof(int));
  ptr2[0] = 68;
  ptr2[127] = 419;

  if (ptr[0] != 42 || ptr[1023] != 25 ) {
    printf("Test 1 failed.\n");
    myfree(ptr);
    myfree(ptr2);
    return false;
  }

  if (ptr2[0] != 68 || ptr2[127] != 419 ) {
    printf("Test 1 failed.\n");
    myfree(ptr);
    return false;
  }

  myfree(ptr);
  myfree(ptr2);
  printf("Test 1 passed.\n");

  return true;
}

// Tests freeing in different order than allocation
bool test2() {

  int* ptr;
  int* ptr2;

  printf("Test 2 - Freeing memory \t - ");

  ptr = myalloc(1024*sizeof(int));
  ptr2 = myalloc(512*sizeof(int));


  ptr[0] = 100;
  ptr[1023] = 25;
  ptr2[0] = 400;
  ptr2[511] = 50;

  myfree(ptr2);
  myfree(ptr);

  printf("Test 2 passed.\n");

  return true;
}

// Tests backwards coalescing.
bool test3() {
  printf("Test 3 - Backward Coalescing \t - ");

  int size = 512*sizeof(int);

  int* ptr_start = myalloc(size);
  int* ptr_mid1 = myalloc(size);
  int* ptr_mid2 = myalloc(size);
  int* ptr_end = myalloc(size);

  int* ptr_compare = ptr_mid1;

  myfree(ptr_mid1);
  myfree(ptr_mid2);

  int* ptr_mid = myalloc(size * 2);

  myfree(ptr_start);
  myfree(ptr_end);

  if (ptr_compare == ptr_mid) {
    myfree(ptr_mid);
    printf("Test 3 passed.\n");
    return true;
  }

  myfree(ptr_mid);

  printf("Test 3 failed. %ld %ld\n", ptr_compare, ptr_mid);

  return false;
}

// Test forwards coalescing
bool test4() {
  printf("Test 4 - Forwards Coalescing \t - ");

  int size = 512*sizeof(int);

  int* ptr_start = myalloc(size);
  int* ptr_mid1 = myalloc(size);
  int* ptr_mid2 = myalloc(size);
  int* ptr_end = myalloc(size);

  int* ptr_compare = ptr_mid1;

  myfree(ptr_mid2);
  myfree(ptr_mid1);

  int* ptr_mid = myalloc(size * 2);

  myfree(ptr_start);
  myfree(ptr_end);

  if (ptr_compare == ptr_mid) {
    myfree(ptr_mid);
    printf("Test 4 passed.\n");
    return true;
  }

  myfree(ptr_mid);

  printf("Test 4 failed. %ld %ld\n", ptr_compare, ptr_mid);

  return false;
}

// Tests forwards and backwards coalescing
bool test5() {
  printf("Test 5 - F and B Coalescing \t - ");

  int size = 512*sizeof(int);

  int* ptr_start = myalloc(size);
  int* ptr_mid1 = myalloc(size);
  int* ptr_mid2 = myalloc(size);
  int* ptr_mid3 = myalloc(size);
  int* ptr_end = myalloc(size);

  int* ptr_compare = ptr_mid1;

  myfree(ptr_mid3);
  myfree(ptr_mid1);
  myfree(ptr_mid2);

  int* ptr_mid = myalloc(size * 3);

  myfree(ptr_start);
  myfree(ptr_end);

  if (ptr_compare == ptr_mid) {
    myfree(ptr_mid);
    printf("Test 5 passed.\n");
    return true;
  }

  myfree(ptr_mid);

  printf("Test 5 failed. %ld %ld\n", ptr_compare, ptr_mid);

  return false;
}

bool test6() {
  printf("Test 6 - Different sizes \t - ");

  int size = 512*sizeof(int);

  int* ptr_1 = myalloc(size);
  int* ptr_2 = myalloc(size);
  int* ptr_3 = myalloc(size);

  int* ptr_compare = ptr_2;

  myfree(ptr_2);

  int* ptr_large = myalloc(size * 2);

  if (ptr_large == ptr_compare) {
    printf("Test 6 failed.\n");
    myfree(ptr_1);
    myfree(ptr_3);
    myfree(ptr_large);
    return false;
  }

  int* ptr_small = myalloc(size);

  if (ptr_small != ptr_compare) {
    printf("Test 6 failed.\n");
    myfree(ptr_1);
    myfree(ptr_3);
    myfree(ptr_large);
    myfree(ptr_small);
    return false;
  }

  myfree(ptr_1);
  myfree(ptr_3);
  myfree(ptr_large);
  myfree(ptr_small);

  printf("Test 6 passed.\n");

  return true;
}

void main()
{
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
}
