
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
  //  InUsedListProcess = NOT_IN_PROCESS;

#ifdef _TESTCOUNT
  GetCount = GetSuccessCount = FreeCount = FreeSuccessCount = 0;
  //  hStart = CreateEvent(0, TRUE, FALSE, 0);
#endif // _TESTCOUNT

  DirectFree = 1;
}

CMemoryAlloc::~CMemoryAlloc()
{
  DelMemoryBuffer();
}

INT     CMemoryAlloc::SetMemoryBuffer(INT number, INT size, INT border)
{
  //  INT   realSize;
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
    __DO_(GetOneList(nlist), "Not alloc context!\n")
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
}


INT CMemoryListCriticalSection::GetOneList(ADDR &nlist)
{
  __LOCK(InProcess)
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

  __FREE(InProcess)
  return (nlist.pList == NULL);
}

INT CMemoryListCriticalSection::FreeOneList(ADDR nlist)
{
  __TRY
    __LOCK(InProcess)
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
    __FREE(InProcess)
  __CATCH_BEGIN
    __FREE(InProcess)
 __CATCH_END
}

INT CMemoryListCriticalSection::AddToUsed(ADDR nlist)
{
  __LOCK(usedProcess)
  nlist.pList->usedList = UsedItem;
  UsedItem = nlist;
  __FREE(usedProcess)
  nlist.pList->countDown = TimeoutInit;
  return 0;
}

INT CMemoryListLockFree::GetOneList(ADDR &nlist)
{
  ADDR  next;
#ifdef _TESTCOUNT
  __sync_fetch_and_add(&GetCount, 1);
  __sync_fetch_and_add(&GetSuccessCount, 1);
#endif // _TESTCOUNT

  do {
    nlist = FreeBufferStart;
    next = FreeBufferStart.pList->nextList;
    //    printf("before nlist,next: %llx,%llx\n", nlist.aLong, next.aLong);
    if (next > MARK_MAX_INT) {                   // Have free
      next.aLong = CmpExg(&FreeBufferStart.aLong, nlist.aLong, next.aLong);
      //    printf("after nlist,next: %llx,%llx\n", nlist.aLong, next.aLong);
    }
    else {
      nlist.pList = NULL;
#ifdef _TESTCOUNT
      __sync_fetch_and_add(&GetSuccessCount, -1);
#endif // _TESTCOUNT
      break;
    }
  }
  while(next != nlist);
  if (nlist.pList != NULL) __sync_fetch_and_add(&FreeNumber, -1);

  return (nlist.pList == NULL);
}

INT CMemoryListLockFree::FreeOneList(ADDR nlist)
{
  printf("In free\n");
  ADDR  next;
#ifdef _TESTCOUNT
  __sync_fetch_and_add(&FreeCount, 1);
  __sync_fetch_and_add(&FreeSuccessCount, 1);
#endif // _TESTCOUNT
  do {
    next = FreeBufferStart;
    nlist.pList->nextList = FreeBufferStart;
    nlist.aLong = CmpExg(&FreeBufferStart.aLong, next.aLong, nlist.aLong);
  }
  while (next != nlist);
  __sync_fetch_and_add(&FreeNumber, 1);
  return 0;
}

INT CMemoryListLockFree::AddToUsed(ADDR )//nlist)
{
  return 0;
}

int main(int, char**)
{
  ADDR nlist[250];
  //  CMemoryListCriticalSection m;
  CMemoryListLockFree m;

  m.SetMemoryBuffer(10, 80, 64);
  m.DisplayInfo();
  for (int i=0; i<9; i++) {
    m.GetMemoryList(nlist[i]);
  }
  m.DisplayInfo();
  
  for (int i=10; i<11; i++) {
    m.GetMemoryList(nlist[i]);
  }
  m.DisplayInfo();
  for (int i=6; i>3; i--) {
    m.FreeMemoryList(nlist[i]);
  }
  m.DisplayInfo();
  for (int i=20; i<25; i++) {
    m.GetMemoryList(nlist[i]);
  }
  m.DisplayInfo();

  return 0;
}

#ifdef _TESTCOUNT


void CMemoryAlloc::DisplayInfo(void)
{
  printf("Total:%10lld, Free:%10lld\n", TotalNumber, FreeNumber);
  printf("Get  :%10lld, Succ:%10lld\n", GetCount, GetSuccessCount);
  printf("Free :%10lld, Succ:%10lld\n", FreeCount, FreeSuccessCount);
  printf("MinFree:%8lld\n", MinFree);

  volatile ADDR item;
  printf("buffer start, end: %llx, %llx\n", FreeBufferStart.aLong, FreeBufferEnd.aLong); 
  item.pList = RealBlock.pList;
  for (int i=0; i<TotalNumber; i++) {
    printf("%5d:%p, %p, %p,\n", i, item.pList, item.pList->nextList.pVoid, item.pList->usedList.pVoid);
    usleep(10);
    item.aLong += BorderSize;
  }
}

/*
void CMemoryAlloc::ResetCount()
{
	GetCount = GetSuccessCount = FreeCount = FreeSuccessCount = 0;
	FreeNumber = MinFree = TotalNumber;
	ResetEvent(hStart);
}

void CMemoryAlloc::CheckEmptyList( void )
{
	MYINT i = 0;
	CListItem* listitem;

	listitem = (CListItem*)FreeBufferStart;
	do 
	{
		listitem = listitem->nextList;
		i++;
	} while (listitem != MARK_FREE_END);

	DEBUG_INFOS(MESSAGE_DISPLAY, "TotalItem:%5d, FreeItem:%5d, Remain:%5d, CheckInList:%5d\r\n", TotalNumber, FreeNumber, m_ItemSaveRemain, i);
	DEBUG_INFOS(MESSAGE_DISPLAY, "Get:%10d, Succ:%10d, Free:%10d, Succ:%10d\r\n", GetCount, GetSuccessCount, FreeCount, FreeSuccessCount);
	DEBUG_INFOS(MESSAGE_DISPLAY, "MinFree:%5d\r\n", MinFree);
}

void CMemoryAlloc::TestList( INT threadNumber, INT getNumber, INT fullTimes, BOOL wait ) 

{
	static volatile INT totalThread = 0;
	static HANDLE threadGetFree[MAXIMUM_WAIT_OBJECTS];
	INT i;
	pthreadPara para;
	unsigned controlThreadAddr;


	for (i=0; i<threadNumber; i++)
	{
		para = (pthreadPara)(malloc(100));
		para->pMemoryList = this;
		para->threadID = InterInc(&totalThread)-1;
		para->getnumber = getNumber;
		para->fullnumber = fullTimes;
		threadGetFree[para->threadID] = 
			(HANDLE)_beginthreadex ( NULL, 0, ThreadItem, (LPVOID)para, 0, &controlThreadAddr );
	}
	if (wait) 
	{
		SetEvent(hStart);

		WaitForMultipleObjects((LONG)totalThread, threadGetFree, TRUE, INFINITE);			// in debug test
		for (i=0; i<threadNumber; i++) 
		{
			TerminateThread(threadGetFree[i], 1);
			CloseHandle(threadGetFree[i]);
		}
		totalThread = 0;		// this means, this test is finished, wait for another test. // I found this line, for an hour.
	}
}

UINT WINAPI CMemoryAlloc::ThreadItem( LPVOID lpParam )
{
#define MAX_TRY_TIME		8

	pthreadPara lppara = (pthreadPara)lpParam;
	CMemoryAlloc* pthis = lppara->pMemoryList;
	CListItem **itembuffer = (CListItem**) malloc(sizeof(void*)*(2+lppara->getnumber));
	CListItem **nowitem;
	MYINT i, j, k;
	DWORD ret;

	WaitForSingleObject(pthis->hStart, INFINITE);				// all thread start at the same time
	for (i=0; i<lppara->fullnumber; i++)
	{
		nowitem = itembuffer;
		for (j=0; j<lppara->getnumber; j++)
		{
			k = 0;
			do 
			{
				ret = pthis->GetOneList(*nowitem);
				k++;
			}
			while (ret && k<MAX_TRY_TIME);
			if (ret) *nowitem = 0;
			nowitem++;
		}
		nowitem = itembuffer;
		for (j=0; j<lppara->getnumber; j++)
		{
			k = 0;
			do 
			{
				if (*nowitem) ret = pthis->FreeOneList(*nowitem);
				k++;
			}
			while (ret && k<MAX_TRY_TIME);
			nowitem++;
		}
	}

	free(itembuffer);
	free(lpParam);
	return 0;
}

void TestListDemo(void)
{
	LARGE_INTEGER tstart, tend, tfreq;
	double passSec, getuSec;

#define MY_THREAD	16
#define MY_NUMBER	3
#define MY_TIMES	1000000

	CMemoryListCriticalSection cList;

	cList.SetBuffer<CListItem>(25, CACHE_SIZE);		//24 NOT - 26 OK
	cList.SetListDetail("test",TRUE,10);

	QueryPerformanceFrequency(&tfreq);

	for (int i=8; i<=MY_THREAD; i++)
	{
		QueryPerformanceCounter(&tstart);

		cList.TestList(i, MY_NUMBER, MY_TIMES/i, 1);

		QueryPerformanceCounter(&tend);
		passSec = ((double)(tend.QuadPart-tstart.QuadPart) / (tfreq.QuadPart));
		getuSec = passSec / (MY_THREAD*MY_NUMBER*MY_TIMES) * 1000 * 1000;

		printf(_T("Total Sec:%10.2f, Per uSec:%10.6f\r\n"), passSec, getuSec);
		// in My Core2 Quad Q8400 2.6G, _DEBUG for Interlocked. All the test for 1B times get/free.
		// FastbutError,	thread, per uSec:	1,0.23u; 2,0.34u; 3,0.44u; 4,0.51u; 5,0.53u; 6,0.57u; 
		//										7,0.61u; 8,0.65u; 9,0.70u; 10,0.65u							// the real interlock
		// CriticalSection,	thread, per uSec:	1,0.13u; 2,0.44u; 3,1.52u; 4,2.45/							// same cri

		cList.CheckEmptyList();
		cList.ResetCount();
	}

};

int main(int argc, TCHAR* argv[], TCHAR* envp[])
{
	TestListDemo();
// 	typical speed in T420 i5-2410M		//	test in Nov. 06 '13
// 	32bit VM	1:0.007;	2:0.023;	3:0.045;	4:0.056;	5:0.061;	6:0.079;	7:0.091;	8:0.105;
// 	64bit		1:0.005;	2:0.021;	3:0.036;	4:0.041;	5:0.042;	6:0.056;	7:0.066;	8:0.078;
// 	64bit WOW	1:0.009;	2:0.018;	3:0.034;	4:0.042;	5:0.053;	6:0.059;	7:0.072;	8:0.085;
}
*/

#endif // _TESTCOUNT



