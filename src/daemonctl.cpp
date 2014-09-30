
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

//#define HUGE_PAGE_USED

// for x64 page table is 9-9-9-9-12 level, for huge page with size of 2M,
// L3 page table contain 1G, L2 for 512G, L1 for total 256T = 2^40.
// In my huge segment, which size if 512G, with boundary 0x0000 ??80 0000 0000
// should use 512+1 page table which size about 2M
// But it seems HUGETLB not support MADV_DONTNEED, 
// I try MADV_DONTNEED by normal page first
#define HUGE_SEGMENT_START (0x52LL << 40)  // 'R'
#define HUGE_PAGE_SIZE     (1LL << 21)

// for x64 page table is 9-9-9-9-12 level, for normal page with size of 4k
// test for DONTNEED behavior
union assignAddress{
  long long int addressLong;
  void *addressVoid;
  long long int *addressLPointer;
};
#define NORMAL_SEGMENT_START (0x52LL << 40)  // 'R'
#define NORMAL_PAGE_SIZE     (1LL << 12)

#define WRITE_SELF(add) *add.addressLPointer = add.addressLong
#define VALID_SELF(add) *add.addressLPointer == add.addressLong
#define NEXT_HUGE_PAGE(add)  add.addressLong += HUGE_PAGE_SIZE
#define NEXT_NORMAL_PAGE(add) add.addressLong += NORMAL_PAGE_SIZE
#define FREE_HUGE_PAGE(add, num) madvise(add.addressVoid, num<<21, MADV_DONTNEED)
#define FREE_NORMAL_PAGE(add, num) madvise(add.addressVoid, num<<12, MADV_DONTNEED)

#ifdef HUGE_PAGE_USED
#define SEGMENT_START HUGE_SEGMENT_START
#define PAGE_SIZE HUGE_PAGE_SIZE
#define NEXT_PAGE NEXT_HUGE_PAGE
#define FREE_PAGE FREE_HUGE_PAGE
#define MMFLAG    MAP_HUGETLB | MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE
#else //HUGE_PAGE_USED
#define SEGMENT_START NORMAL_SEGMENT_START
#define PAGE_SIZE NORMAL_PAGE_SIZE
#define NEXT_PAGE NEXT_NORMAL_PAGE
#define FREE_PAGE FREE_NORMAL_PAGE
#define MMFLAG    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE
#endif //HUGE_PAGE_USED

union assignAddress assignStart, realStart, usedp;
timespec ts1, ts2, ts3, ts4, ts5;

unsigned long long difftime (timespec &t2, timespec &t1) {
  return ((t2.tv_sec-t1.tv_sec)*1000000000 + (t2.tv_nsec-t1.tv_nsec));
}

void map128M(union assignAddress add) {
  add.addressVoid = mmap (add.addressVoid, 128*1024*1024, PROT_READ | PROT_WRITE, 
			  MMFLAG | MAP_FIXED, -1, 0);
  for (unsigned int i=0; i<(128*1024*1024/sizeof(long long)); i++) {
    WRITE_SELF(add);
    add.addressLong += sizeof(long long);
  }
}

void unmap128M(union assignAddress add) {
  munmap(add.addressVoid, 128*1024*1024);
}

// time ./content form mmap & munmap
// real 7.8s, user 0.2s, sys 7.6s for 400*128M alloc and page fault 
// it same as MADV_DONTNEED
// avg for 4k page is 595ns
int mapunmap() {
  usedp.addressLong = SEGMENT_START;
  for (int i=0; i<400; i++) {
    //    printf("128 Loop %d, addr:%llx\n", i, usedp.addressLong);
    map128M(usedp);
    unmap128M(usedp);
    usedp.addressLong += 128*1024*1024;
  }
  return 0;
}

void get128M(union assignAddress add) {
  for (unsigned int i=0; i<(128*1024*1024/sizeof(long long)); i++) {
    //    if (VALID_SELF(add)) printf("IS VALID %llx\n", add.addressLong);
    WRITE_SELF(add);
    add.addressLong += sizeof(long long);
    //    NEXT_PAGE(add);
  }
}

// madvise for HUGEPAGE will fail
void back128M(union assignAddress add) {
  FREE_PAGE(add, 128*1024*1024/PAGE_SIZE);
}

// time ./content for MADV_DONTNEED
// real 7.8s, user 0.2s, sys 7.6s for 400*128M alloc and page fault
// avg for 4k page is 595ns
int madviseDontNeed () {
  assignStart.addressLong = SEGMENT_START;
  long long int memsize = 1LL << 39;
  //  printf("memsize: 0x%llx\n", memsize);

  realStart.addressVoid = (mmap (assignStart.addressVoid, memsize, PROT_READ | PROT_WRITE, 
				 MMFLAG, -1, 0));
  //  printf("after mmap: error:%d,realStart:0x%llx\n", errno, realStart.addressLong);
  usedp = realStart;

  for (int i=0; i<400; i++) {
    //    printf("128 Loop %d, addr:%llx\n", i, usedp.addressLong);
    get128M(usedp);
    if (i>2) back128M(usedp);
    usedp.addressLong += 128*1024*1024;
  }

  usedp = realStart;
  for (int i=0; i<400; i++) {
    //    printf("Valid view Loop %d, addr:%llx, val:%llx\n", i, usedp.addressLong, *usedp.addressLPointer);
    usedp.addressLong += 128*1024*1024;
  }

  munmap(realStart.addressVoid, memsize);
  //  printf("after munmap: error:%d,realStart:0x%llx\n", errno, realStart.addressLong);

  return 0;
}

#define SIZE_RAY 10

int hugePageAlloc () {
  char *pagestart1;
  char *nowstart, *nowend;

  clock_gettime(CLOCK_MONOTONIC, &(ts1));
  pagestart1 = static_cast<char*>
    (mmap (0, SIZE_RAY << 21, PROT_READ | PROT_WRITE,
	   MAP_HUGETLB | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  perror("after huge mmap 10");

  clock_gettime(CLOCK_MONOTONIC, &(ts2));
  nowstart = pagestart1;
  nowend = nowstart + (SIZE_RAY << 21) - 1;
  do {
    *nowstart = 0x67;
    nowstart ++;
  }
  while (nowstart < nowend);
  perror("after write huge first");

  clock_gettime(CLOCK_MONOTONIC, &(ts3));
  nowstart = pagestart1 + 1;
  nowend = nowstart + (SIZE_RAY << 21) - 1;
  do {
    *nowstart = 0x67;
    nowstart ++;
  }
  while (nowstart < nowend);
  perror("after write huge second");

  clock_gettime(CLOCK_MONOTONIC, &(ts4));
  nowstart = pagestart1 + 1;
  nowend = nowstart + (SIZE_RAY << 21) - 1;
  do {
    *nowstart = 0x67;
    nowstart ++;
  }
  while (nowstart < nowend);
  perror("after write huge third");

  clock_gettime(CLOCK_MONOTONIC, &(ts5));
  munmap(pagestart1, SIZE_RAY << 21);
  perror("after unmap huge");

  printf("%llu,%llu,%llu,%llu\n", 
	 difftime(ts2, ts1), difftime(ts3, ts2), difftime(ts4, ts3), difftime(ts5, ts4));
  return 0;
}

int main(int , char **) {
  //  mapunmap();
  madviseDontNeed();
  //  hugePageAlloc();
}
