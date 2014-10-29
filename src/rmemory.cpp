#include <string.h>
#include <unistd.h>
#include <time.h>
#include "../include/rmemory.hpp"

//#define _TESTCOUNT
volatile INT GlobalTime = 0;

#ifdef  HUGE_PAGE
#define SIZE_PAGE               SIZE_HUGE_PAGE
#else   // HUGE_PAGE
#define SIZE_PAGE               SIZE_NORMAL_PAGE
#endif  // HUGE_PAGE

ADDR getMemory(INT size, INT flag)
{
  static ADDR totalMemoryStart = {SEG_START_STACK};
  ADDR p;
  // per thread info is store in the begining of stack 
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
  GlobalTime = time(NULL);
}

CMemoryAlloc::~CMemoryAlloc()
{
  DelMemoryBuffer();
}

INT CMemoryAlloc::SetMemoryBuffer(INT number, INT size, INT border)
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
      thisItem.UsedList = MARK_UNUSED;
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

INT CMemoryAlloc::DelMemoryBuffer(void)
{
  return 0;
}

void CMemoryAlloc::SetListDetail(char *listname, INT directFree, INT timeout)
{
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
    if (!DirectFree) nlist.CountDown = TIMEOUT_QUIT;
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

  if (nlist.NextList > MARK_MAX) {                              // Have free
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
    __DO_((nlist < MARK_MAX || nlist.NextList != MARK_USED),
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
  nlist.CountDown = GlobalTime;                                 // now time
  __LOCKp(pusedProcess)
  nlist.UsedList = UsedItem;
  UsedItem = nlist;
  __FREEp(pusedProcess)
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
    do {
      nlist.aLong = *start;
      if (nlist > 0)
	changed = __sync_bool_compare_and_swap(start, nlist.aLong, 0);
    } while (!changed);

    next = nlist.NextList;
    if (next > MARK_MAX) {                   // Have free
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

  do {
    do {
      next.aLong = *start;
      if (next > 0)
	changed = __sync_bool_compare_and_swap(start, next.aLong, 0);
    } while (!changed);

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
      GetListGroup(minfo->freeLocalStart, minfo->getSize);
    }
    __DO(minfo->freeLocalStart > minfo->localArrayEnd);
    nlist = *(minfo->freeLocalStart.pAddr);
    minfo->freeLocalStart += sizeof(ADDR);
#ifdef _TESTCOUNT
   __sync_fetch_and_add(&FreeNumber, -1);
   if (FreeNumber < MinFree) MinFree = FreeNumber;
   __sync_fetch_and_add(&GetSuccessCount, 1);
#endif // _TESTCOUNT
  __CATCH
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
    __DO_((nlist < MARK_MAX || nlist.NextList != MARK_USED),
	    "FreeList Twice %p\n", nlist.pList);

    minfo->freeLocalStart -= sizeof(ADDR);
    *(minfo->freeLocalStart.pAddr) = nlist; 
    nlist.NextList = nlist;                                     // only mark for unused
    nlist.UsedList = MARK_UNUSED;                               // mark for unsed too
#ifdef _TESTCOUNT
    __sync_fetch_and_add(&FreeNumber, 1);
    __sync_fetch_and_add(&FreeSuccessCount, 1);
#endif // _TESTCOUNT
  __CATCH
}

// need NOT lock, schedule thread will NOT change usedLocalStart, 
// and NOT remove first node in UsedList, even countdowned.
INT CMemoryListArray::AddToUsed(ADDR nlist)
{
  threadMemoryInfo *minfo;
  getThreadInfo(minfo, OFFSET_MEMORYLIST);

  nlist.CountDown = GlobalTime;                                 // it is now time
  nlist.UsedList = minfo->usedLocalStart;
  minfo->usedLocalStart = nlist;
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

      threadarea += threadAreaSize;
      threadarray += threadArraySize;
    }
  __CATCH
}

INT CMemoryListArray::SetThreadArea(INT flag)
{
  INT   id;
  threadMemoryInfo* info;
  ADDR  addr;

  __TRY
    id = __sync_fetch_and_add(&nowThread, 1);
    __DO(id >= threadNum);
    
    addr = threadArea + id * threadAreaSize;
    info = (threadMemoryInfo*)addr.pVoid;
    info->threadFlag = flag;
    if (flag & THREAD_FLAG_SCHEDULE) {
      info->localArrayStart = info->freeLocalStart;
      info->maxSize = info->getSize;
    }
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

// process from CMemoryAlloc::UsedItem or threadMemoryInfo::usedListStart
// it will not change the in para, and do NOT remove first node even countdowned.
void CMemoryAlloc::CountTimeout(ADDR usedStart)
{
  static volatile INT inCountDown = NOT_IN_PROCESS;
  INT   nowTimeout = GlobalTime - TIMEOUT_TCP;
  ADDR  thisAddr, nextAddr;

  if (!CmpExg(&inCountDown, NOT_IN_PROCESS, IN_PROCESS)) return;
  if (usedStart > MARK_MAX) {
    thisAddr = usedStart;
    nextAddr = thisAddr.UsedList;
    if (thisAddr.CountDown <= TIMEOUT_QUIT) {                   // countdown first node but not free
      if (thisAddr.CountDown) {
	if (thisAddr.CountDown-- <= 0) {
	  thisAddr = thisAddr;                                  // only for cheat compiler
	  // DO close thisAddr.Handle
	  // and thisAddr.Handle = 0;
	} } }
    else if (thisAddr.CountDown < nowTimeout) thisAddr.CountDown = TIMEOUT_QUIT;

    while (nextAddr > MARK_MAX) {
      if (nextAddr.CountDown <= TIMEOUT_QUIT) {
	if (nextAddr.CountDown-- <= 0) {                        // maybe -1
	  thisAddr.UsedList = nextAddr.UsedList;                // step one
	  // DO close nextAddr.Handle
	  // and nextAddr.Handle = 0;
	  FreeOneList(nextAddr);
	} }
      else if (nextAddr.CountDown < nowTimeout) nextAddr.CountDown = TIMEOUT_QUIT;

      if (thisAddr.UsedList == nextAddr) thisAddr = nextAddr;   // have do step one, no go next
      nextAddr = thisAddr.UsedList;
    } }
  inCountDown = NOT_IN_PROCESS;
  return;
}

INT CMemoryAlloc::TimeoutAll(void)
{
  GlobalTime = time(NULL);
  CountTimeout(UsedItem);
  return 0;
}

INT CMemoryListArray::TimeoutAll(void)
{
  INT   i;
  ADDR  infoAddr;
  threadMemoryInfo *info;
  GlobalTime = time(NULL);
  infoAddr = threadArea;

  for (i=0; i<nowThread; i++) {
    info = (threadMemoryInfo*)infoAddr.pVoid;
    CountTimeout(info->usedLocalStart);
    infoAddr += threadAreaSize;
  }
  return 0;
}

#ifdef _TESTCOUNT
void CMemoryListArray::DisplayLocal(threadListInfo* info)
{
  printf("%lld, free:%p, start:%p, end:%p\n", info->maxSize, info->freeLocalStart.pVoid,
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

void CMemoryListArray::DisplayArray(void)
{
  INT i;
  printf("Array Start:%p, Free:%p, End%p\n", 
	 memoryArrayStart.pVoid, memoryArrayFree.pVoid, memoryArrayEnd.pVoid);
  ADDR arr = memoryArrayStart;
  threadListInfo** info = (threadListInfo**) &arr;
  for (i=0; i<TotalNumber; i++) {
    printf("%p, %p, %p\n", arr.pVoid, (*arr.pAddr).pVoid, (*arr.pAddr).NextList.pVoid);
    arr += sizeof(ADDR);
  }
  for (i=0; i<threadNum; i++) {
    arr = threadArea + i * threadAreaSize;
    printf("thread :%lld, ", i);
    DisplayLocal(*info);
  }
  printf("\n");
}

void CMemoryAlloc::DisplayContext(void)
{
  ADDR nlist = UsedItem;
  while (nlist > MARK_MAX) {
    printf("%p:%lld->", nlist.pVoid, nlist.CountDown);
    nlist = nlist.UsedList;
  }
  if (UsedItem.aLong) printf("\n");
}

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

void CMemoryListArray::DisplayContext(void)
{
  INT   i;
  ADDR nlist;
  ADDR  infoAddr;
  threadMemoryInfo *info;
  GlobalTime = time(NULL);
  infoAddr = threadArea;

  for (i=0; i<nowThread; i++) {
    info = (threadMemoryInfo*)infoAddr.pVoid;
    if (info->threadFlag & THREAD_FLAG_GETMEMORY) {
      nlist = info->usedLocalStart;
      printf("thread:%lld:->", i);
      while (nlist > MARK_MAX) {
	printf("%p:%lld->", nlist.pVoid, nlist.CountDown);
	nlist = nlist.UsedList;
      }
      if (info->usedLocalStart.aLong) printf("\n");
    }
    infoAddr += threadAreaSize;
  }
}
#endif // _TESTCOUNT
