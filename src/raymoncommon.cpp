
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "../include/raymoncommon.h"

void perrorexit(const char* s)
{
  perror(s);
  exit(-1);
}

char* getStack(void)
{
  static union assignAddress totalStackStart = {STACK_START};

  union assignAddress nowstack, realstack;
  nowstack.addressLong = __sync_fetch_and_add(&(totalStackStart.addressLong), STACK_SIZE);
  realstack.addressVoid = mmap (nowstack.addressVoid, STACK_SIZE, PROT_READ | PROT_WRITE, 
				MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED | MAP_GROWSDOWN, -1, 0);
  if (realstack.addressVoid == MAP_FAILED ) {
    printf("error in mmap%llx, %llx\n", nowstack.addressLong, realstack.addressLong);
    perrorexit("adsf");
    return 0;
  }
  return realstack.addressChar;
}
