// rthread.cpp

#include "../include/rthread.hpp"
#include "../include/testmain.hpp"

// first page for pad
// second page for callnest info, which i fix the place
volatile INT RThreadResource::globalResourceOffset = PAD_TRACE_INFO + sizeof(threadTraceInfo);

RThreadResource::RThreadResource(INT size)
{
  SetResourceOffset(size);
}

INT RThreadResource::SetResourceOffset(INT size)
{
  nowOffset = LockAdd(RThreadResource::globalResourceOffset, PAD_INT(size, 0, 64));
  //  printf("in offset %llx\n", nowOffset);
  return nowOffset;
}

const static ADDR CONSTADDR = {1};

INT REvent::EventInit()
{
  __TRY
    handleLock = NOT_IN_PROCESS;
    __DO1_(eventFd,
	   eventfd(0, EFD_SEMAPHORE),
	   "Error in create eventfd");
  __CATCH
}

INT REvent::EventWrite(ADDR addr)
{
  int status;
  __TRY
    __DO  (!CmpExg(&handleLock.aLong, NOT_IN_PROCESS, addr.aLong))
    __DO1_(status,
	   write(eventFd, &CONSTADDR, sizeof(ADDR)),
	   "Error in write eventfd function");
  __CATCH
}

INT REvent::EventRead(ADDR &addr)
{
  int status;
  __TRY
    __DO1(status,
	  read(eventFd, &addr, sizeof(ADDR)));
    addr = handleLock;
    handleLock = NOT_IN_PROCESS;
  __CATCH
}

INT RMultiEvent::EventInit(INT num, isThis func)
{
  __TRY
    __DO(num > MAX_HANDLE_LOCK)
    handleLock = NOT_IN_PROCESS;
    handleStart = 0;
    handleEnd = num;        // there means empty
    handleNumber = num + 1;
    nextEvent = 0;
    isThisFunc = func;
    __DO1(eventFd,
	  eventfd(0, EFD_SEMAPHORE));
  __CATCH
}

INT RMultiEvent::EventWrite(ADDR addr)
{
  int status;
  __TRY
    __DO (handleStart == handleEnd);
    __LOCK(handleLock);
    handleBuffer[handleStart] = addr;
    if (++handleStart == handleNumber) handleStart = 0;
    __FREE(handleLock);
    __DO1(status,
	  write(eventFd, &CONSTADDR, sizeof(ADDR)));
  __CATCH
}

INT RMultiEvent::EventRead(ADDR &addr)
{
  int status;
  __TRY
    __DO1(status,
	  read(eventFd, &addr, sizeof(ADDR)));
    __LOCK(handleLock);
    if (++handleEnd == handleNumber) handleEnd = 0;
    addr = handleBuffer[handleEnd];
    __FREE(handleLock);
  __CATCH
}

#include <typeinfo>

volatile INT RThread::globalThreadNumber = 1;
volatile INT RThread::nowThreadNumber = 0;

RThread::RThread()
{
  workId = 0;
  stackStart = 0;
  shouldQuit = 0;
  nowThread = 0;
}

INT RThread::RThreadClone(void)
{
  __TRY
    nowThread = LockInc(RThread::globalThreadNumber) - 1;
    __DO  (getStack(stackStart));
    __DO1_(workId, 
	   clone(&(RThread::RThreadFunc), stackStart.pChar + SIZE_THREAD_STACK, 
		 CLONE_VM | CLONE_FILES, this), 
	   "Error for clone");
  __CATCH
}

INT RThread::RThreadKill(void)
{
  __TRY
    kill(workId, SIGTERM);
  __CATCH
}

int RThread::RThreadFunc(void* point)
{
  RThread* thread = (RThread*) point;

  __TRY
    RWAIT(RThread::nowThreadNumber, thread->nowThread);
    __DO(thread->RThreadInit());
    LockInc(RThread::nowThreadNumber);
    RWAIT(RThread::nowThreadNumber, RThread::globalThreadNumber);
    while (!thread->shouldQuit) {                               // Do NOT add __DO 
      thread->RThreadDoing();                                   // continuous even error
    }
  __CATCH
}

#ifdef TEST_MEMORY

#define GETSIZE                 4
#define FREESIZE                4
#define MAXSIZE                 8
#define TEST_ITMES              3
#define TEST_TIMES              500000

INT RThreadGet::RThreadInit(void)
{
  __TRY
    mList = ::GetContextList();
    mList->SetThreadArea(GETSIZE, MAXSIZE, FREESIZE, THREAD_FLAG_GETMEMORY);
  __CATCH
}

INT RThreadGet::RThreadDoing(void)
{    
  ADDR item[TEST_ITMES+2];

  __TRY
#ifndef __DIRECT
  // for undirect free
    for (int j=0; j<TEST_ITMES; j++) 
      if (mList->GetMemoryList(item[j])) item[j]=0;
        sleep(3);
    for (int j=0; j<1; j++) 
      if (mList->GetMemoryList(item[j])) item[j]=0;
        sleep(1);
#else  // __DIRECT
  // for direct free
    for (int i=0; i<TEST_TIMES; i++) {
      for (int j=0; j<TEST_ITMES; j++) 
	if (mList->GetMemoryList(item[j])) item[j]=0;
      for (int j=0; j<TEST_ITMES; j++) 
        if (item[j].aLong != 0) mList->FreeMemoryList(item[j]);
      }
#endif // __DIRECT
    shouldQuit = true;
  __CATCH
}

INT RThreadSchedule::RThreadInit(void)
{
  __TRY
    mList = ::GetContextList();
    mList->SetThreadArea(0, GETSIZE, GETSIZE, THREAD_FLAG_SCHEDULE);
  __CATCH
}

INT RThreadSchedule::RThreadDoing(void)
{
  __TRY
    mList->TimeoutAll();
#ifdef  _TESTCOUNT
    mList->DisplayContext();
#endif  // _TESTCOUNT
    usleep(250000);
  __CATCH
}

#endif  // TEST_MEMORY
