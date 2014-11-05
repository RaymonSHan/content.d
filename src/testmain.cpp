
#include "../include/raymoncommon.h"
#include "../include/rmemory.hpp"
#include "../include/rthread.hpp"
#include "../include/epollpool.hpp"
#include "../include/testmain.hpp"

RpollGlobalApp RpollApp;

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
    stack.pVoid = mmap (stack.pChar + PAD_TRACE_INFO, sizeof(threadTraceInfo), 
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  } else {
    printf("Got signal %d, faulty address is %p, from %llx\n Calling: \n",
	   sig, info->si_addr, uc->uc_mcontext.gregs[REG_RIP]);
    displayTraceInfo(tinfo);
    RpollApp.KillAllChild();
    exit(-1);
  }
}

void SetupSIG(int num, SigHandle func)
{
  struct sigaction sa;
 
  sa.sa_sigaction = func;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigaction(num, &sa, NULL);
}

#ifdef  TEST_THREAD


RpollGlobalApp* GetApplication()
{
  return &RpollApp;
}



int main (int, char**)
{
  union SOCKADDR addr;
  char local_addr[] = "127.0.0.1";  
  int retthread, status;
 
  SetupSIG(SIGSEGV, SIGSEGV_Handle);
  SetupSIG(SIGILL, SIGSEGV_Handle);
  SetupSIG(SIGTERM, SIGSEGV_Handle);

  __TRY
    RpollApp.InitRpollGlobalApp();                              // setup memory

    bzero(&addr.saddrin, sizeof(sockaddr_in));   
    addr.saddrin.sin_family = AF_INET; 
    inet_aton(local_addr,&(addr.saddrin.sin_addr));
    addr.saddrin.sin_port=htons(8998);

    RpollApp.StartRpoll(RUN_WITH_CONSOLE, addr.saddr);

    retthread = waitpid(-1, &status, __WCLONE);
    RpollApp.KillAllChild();
  __CATCH
}

#endif  // TEST_THREAD

#ifdef  TEST_RMEMORY

#define THREADS                 2
#define SCHEDULE_THREAD         1
#define NUMBER_BUFFER           20

#ifdef  __DIRECT
#define DIRECT                  1
#else   // __DIRECT
#define DIRECT                  0
#endif  // _DIRECT


CMemoryAlloc MList;

CMemoryAlloc* GetContextList()
{
  return &MList;
}

int main(int, char**)
{
  int   status;
  struct sigaction sa;
  RThreadGet threadGet[THREADS];
  RThreadSchedule threadSchedule;

  sa.sa_sigaction = SIGSEGV_Handle;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);

  __TRY
    __DO(MList.SetMemoryBuffer(NUMBER_BUFFER, 80, 64, DIRECT));
    for (int i=0; i<THREADS; i++) {
      threadGet[i].RThreadClone();
    }
#ifndef __DIRECT
    threadSchedule.RThreadClone();
#endif // __DIRECT
    RThread::RThreadCloneFinish();

    for (int i=0; i<THREADS; i++) waitpid(-1, &status, __WCLONE);
#ifndef __DIRECT
    threadSchedule.RThreadKill();
#endif // __DIRECT

#ifdef  _TESTCOUNT              // for test function, such a multithread!
     MList.DisplayArray();
     MList.DisplayInfo();
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
