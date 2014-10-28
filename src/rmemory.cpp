
//#define _TESTCOUNT
#include <string.h>
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
  // some per thread info is store in the begining of stack 
  size = PAD_INT(size, 0, SIZE_THREAD_STACK);
  p = __sync_fetch_and_add(&(totalMemoryStart.aLong), size);
  p.pVoid = mmap (p.pVoid, size, PROT_READ | PROT_WRITE,
		  MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED | flag, -1, 0);
  return p;
}

ADDR getStack(void)
{
  ADDR  stack = getMemory(SIZE_THREAD_STACK, MAP_GROWSDOWN);
  if (stack.aLong & (SIZE_THREAD_STACK - 1)) 
    stack = MAP_FAIL;
  return stack;
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
    nextItem.NextList = MARK_FREE_END;
    
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
    nlist.NextList = MARK_USED;
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
    __DO_((nlist < MARK_MAX_INT || nlist.NextList != MARK_USED),
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
    nlist.NextList = MARK_FREE_END;
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
      if (nlist > 0)
	changed = __sync_bool_compare_and_swap(start, nlist.aLong, 0);
    } while (!changed);

    changed = 0;
    next = nlist.NextList;
    if (next > MARK_MAX_INT) {                   // Have free
      changed = __sync_bool_compare_and_swap(start, 0, next.aLong);
    }
    else {
      nlist = NUL;
#ifdef _TESTCOUNT
      __sync_fetch_and_add(&GetSuccessCount, -1);
#endif // _TESTCOUNT
      break;
    }
  } while (!changed);
  if (nlist != NUL) __sync_fetch_and_add(&FreeNumber, -1);

  return (nlist == NUL);
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
      if (next > 0)
	changed = __sync_bool_compare_and_swap(start, next.aLong, 0);
    } while (!changed);

    changed = 0;

    nlist.NextList = next;
    changed = __sync_bool_compare_and_swap(start, 0, nlist.aLong);
  }
  while (!changed);
  __sync_fetch_and_add(&FreeNumber, 1);
  return 0;
}

INT CMemoryListLockFree::AddToUsed(ADDR)// nlist)
{
  return 0;
}

INT CMemoryListArray::GetOneList(ADDR &nlist)
{
  threadMemoryInfo *minfo;
  getThreadInfo(minfo, OFFSET_MEMORYLIST);

  __TRY
#ifdef _TESTCOUNT
    __sync_fetch_and_add(&GetCount, 1);
#endif // _TESTCOUNT
    if (minfo->freeLocalStart > minfo->localArrayEnd) {
  //    if (minfo->freeLocalStart >= minfo->localArrayEnd) {
      GetListGroup(minfo->freeLocalStart, minfo->getSize);
    }
    __DO(minfo->freeLocalStart > minfo->localArrayEnd);
//   __DO(minfo->freeLocalStart >= minfo->localArrayEnd);
    nlist = *(minfo->freeLocalStart.pAddr);
    minfo->freeLocalStart += sizeof(ADDR);
#ifdef _TESTCOUNT
  __sync_fetch_and_add(&GetSuccessCount, 1);
#endif // _TESTCOUNT
  __CATCH
    //  printf("%p\n", nlist.pVoid);
}

INT CMemoryListArray::FreeOneList(ADDR nlist)
{
  threadMemoryInfo *minfo;
  getThreadInfo(minfo, OFFSET_MEMORYLIST);

  __TRY
#ifdef _TESTCOUNT
  __sync_fetch_and_add(&FreeCount, 1);
#endif // _TESTCOUNT
    if (minfo->freeLocalStart <= minfo->localArrayStart) {
      FreeListGroup(minfo->freeLocalStart, minfo->getSize);
    }
    __DO_((nlist < MARK_MAX_INT || nlist.NextList != MARK_USED),
	    "FreeList Twice %p\n", nlist.pList);

    minfo->freeLocalStart -= sizeof(ADDR);
    *(minfo->freeLocalStart.pAddr) = nlist; 
    nlist.NextList = nlist;                                     // only mark for unused
#ifdef _TESTCOUNT
  __sync_fetch_and_add(&FreeSuccessCount, 1);
#endif // _TESTCOUNT
  __CATCH

  return 0;
}

INT CMemoryListArray::AddToUsed(ADDR)// nlist)
{
  threadMemoryInfo *minfo;
  getThreadInfo(minfo, OFFSET_MEMORYLIST);
  return 0;
}

INT CMemoryListArray::SetThreadLocalArray(INT threadnum, INT maxsize, INT getsize)
{
  INT   memorySize = 0;
  ADDR  memoryarray, memoryarraylist;
  ADDR  threadarray, threadarea;
  ADDR  threadfreestart;
  threadListInfo **info = (threadListInfo**)&(threadarea);
  INT   i;
  BOOL  isOK;

  __TRY
    __DO_(!TotalNumber, "Must call SetMemoryBuffer before SetThreadLocalArray");
    isOK = (maxsize > getsize) && (TotalNumber > (getsize+1)*threadnum+1);
    __DO_(!isOK, "Invalid number for CMemoryList");

    // threadArraySize means every thread local array size, in byte with pad
    threadArraySize = (maxsize + SIZE_SMALL_PAD) * sizeof(ADDR);
    // threadAreaSize means thread info size, in byte with pad
    threadAreaSize = PAD_SIZE(threadListInfo, SIZE_SMALL_PAD, SIZE_CACHE);
    // memorySize means total size in byte
    memorySize = TotalNumber * sizeof(ADDR) + (threadAreaSize + threadArraySize) * threadnum;
    memorySize = PAD_INT(memorySize, SIZE_CACHE, SIZE_PAGE);
    nowThread = 0;
    threadNum = threadnum;
    memoryArea = getMemory(memorySize, 0);
    __DO_(memoryArea == MAP_FAIL, "Could NOT mmap memory");
    memoryArrayStart = memoryArrayFree = memoryArea;
    memoryArrayEnd =  memoryArea + (TotalNumber - 1) * sizeof(ADDR);
    memoryarray = memoryArea;
    memoryarraylist = FreeBufferStart;
    // Init global array
    for(i=0; i<TotalNumber; i++) {
      *(memoryarray.pAddr) = memoryarraylist;
      memoryarray += sizeof(ADDR);
      memoryarraylist = memoryarraylist.NextList;
    }

    // Init thread local area
    threadarray = memoryArea + TotalNumber * sizeof(ADDR);
    threadArea = threadarray + threadArraySize * threadnum;
    threadArea = PAD_ADDR(threadArea, 0, SIZE_CACHE);
    threadarea = threadArea;
    for (i=0; i<threadnum; i++) {
      (*info)->maxSize = maxsize;
      (*info)->getSize = getsize;
      (*info)->localArrayStart = threadarray;
      (*info)->freeLocalStart = threadfreestart =	\
	threadarray + (maxsize-getsize)*sizeof(ADDR);
      (*info)->localArrayEnd = threadarray + (maxsize-1)*sizeof(ADDR);
      (*info)->usedLocalStart = MARK_USED_END;
      // Init thread local array
      memcpy (threadfreestart.pVoid, memoryArrayFree.pVoid, getsize*sizeof(ADDR));
      memoryArrayFree += getsize*sizeof(ADDR);
      //      (*threadfreeend.pAddr).NextList = MARK_FREE_END;

      threadarea += threadAreaSize;
      threadarray += threadArraySize;
    }

  __CATCH
  return 0;
}

INT CMemoryListArray::SetThreadArea()
{
  INT   id;

  __TRY
    id = __sync_fetch_and_add(&nowThread, 1);
    __DO(id >= threadNum);
    
    setThreadInfo(threadArea + id * threadAreaSize, OFFSET_MEMORYLIST );
  __CATCH
}

// groupbegin is local freeLocalStart
INT CMemoryListArray::GetListGroup(ADDR &groupbegin, INT number)
{
  INT   getsize, getnumber;
  __TRY
    __LOCKp(pInProcess)
    getsize = memoryArrayEnd.aLong - memoryArrayFree.aLong;
    getnumber = getsize / sizeof(ADDR);
    if (getnumber < number) number = getnumber;
    if (number) {
      getsize = number * sizeof(ADDR);
      groupbegin -= getsize;                                      // change thread's freeLocalStart
      memcpy(groupbegin.pVoid, memoryArrayFree.pVoid, getsize);
      memoryArrayFree += getsize;
    }
    __FREEp(pInProcess)
  __CATCH
}

// groupbegin is local freeLocalStart
INT CMemoryListArray::FreeListGroup(ADDR &groupbegin, INT number)
{
  INT   freesize;
  __TRY
    __LOCKp(pInProcess)
    freesize = number * sizeof(ADDR);
    if (number) {
      memoryArrayFree -= freesize;
      memcpy(memoryArrayFree.pVoid, groupbegin.pVoid, freesize);
      groupbegin += freesize;
    }
    __FREEp(pInProcess)
  __CATCH
}

void displaylocal(threadListInfo* info)
{
  printf("%lld,%p, %p,%p\n", info->maxSize, info->freeLocalStart.pVoid,
	 info->localArrayStart.pVoid, info->localArrayEnd.pVoid);
  ADDR list = info->localArrayStart;
  for (int i=0; i<info->maxSize; i++) {
    if (list.pAddr->pVoid)
      printf("%p, %p, %p\n", list.pVoid, list.pAddr->pVoid, list.pAddr->NextList.pVoid);
    else
      printf("%p, %p, (nil)\n", list.pVoid, list.pAddr->pVoid);
    list += sizeof(ADDR);
  }
}

#include <sched.h>
#include <sys/wait.h>
#define THREADS 4

#ifdef  _TESTCOUNT
void CMemoryListArray::DisplayArray(void)
{
  INT i;
  printf("Array Start:%p, Free:%p, End%p\n", memoryArrayStart.pVoid, memoryArrayFree.pVoid, memoryArrayEnd.pVoid);
  ADDR arr = memoryArrayStart;
  threadListInfo** info = (threadListInfo**) &arr;
  for (i=0; i<TotalNumber; i++) {
    printf("%p, %p, %p\n", arr.pVoid, (*arr.pAddr).pVoid, (*arr.pAddr).NextList.pVoid);
    arr += sizeof(ADDR);
  }
  for (i=0; i<threadNum; i++) {
    arr = threadArea + i * threadAreaSize;
    printf("thread :%lld, ", i);
    displaylocal(*info);
  }
  printf("\n");
}
#endif  // _TESTCOUNT

int main(int, char**)
{
  ADDR cStack;
  CMemoryListArray m;
  //CMemoryListCriticalSection m;
  //CMemoryListLockFree m;
  int status;
 
  m.SetMemoryBuffer(100, 80, 64);
  m.SetThreadLocalArray(THREADS, 16, 8);

  for (int i=0; i<THREADS; i++) {
    cStack = getStack();
    clone (&ThreadItem, cStack.pChar + SIZE_THREAD_STACK, CLONE_VM | CLONE_FILES, &m);
  }
  for (int i=0; i<THREADS; i++) waitpid(-1, &status, __WCLONE);

#ifdef  _TESTCOUNT              // for test function, such a multithread!
  m.DisplayArray();
  m.DisplayInfo();
#endif  // _TESTCOUNT
 return 0;
}


#define TEST_TIMES 5000000
#define TEST_ITMES 15

int ThreadItem(void *para)
{
  ADDR item[TEST_ITMES+2];
  CMemoryAlloc *cmem = (CMemoryAlloc*) para;
  CMemoryListArray *clist = (CMemoryListArray*) para;

  __TRY
    __DO_(clist->SetThreadArea(), "Too many thread than setting");
   for (int i=0; i<TEST_TIMES; i++) {
      for (int j=0; j<TEST_ITMES; j++) 
	if (cmem->GetMemoryList(item[j])) item[j]=0;
      //      printf("%p,%p,%p\n", item[0].pVoid, item[1].pVoid, item[2].pVoid);
      //      exit(0);
      for (int j=0; j<TEST_ITMES; j++) 
	if (item[j].aLong != 0) cmem->FreeMemoryList(item[j]);
    }
  __CATCH
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


#endif // _TESTCOUNT



/*
60M times 9.2 2thread1cpu lockfree, 11.2 2thread2cpu lockfree, 3.4*2 1thread lockfree
60M times 5.9 2thread2cpu semi, 1.0*2 1thread semi
60M times 6.3 2thread2cpu semi add head, 1.1*2 1thread semiadd head
60M times 4.0 2thread2cpu localarray, 1.1 2thread2cpu localarray without add count
this is the best way i have found, about 30ns per Get/Free for one cpu
 */
