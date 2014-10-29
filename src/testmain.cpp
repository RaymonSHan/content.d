#include <sched.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../include/raymoncommon.h"
#include "../include/rmemory.hpp"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// This is rmemory test main program from. From Oct. 22 - 29 2014.                                 //
// total program time about 50 hour, base on memoryiocp                                            //
// request : rmemory.cpp                                                                           //
#define TEST_RMEMORY                                                                             //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //

#ifdef  TEST_RMEMORY

#define THREADS                 2
#define SCHEDULE_THREAD         1
#define TEST_TIMES              500000
#define TEST_ITMES              3
#define NUMBER_BUFFER           30

int ThreadSchedule(void *para)
{
  CMemoryListArray *clist = (CMemoryListArray*) para;

  __TRY
    __DO_(clist->SetThreadArea(THREAD_FLAG_SCHEDULE), "Too many thread than setting");
    for (;;) {
      clist->TimeoutAll();
#ifdef  _TESTCOUNT
      clist->DisplayContext();
#endif  // _TESTCOUNT
      usleep(250000);                                           // every 0.25s
    }
  __CATCH
}

int ThreadItem(void *para)
{
  ADDR item[TEST_ITMES+2];
  CMemoryAlloc *cmem = (CMemoryAlloc*) para;
  CMemoryListArray *clist = (CMemoryListArray*) para;

  __TRY
    __DO_(clist->SetThreadArea(THREAD_FLAG_GETMEMORY), "Too many thread than setting");

    for (int i=0; i<TEST_TIMES; i++) {
      for (int j=0; j<TEST_ITMES; j++) 
	if (cmem->GetMemoryList(item[j])) item[j]=0;
      for (int j=0; j<TEST_ITMES; j++) 
	if (item[j].aLong != 0) cmem->FreeMemoryList(item[j]);
    }
  __CATCH
}

int main(int, char**)
{
  ADDR cStack;
  CMemoryListArray m;
  //CMemoryListCriticalSection m;
  //CMemoryListLockFree m;
  int status;
 
  m.SetMemoryBuffer(NUMBER_BUFFER, 80, 64);
  m.SetThreadLocalArray(THREADS + SCHEDULE_THREAD, 4, 2);                       // add for schedule use

  for (int i=0; i<THREADS; i++) {
    cStack = getStack();
    clone (&ThreadItem, cStack.pChar + SIZE_THREAD_STACK, CLONE_VM | CLONE_FILES, &m);
  }
  cStack = getStack();
  int sch = clone (&ThreadSchedule, cStack.pChar + SIZE_THREAD_STACK, CLONE_VM | CLONE_FILES, &m);
  for (int i=0; i<THREADS; i++) waitpid(-1, &status, __WCLONE);
  kill(sch, SIGTERM);

#ifdef  _TESTCOUNT              // for test function, such a multithread!
  m.DisplayArray();
  m.DisplayInfo();
#endif  // _TESTCOUNT
 return 0;
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
