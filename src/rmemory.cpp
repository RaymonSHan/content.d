
#define _TESTCOUNT
#include <unistd.h>
#include "../include/rmemory.hpp"

volatile INT GlobalTime = 0;
static ADDR isNULL = {0};

#undef  HUGE_PAGE

#ifdef  HUGE_PAGE
#define SIZE_PAGE               SIZE_HUGE_PAGE
#else   // HUGE_PAGE
#define SIZE_PAGE               SIZE_NORMAL_PAGE
#endif  // HUGE_PAGE

ADDR getMemory(INT size, INT flag)
{
  static ADDR totalMemoryStart = {SEG_START_STACK};
  ADDR p;

  PAD_INT(size, 0, SIZE_PAGE);
  p = __sync_fetch_and_add(&(totalMemoryStart.aLong), size);
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
  RealBlock = 0;
  BorderSize = TotalSize = 0;

  TotalBuffer = UsedItem = FreeBufferStart = FreeBufferEnd = 0;
  TotalNumber = FreeNumber = 0;

  InProcess = usedProcess = NOT_IN_PROCESS;
  pInProcess = &InProcess;
  pusedProcess = &usedProcess;

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
    __DO(RealBlock == MAP_FAIL);

    BufferSize = size - sizeof(CListItem);
    thisItem = nextItem = RealBlock;
    for (i=0; i<number-1; i++) {
      nextItem += BorderSize;
      thisItem.NextList = nextItem;
      thisItem = nextItem;
    }
    FreeBufferStart = TotalBuffer = RealBlock;
    FreeBufferEnd = nextItem;
    nextItem.NextList = MARK_FREE_END_;
    
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
    printf("0x%p:%lld->", mList.pList, mList.CountDown);
    mList = mList.UsedList;
    num--;
  }
  if (UsedItem.aLong) printf("\n");
  if (!num) printf("ERROR\n");
}

INT CMemoryAlloc::GetMemoryList(ADDR &nlist)
{
  __TRY
    __DO(GetOneList(nlist))
    nlist.NextList = MARK_USED_;
    if (!DirectFree) AddToUsed(nlist);
  __CATCH
}

INT CMemoryAlloc::FreeMemoryList(ADDR nlist)
{
  __TRY
    if (!DirectFree) nlist.CountDown = 2;
    else __DO(FreeOneList(nlist))
  __CATCH
}

INT CMemoryListCriticalSection::GetOneList(ADDR &nlist)
{
  __LOCKp(pInProcess)
#ifdef _TESTCOUNT
    GetCount++;
#endif // _TESTCOUNT
  nlist = FreeBufferStart;                                      // Get the first free item

  if (nlist.NextList > MARK_MAX_INT) {                   // Have free
    FreeBufferStart = nlist.NextList;
    FreeNumber--;
#ifdef _TESTCOUNT
    if (FreeNumber < MinFree) MinFree = FreeNumber;
    GetSuccessCount++;
#endif // _TESTCOUNT
  }
  else nlist = NUL;

  __FREEp(pInProcess)
  return (nlist == NUL);
}

INT CMemoryListCriticalSection::FreeOneList(ADDR nlist)
{
  __TRY
    __LOCKp(pInProcess)

#ifdef _TESTCOUNT
    FreeCount++;
#endif // _TESTCOUNT
    __DO_ ( (nlist < MARK_MAX_INT || nlist.NextList != MARK_USED_),
	    "FreeList Twice %p\n", nlist.pList);
    //FreeListToTail(nlist);
    // or
    FreeListToHead(nlist);
#ifdef _TESTCOUNT
    FreeSuccessCount++;
#endif // _TESTCOUNT
    FreeNumber++;

    __FREEp(pInProcess)
  __CATCH_BEGIN
    __FREEp(pInProcess)
 __CATCH_END
}

inline void CMemoryListCriticalSection::FreeListToTail(ADDR nlist)
{
    nlist.NextList = MARK_FREE_END_;
    FreeBufferEnd.NextList = nlist;
    FreeBufferEnd = nlist;
}

inline void CMemoryListCriticalSection::FreeListToHead(ADDR nlist)
{
    nlist.NextList = FreeBufferStart;
    FreeBufferStart = nlist;
}

INT CMemoryListCriticalSection::AddToUsed(ADDR nlist)
{
  __LOCKp(pusedProcess)
  nlist.UsedList = UsedItem;
  UsedItem = nlist;
  __FREEp(pusedProcess)
  nlist.CountDown = TimeoutInit;
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

INT CMemoryListArray::GetOneList(ADDR &nlist)
{
}

INT CMemoryListArray::FreeOneList(ADDR nlist)
{
}

INT CMemoryListArray::AddToUsed(ADDR nlist)
{
  return 0;
}

INT CMemoryListArray::SetThreadLocalArray(INT threadnum, INT maxsize, INT getsize)
{
  INT  memorySize = 0;

  __TRY
    memorySize = PAD_SIZE(threadListInfo, SIZE_SMALL_PAD, SIZE_CACHE) * threadnum;
    memorySize += (TotalNumber + (maxsize + SIZE_SMALL_PAD)*threadnum) * sizeof(ADDR);

    nowThread = 0;
    memoryArea = getMemory(memorySize, 0);
    __DO_(memoryArea == MAP_FAIL, "Could NOT mmap memory");
    memoryArrayStart = memoryArea;
    memoryArrayEnd =  memoryArea + (TotalNumber + 1) * sizeof(ADDR);

  __CATCH
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

#define TEST_TIMES 1000000
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
