
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
//#include "../include/daemonctl.h"

#define SIZE_RAY 64

int
main (int , char **) {
  struct timespec  ts1, ts2, ts3;
  char* smallpagestart;
  char* nowstart, *nowend;


  clock_gettime(CLOCK_MONOTONIC, &(ts1));

  smallpagestart = static_cast<char*>
    (mmap (0, SIZE_RAY << 22, PROT_READ | PROT_WRITE,
	   /*MAP_HUGETLB |*/ MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  nowstart = smallpagestart;
  nowend = nowstart + (SIZE_RAY << 22);
  do {
    *nowstart = 0x67;
    nowstart += 4096;
  }
  while (nowstart < nowend);
  clock_gettime(CLOCK_MONOTONIC, &(ts2));

  nowstart = smallpagestart + 1;
  nowend = nowstart + (SIZE_RAY << 22);
  do {
    *nowstart = 0x68;
    nowstart += 4096;
  }
  while (nowstart < nowend);

  clock_gettime(CLOCK_MONOTONIC, &(ts3));

  // printf("%lu,%lu,%lu\n", ts1.tv_nsec, ts2.tv_nsec, ts3.tv_nsec);
  printf("%lu,%lu\n", ts2.tv_nsec-ts1.tv_nsec, ts3.tv_nsec-ts2.tv_nsec);

  return 0;
}
