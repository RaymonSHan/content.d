#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../include/raymoncommon.h"
#include "../include/rmemory.hpp"
#include "../include/rthread.hpp"
#include "../include/epollpool.hpp"
#include "../include/testmain.hpp"


#ifdef  TEST_RMEMORY
#define __DIRECT

#define THREADS                 2
#define SCHEDULE_THREAD         1
#define TEST_TIMES              5000000
#define TEST_ITMES              3
#define NUMBER_BUFFER           20
#define GETSIZE                 4
#define FREESIZE                4
#define MAXSIZE                 8
#ifdef  __DIRECT
#define DIRECT                  1
#else   // __DIRECT
#define DIRECT                  0
#endif  // _DIRECT

void SIGSEGV_Handle(int sig, siginfo_t *info, void *secret)
{
  ADDR  stack, erroraddr;
  ucontext_t *uc = (ucontext_t *)secret;
  threadTraceInfo *tinfo;

  stack.pAddr = &stack;
  stack &= NEG_SIZE_THREAD_STACK;
  erroraddr.pVoid = info->si_addr;
  erroraddr &= NEG_SIZE_THREAD_STACK;

  if (stack == erroraddr) {
    stack.pVoid = mmap (stack.pChar + SIZE_NORMAL_PAGE, SIZE_NORMAL_PAGE, PROT_READ | PROT_WRITE,
  			  MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  } else {
    printf("Got signal %d, faulty address is %p, from %llx\n Calling: \n",
	   sig, info->si_addr, uc->uc_mcontext.gregs[REG_RIP]);
    displayTraceInfo(tinfo);
    exit(-1);
  }
}

int ThreadSchedule(void *para)
{
  CMemoryAlloc *clist = (CMemoryAlloc*) para;

  clist->SetThreadArea(0, GETSIZE, GETSIZE, THREAD_FLAG_SCHEDULE);
  for (;;) {
    clist->TimeoutAll();
#ifdef  _TESTCOUNT
    clist->DisplayContext();
#endif  // _TESTCOUNT
    usleep(250000);                                           // every 0.25s
  }
  return 0;
}

int ThreadItem(void *para)
{
  __TRY
    ADDR item[TEST_ITMES+2];
    CMemoryAlloc *clist = (CMemoryAlloc*) para;

    clist->SetThreadArea(GETSIZE, MAXSIZE, FREESIZE, THREAD_FLAG_GETMEMORY);

#ifndef __DIRECT
  // for undirect free
    for (int j=0; j<TEST_ITMES; j++) 
      if (clist->GetMemoryList(item[j])) item[j]=0;
        sleep(3);
    for (int j=0; j<1; j++) 
      if (clist->GetMemoryList(item[j])) item[j]=0;
        sleep(1);
#else  // __DIRECT
  // for direct free
    for (int i=0; i<TEST_TIMES; i++) {
      for (int j=0; j<TEST_ITMES; j++) 
	if (clist->GetMemoryList(item[j])) item[j]=0;
      for (int j=0; j<TEST_ITMES; j++) 
        if (item[j].aLong != 0) clist->FreeMemoryList(item[j]);
      }
#endif // __DIRECT
  __CATCH
}

int main(int, char**)
{
  ADDR  cStack;
  CMemoryAlloc m;
  int   status;
  struct sigaction sa;

  sa.sa_sigaction = SIGSEGV_Handle;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);

  __TRY
    __DO(m.SetMemoryBuffer(NUMBER_BUFFER, 80, 64, DIRECT));
    for (int i=0; i<THREADS; i++) {
      getStack(cStack);
      clone (&ThreadItem, cStack.pChar + SIZE_THREAD_STACK, CLONE_VM | CLONE_FILES, &m);
    }

#ifndef __DIRECT
    int sch;
    getStack(cStack);
    sch = clone (&ThreadSchedule, cStack.pChar + SIZE_THREAD_STACK, CLONE_VM | CLONE_FILES, &m);
#endif // __DIRECT
    for (int i=0; i<THREADS; i++) waitpid(-1, &status, __WCLONE);

#ifndef __DIRECT
    kill(sch, SIGTERM);
#endif // __DIRECT

#ifdef  _TESTCOUNT              // for test function, such a multithread!
     m.DisplayArray();
     m.DisplayInfo();
#endif  // _TESTCOUNT
  __CATCH
}

/*
60M times 9.2 2thread1cpu lockfree, 11.2 2thread2cpu lockfree, 3.4*2 1thread lockfree
60M times 5.9 2thread2cpu semi, 1.0*2 1thread semi
60M times 6.3 2thread2cpu semi add head, 1.1*2 1thread semiadd head
60M times 4.0 2thread2cpu localarray(4 local), 1.1 2thread2cpu localarray without add count
this is the best way i have found, about 30ns per Get/Free for one cpu

test for 4 thread in 2cpu, every thread get 15 and free 15 for 5M times
total for 300M times get and free. second line is second
the size in table means max number, it will free only reach max.
16 means all local, 14-8 are almost same
|  16 |  14 |  12 |  10 |    8 |    6 |    4 |    2 |
| 4.2 | 9.1 | 8.8 | 8.7 | 11.4 | 17.1 | 21.6 | 40.8 |
the fast is all local, only 14ns for one GET/FREE. 
 */
#endif // TEST_RMEMORY
