
//#include <ctype.h>
#include <stdio.h>
#include <errno.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <getopt.h>
//#include <signal.h>  
//#include <sys/param.h>  
//#include <sys/types.h>  
//#include <sys/stat.h> 
#include <time.h> 
#include <sys/mman.h>
//#include "../include/daemonctl.h"1

#define SIZE_RAY 64

unsigned long difftime (timespec &t2, timespec &t1) {
  return ((t2.tv_sec-t1.tv_sec)*1000000000 + (t2.tv_nsec-t1.tv_nsec));
}

int
main (int , char **) {
  struct timespec  ts1, ts2, ts3, ts4;
  char* smallpagestart;
  char* nowstart, *nowend;

  for (int loop = 0; loop < 1280; loop++) {
    clock_gettime(CLOCK_MONOTONIC, &(ts1));

    smallpagestart = static_cast<char*>
      (mmap (0, SIZE_RAY << 21, PROT_READ | PROT_WRITE,
	     MAP_HUGETLB | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    clock_gettime(CLOCK_MONOTONIC, &(ts2));

    nowstart = smallpagestart;
    nowend = nowstart + (SIZE_RAY << 21);
    do {
      *nowstart = 0x67;
      nowstart += 4096;
    }
    while (nowstart < nowend);
    clock_gettime(CLOCK_MONOTONIC, &(ts3));

    nowstart = smallpagestart + 1;
    nowend = nowstart + (SIZE_RAY << 21);
    do {
      *nowstart = 0x68;
      nowstart += 4096;
    }
    while (nowstart < nowend);

    clock_gettime(CLOCK_MONOTONIC, &(ts4));
    munmap(smallpagestart, SIZE_RAY << 21);
  }
  printf("%x, %7lu,%12lu,%9lu\n", smallpagestart, difftime(ts2,ts1), difftime(ts3,ts2), difftime(ts4,ts3));

  return 0;
}
