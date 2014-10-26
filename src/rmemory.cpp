
#define _TESTCOUNT
#include <unistd.h>
#include "../include/rmemory.hpp"

volatile INT GlobalTime = 0;
static ADDR isNULL = {0};

ADDR getMemory(INT size, INT flag)
{
  static ADDR totalMemoryStart = {SEG_START_STACK};
  ADDR p;

  p.aLong = __sync_fetch_and_add(&(totalMemoryStart.aLong), size);
  p.aLong = ((p.aLong - 1) & (-1*SIZE_NORMAL_PAGE)) + SIZE_NORMAL_PAGE;
  p.pVoid = mmap (p.pVoid, size, PROT_READ | PROT_WRITE,
		  MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED | flag, -1, 0);
  return p;
}

ADDR getStack(void)
{
  return getMemory(SIZE_THREAD_STACK, MAP_GROWSDOWN);
}

CMemoryAlloc::CMemoryAlloc()
{ 
  RealBlock.aLong = 0;
  BorderSize = TotalSize = 0;

  TotalBuffer.aLong = UsedItem.aLong = FreeBufferStart.aLong = FreeBufferEnd.aLong = 0;
  TotalNumber = FreeNumber = 0;

#ifdef _TESTCOUNT
  GetCount = GetSuccessCount = FreeCount = FreeSuccessCount = 0;
  FreeLoop = GetLoop = GetFail = 0;
#endif // _TESTCOUNT

  DirectFree = 1;
}

CMemoryAlloc::~CMemoryAlloc()
{
  DelMemoryBuffer();
}

INT     CMemoryAlloc::SetMemoryBuffer(INT number, INT size, INT border)
{
  ADDR  thisItem, nextItem;
  INT   i;

  __TRY
    __MARK(mmapMemory)
    BorderSize = ((size-1) / border + 1) * border;
    TotalSize = BorderSize * number;
    RealBlock = getMemory(TotalSize, 0);
    __DO(RealBlock.pVoid == MAP_FAILED);

    BufferSize = size - sizeof(CListItem);
    thisItem = nextItem = RealBlock;
    for (i=0; i<number-1; i++) {
      nextItem.aLong += BorderSize;
      thisItem.pList->nextList = nextItem;
      thisItem = nextItem;
    }
    FreeBufferStart = TotalBuffer = RealBlock;
    FreeBufferEnd = nextItem;
    nextItem.pList->nextList.pList = MARK_FREE_END;
    
    TotalNumber = FreeNumber = number;
#ifdef _TESTCOUNT
    MinFree = FreeNumber;
#endif // _TESTCOUNT
  __CATCH
}

/*
  void  SetListDetail(char *listname, INT directFree, INT timeout);
  void  TimeoutContext(void);
*/

INT CMemoryAlloc::DelMemoryBuffer(void)
{
  return 0;
}

void CMemoryAlloc::SetListDetail(char *listname, INT directFree, INT timeout)
{
}
											//
void CMemoryAlloc::DisplayContext(void)
{
  ADDR mList = UsedItem;
  INT num = TotalNumber + 3;

  while (mList.aLong && num) {
    printf("0x%p:%lld->", mList.pList, mList.pList->countDown);
    mList = mList.pList->usedList;
    num--;
  }
  if (UsedItem.aLong) printf("\n");
  if (!num) printf("ERROR\n");
}

INT CMemoryAlloc::GetMemoryList(ADDR &nlist)
{
  __TRY
    __DO(GetOneList(nlist))
    nlist.pList->nextList.pList = MARK_USED;
    if (!DirectFree) AddToUsed(nlist);
  __CATCH
}

INT CMemoryAlloc::FreeMemoryList(ADDR nlist)
{
  __TRY
    if (!DirectFree) nlist.pList->countDown = 2;
    else __DO(FreeOneList(nlist))
  __CATCH
}

CMemoryListCriticalSection::CMemoryListCriticalSection()
{
  InProcess = usedProcess = NOT_IN_PROCESS;
  pInProcess = &InProcess;
  pusedProcess = &usedProcess;
}

INT CMemoryListCriticalSection::GetOneList(ADDR &nlist)
{
  __LOCKp(pInProcess)
#ifdef _TESTCOUNT
    GetCount++;
#endif // _TESTCOUNT
  nlist = FreeBufferStart;                                      // Get the first free item

  if (nlist.pList->nextList > MARK_MAX_INT) {                   // Have free
    FreeBufferStart=nlist.pList->nextList;
    FreeNumber--;
#ifdef _TESTCOUNT
    if (FreeNumber<MinFree) MinFree = FreeNumber;
    GetSuccessCount++;
#endif // _TESTCOUNT
  }
  else nlist.pList = NULL;

  __FREEp(pInProcess)
  return (nlist.pList == NULL);
}

/* this add to tail
INT CMemoryListCriticalSection::FreeOneList(ADDR nlist)
{
  __TRY
    __LOCKp(pInProcess)

#ifdef _TESTCOUNT
    FreeCount++;
#endif // _TESTCOUNT
    __DO_ ( (nlist < MARK_MAX_INT || nlist.pList->nextList.pList != MARK_USED),
	    "FreeList Twice %p\n", nlist.pList)
    nlist.pList->nextList.pList = MARK_FREE_END;
    FreeBufferEnd.pList->nextList = nlist;
    FreeBufferEnd = nlist;
#ifdef _TESTCOUNT
    FreeSuccessCount++;
#endif // _TESTCOUNT
    FreeNumber++;

    __FREEp(pInProcess)
  __CATCH_BEGIN
    __FREEp(pInProcess)
 __CATCH_END
}
*/

/* this add to head*/
INT CMemoryListCriticalSection::FreeOneList(ADDR nlist)
{
  __TRY
    __LOCKp(pInProcess)

#ifdef _TESTCOUNT
    FreeCount++;
#endif // _TESTCOUNT

    __DO_ ( (nlist < MARK_MAX_INT || nlist.pList->nextList.pList != MARK_USED),
	    "FreeList Twice %p\n", nlist.pList);
    nlist.pList->nextList = FreeBufferStart;
    FreeBufferStart = nlist;

#ifdef _TESTCOUNT
    FreeSuccessCount++;
#endif // _TESTCOUNT
    FreeNumber++;

    __FREEp(pInProcess)
  __CATCH_BEGIN
    __FREEp(pInProcess)
 __CATCH_END
}

INT CMemoryListCriticalSection::AddToUsed(ADDR nlist)
{
  __LOCKp(pusedProcess)
  nlist.pList->usedList = UsedItem;
  UsedItem = nlist;
  __FREEp(pusedProcess)
  nlist.pList->countDown = TimeoutInit;
  return 0;
}

INT CMemoryListLockFree::GetOneList(ADDR &nlist)
{
  ADDR  next;
  volatile INT* start = &FreeBufferStart.aLong;
  BOOL changed;
#ifdef _TESTCOUNT
  __sync_fetch_and_add(&GetCount, 1);
  __sync_fetch_and_add(&GetSuccessCount, 1);
#endif // _TESTCOUNT

  do {
    changed = 0;
    do {
      nlist.aLong = *start;
      if (nlist.aLong > 0)
	changed = __sync_bool_compare_and_swap(start, nlist.aLong, 0);
    } while (!changed);

    changed = 0;
    next = nlist.pList->nextList;
    if (next > MARK_MAX_INT) {                   // Have free
      changed = __sync_bool_compare_and_swap(start, 0, next.aLong);
    }
    else {
      nlist.pList = NULL;
#ifdef _TESTCOUNT
      __sync_fetch_and_add(&GetSuccessCount, -1);
#endif // _TESTCOUNT
      break;
    }
  } while (!changed);
  if (nlist.pList != NULL) __sync_fetch_and_add(&FreeNumber, -1);

  return (nlist.pList == NULL);
}

INT CMemoryListLockFree::FreeOneList(ADDR nlist)
{
  ADDR  next;
  BOOL changed;
  volatile INT* start = &FreeBufferStart.aLong;
#ifdef _TESTCOUNT
  __sync_fetch_and_add(&FreeCount, 1);
  __sync_fetch_and_add(&FreeSuccessCount, 1);
#endif // _TESTCOUNT

  next = nlist;  //
  do {
     changed = 0;
    do {
      next.aLong = *start;
      if (next.aLong > 0)
	changed = __sync_bool_compare_and_swap(start, next.aLong, 0);
    } while (!changed);

    changed = 0;

    nlist.pList->nextList = next;
    changed = __sync_bool_compare_and_swap(start, 0, nlist.aLong);
  }
  while (!changed);
  __sync_fetch_and_add(&FreeNumber, 1);
  return 0;
}

INT CMemoryListLockFree::AddToUsed(ADDR )//nlist)
{
  return 0;
}


#include <sched.h>
#include <sys/wait.h>
#define THREADS 2

int main(int, char**)
{
  ADDR cStack;
  CMemoryListCriticalSection m;
  //    CMemoryListLockFree m;
  int status;
 
  m.SetMemoryBuffer(10, 80, 64);

  for (int i=0; i<THREADS; i++) {
    cStack = getStack();
    clone (&ThreadItem, cStack.pChar + SIZE_THREAD_STACK, CLONE_VM | CLONE_FILES, &m);
  }
  for (int i=0; i<THREADS; i++) waitpid(-1, &status, __WCLONE);

  m.DisplayInfo();
  return 0;
}

#ifdef _TESTCOUNT


void CMemoryAlloc::DisplayInfo(void)
{
  printf("Total:%10lld, Free:%10lld\n", TotalNumber, FreeNumber);
  printf("Get  :%10lld, Succ:%10lld\n", GetCount, GetSuccessCount);
  printf("Free :%10lld, Succ:%10lld\n", FreeCount, FreeSuccessCount);
  printf("GLoop:%10lld, FLoop:%9lld, GetFail:%10lld\n", GetLoop, FreeLoop, GetFail);

  printf("MinFree:%8lld,\n", MinFree);

  volatile ADDR item;
  printf("buffer start, end: %llx, %llx\n", FreeBufferStart.aLong, FreeBufferEnd.aLong); 
    item.pList = RealBlock.pList;
    for (int i=0; i<TotalNumber; i++) {
      printf("%5d:%p, %p, %p,\n", i, item.pList, item.pList->nextList.pVoid, item.pList->usedList.pVoid);
      item.aLong += BorderSize;
  }
}

#define TEST_TIMES 10000000
#define TEST_ITMES 3

int ThreadItem(void *para)
{
  __TRY
  ADDR item[TEST_ITMES+2];
  CMemoryAlloc* cmem = (CMemoryAlloc*) para;
  for (int i=0; i<TEST_TIMES; i++) {
    for (int j=0; j<TEST_ITMES; j++) 
      cmem->GetMemoryList(item[j]);
    for (int j=0; j<TEST_ITMES; j++) 
      if (item[j].aLong != 0) cmem->FreeMemoryList(item[j]);
  }
  __CATCH
}

#endif // _TESTCOUNT



/*
60M times 9.2 2thread1cpu lockfree, 11.2 2thread2cpu lockfree, 3.4*2 1thread lockfree
60M times 5.9 2thread2cpu semi, 1.0*2 1thread semi
60M times 6.3 2thread2cpu semi add head, 1.1*2 1thread semiadd head
 */
