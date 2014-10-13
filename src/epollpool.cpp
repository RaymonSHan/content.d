#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>                                                                                          
#include <sched.h>
#include <signal.h>
#include <errno.h>  
#include <fcntl.h>                                                                                           
#include <arpa/inet.h>                                                                                       
#include <netinet/in.h>                                                                                      
#include <sys/socket.h>                                                                                      
#include <sys/epoll.h>
#include <sys/mman.h>

#define MAX_RPOLL_ACCEPT_THREAD         3
#define MAX_RPOLL_READ_THREAD           4
#define MAX_RPOLL_WRITE_THREAD          2
#define MAX_WORK_THREAD                 16

#define RPOLL_INIT_NULL                 0
#define RPOLL_INIT_START                1
#define RPOLL_INIT_OK                   2

#define ENOUGH_TIME_FOR_LISTEN 10000

typedef struct workThreadStruct
{
  int workSignal;
  pid_t workId;
}workThreadStruct;

typedef struct rpollGlobal{
  int rpollInitSignal;
  int rpollNowAccept;
  int rpollNowRead;
  int rpollNowWrite;
  int workNow;

  pid_t rpollScheduleId;
  pid_t rpollAcceptId[MAX_RPOLL_ACCEPT_THREAD];
  pid_t rpollReadId[MAX_RPOLL_READ_THREAD];
  pid_t rpollWriteId[MAX_RPOLL_WRITE_THREAD];
  workThreadStruct workThread[MAX_WORK_THREAD];

  int rpollAcceptfd;
  int rpollReadfd[MAX_RPOLL_READ_THREAD];
  int rpollWritefd[MAX_RPOLL_WRITE_THREAD];
  int rpollReadSchedulefd[MAX_RPOLL_READ_THREAD];
  int rpollWriteSchedulefd[MAX_RPOLL_WRITE_THREAD];

}rpollGlobal;

struct rpollGlobal RPollInfo;

#define STACK_START (0x53LL << 40)           // 'S'
#define STACK_SIZE  (16*1024*1024)

union assignAddress
{
  long long int addressLong;
  char *addressChar;
  void *addressVoid;
  long long int *addressLPointer;
};

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

int rpollAcceptThread(void *r)
{
  struct rpollGlobal *rGlobal = (rpollGlobal*)r;
  int rpollInit;

  rpollInit = 
    __sync_val_compare_and_swap(&(rGlobal->rpollInitSignal), RPOLL_INIT_NULL, RPOLL_INIT_START);
  if (rpollInit == RPOLL_INIT_NULL) {
    printf("in first thread, should do listen\n");
    rGlobal->rpollAcceptfd = 0xaabbccdd;
    rGlobal->rpollInitSignal = RPOLL_INIT_OK;
  }
  else {
    while (rGlobal->rpollInitSignal != RPOLL_INIT_OK) usleep(ENOUGH_TIME_FOR_LISTEN);
    printf("in second thread, listen is ok\n");
  }
  return 0;
}

int rpollReadThread(void *r)
{
  return 0;
}

int rpollWriteThread(void *r)
{
  return 0;
}

int WorkThread(void *r)
{
  return 0;
}

int main (int, char**)
{
  //  int status;
  //  pid_t retthread;
  int i;

  printf("epoll start\n");
  for (i=0; i<MAX_RPOLL_ACCEPT_THREAD; i++) {
    RPollInfo.rpollAcceptId[i] = 
      clone(&rpollAcceptThread, getStack()+STACK_SIZE, CLONE_VM | CLONE_FILES, &RPollInfo);
    if (!RPollInfo.rpollInitSignal) usleep(ENOUGH_TIME_FOR_LISTEN);
  }
  RPollInfo.rpollNowAccept = MAX_RPOLL_ACCEPT_THREAD;

  //__sync_val_compare_and_swap(target, old_value, new_value);
  sleep(2);
  return (EXIT_SUCCESS);
}
