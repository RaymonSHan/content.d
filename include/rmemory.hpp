
// #undef boolean	// for oratypes.h define boolean as int
// #include	<openssl/ssl.h>
// #include	<openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "raymoncommon.h"

#define  _TESTCOUNT

/////////////////////////////////////////////////////////////////////////////////////////////////////
// const for memory alloc                                                                          //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
class   CListItem;

#define MARK_USED_END           0x30
#define MARK_UNUSED             0x28
#define MARK_FREE_END           0x20
#define MARK_USED               0x10
#define MARK_MAX	        0x100

#define TIMEOUT_TCP             20
#define TIMEOUT_INFINITE        0xffffffff
#define TIMEOUT_QUIT            2

/////////////////////////////////////////////////////////////////////////////////////////////////////
// per thread info, saved at the bottom of stack                                                   //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
struct  threadMemoryInfo;
typedef struct perThreadInfo
{
  threadMemoryInfo              *memoryListInfo;
}perThreadInfo;

#define OFFSET_MEMORYLIST       0

#define setThreadInfo(info, off)					\
  asm volatile ("movq %%rsp, %%rax;"					\
		"andq %2, %%rax;"					\
		"addq %1, %%rax;"					\
		"movq %0, (%%rax);"					\
		: : "b"(info), "i"(off), "i"(NEG_SIZE_THREAD_STACK));

#define getThreadInfo(info, off)			\
  asm volatile ("movq %%rsp, %%rax;"			\
		"andq %2, %%rax;"			\
		"addq %1, %%rax;"			\
		"movq (%%rax), %0;"			\
		: "=b" (info)				\
		: "i" (off), "i"(NEG_SIZE_THREAD_STACK));

/////////////////////////////////////////////////////////////////////////////////////////////////////
// pad size for border, stack must at 2^n border                                                   //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define PAD_SIZE(p, pad, bord)				\
  ((sizeof(p) + pad - 1) & (-1 * bord)) + bord
#define PAD_INT(p, pad, bord)				\
  ((p + pad - 1) & (-1 * bord)) + bord
#define PAD_ADDR(p, pad, bord)				\
  ((p.aLong + pad - 1) & (-1 * bord)) + bord

/////////////////////////////////////////////////////////////////////////////////////////////////////
// the important struct, for my lazy                                                               //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define ADDR_INT_SELF_OPERATION(op)			\
  assignAddress& operator op (const INT &one) { this->aLong op one; return *this; };

#define ADDR_PCHAR_SELF_OPERATION(op)					\
  assignAddress& operator op (char *&one) { this->pChar op one; return *this; };

#define ADDR_PVOID_SELF_OPERATION(op)					\
  assignAddress& operator op (void *&one) { this->pVoid op one; return *this; };

typedef union assignAddress
{
  INT   aLong;
  char  *pChar;
  void  *pVoid;
  INT   *pLong;
  CListItem *pList;
  assignAddress *pAddr;

  ADDR_INT_SELF_OPERATION(=)
  ADDR_INT_SELF_OPERATION(+=)
  ADDR_INT_SELF_OPERATION(-=)
  ADDR_INT_SELF_OPERATION(&=)
  ADDR_INT_SELF_OPERATION(|=)

  ADDR_PCHAR_SELF_OPERATION(=)
  // One question i have not solved is: WHAT WRONG with the following line.
//ADDR_PVOID_SELF_OPERATION(=)

  assignAddress& operator = (void *&one) { this->pVoid = one; return *this; };
  assignAddress& operator = (CListItem *&one) { this->pList = one; return *this; };
  assignAddress& operator = (assignAddress *&one) { this->pAddr = one; return *this; };
}ADDR;

#define ADDR_ADDR_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, ADDR &two) {	\
  return (one.aLong op two.aLong); }
  ADDR_ADDR_COMPARE(==)
  ADDR_ADDR_COMPARE(!=)
  ADDR_ADDR_COMPARE(>)
  ADDR_ADDR_COMPARE(>=)
  ADDR_ADDR_COMPARE(<)
  ADDR_ADDR_COMPARE(<=)

#define ADDR_ADDR_OPERATION(op)				\
  ADDR inline operator op (ADDR &one, ADDR &two) {	\
  ADDR ret;						\
  ret.aLong = one.aLong op two.aLong;  return ret; }
  ADDR_ADDR_OPERATION(+)
  ADDR_ADDR_OPERATION(-)

#define ADDR_INT_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, const INT &two) {	\
  return (one.aLong op two); }
  ADDR_INT_COMPARE(==)
  ADDR_INT_COMPARE(!=)
  ADDR_INT_COMPARE(>)
  ADDR_INT_COMPARE(>=)
  ADDR_INT_COMPARE(<)
  ADDR_INT_COMPARE(<=)

#define ADDR_INT_OPERATION(op)				\
  ADDR inline operator op (ADDR &one, const INT &two) {	\
  ADDR ret;						\
  ret.aLong = one.aLong op two;  return ret; }
  ADDR_INT_OPERATION(+)
  ADDR_INT_OPERATION(-)
  ADDR_INT_OPERATION(&)
  ADDR_INT_OPERATION(|)

#define ADDR_INT_OPERATION2(op)				\
  INT inline operator op (ADDR &one, const INT &two) {	\
    return one.aLong op two; }
  ADDR_INT_OPERATION2(*)
  ADDR_INT_OPERATION2(/)

#define ADDR_PCHAR_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, char* &two) {	\
  return (one.pChar op two); }
  ADDR_PCHAR_COMPARE(==)
  ADDR_PCHAR_COMPARE(!=)
  ADDR_PCHAR_COMPARE(>)
  ADDR_PCHAR_COMPARE(>=)
  ADDR_PCHAR_COMPARE(<)
  ADDR_PCHAR_COMPARE(<=)

#define ADDR_PVOID_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, const void* &two){\
  return (one.pVoid op two); }
  ADDR_PVOID_COMPARE(==)
  ADDR_PVOID_COMPARE(!=)
  ADDR_PVOID_COMPARE(>)
  ADDR_PVOID_COMPARE(>=)
  ADDR_PVOID_COMPARE(<)
  ADDR_PVOID_COMPARE(<=)

#define ADDR_PLONG_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, INT* &two) {	\
  return (one.pLong op two); }
  ADDR_PLONG_COMPARE(==)
  ADDR_PLONG_COMPARE(!=)
  ADDR_PLONG_COMPARE(>)
  ADDR_PLONG_COMPARE(>=)
  ADDR_PLONG_COMPARE(<)
  ADDR_PLONG_COMPARE(<=)

/////////////////////////////////////////////////////////////////////////////////////////////////////
// memory alloc base class                                                                         //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
ADDR    getMemory(INT size, INT flag);
ADDR    getStack(void);

class CListItem
{
public:
  // LIST for free, start with CMemoryAlloc::FreeBufferStart; or be MASK_USED, or MASK_FREE_END
  ADDR  nextList;
  // LIST for used item, start with CMemoryAlloc::UsedItem; the most use is NonDirectly free
  ADDR  usedList;
  // In NonDirectly free, free the item when equal TIMEOUT_QUIT
  INT   countDown;
  // the behavior the item, for these const start with FLAG_
  INT   listFlag;
};

#define NextList                pList->nextList
#define UsedList                pList->usedList
#define CountDown               pList->countDown
#define ListFlag                plist->listFlag
class CMemoryAlloc
{
private:
  ADDR  RealBlock;
  INT   BorderSize;                                             // real byte size
  INT   TotalSize;                                              // Total memory size in byte
protected:
  INT   TotalNumber;
  volatile INT                  FreeNumber;
  ADDR  FreeBufferStart;
  ADDR  FreeBufferEnd;                                          // used in CriticalSection
  ADDR  TotalBuffer;
  ADDR  UsedItem;
  volatile HANDLE_LOCK          InProcess, usedProcess;
  volatile HANDLE_LOCK          *pInProcess, *pusedProcess;
public:
  INT   DirectFree;
  INT   TimeoutInit;
  INT   BufferSize;

#ifdef _TESTCOUNT
public:
  INT   GetCount, GetSuccessCount;
  INT   FreeCount, FreeSuccessCount;
  INT   MinFree;

  INT   FreeLoop, GetLoop, GetFail;
  INT   hStart;
#endif  // _TESTCOUNT

public:
  CMemoryAlloc();
  ~CMemoryAlloc();
  void  SetListDetail(char *listname, INT directFree, INT timeout);
  INT   SetMemoryBuffer(INT number, INT size, INT border);
  INT   DelMemoryBuffer(void);

  virtual INT GetOneList(ADDR &nlist) = 0;
  virtual INT FreeOneList(ADDR nlist) = 0;
  virtual INT AddToUsed(ADDR nlist) = 0;
  INT   GetMemoryList(ADDR &nlist);                             // For DirectFree mode, same as GetOneList
  INT   FreeMemoryList(ADDR nlist);                             // For NON-Direct mode, only make a mark

  void  CountTimeout(ADDR usedStart);
  ADDR  SetBuffer(INT number, INT itemsize, INT bordersize);
  INT   GetNumber()             { return TotalNumber; };
  INT   GetFreeNumber()         { return FreeNumber; };

  virtual INT TimeoutAll(void);
#ifdef  _TESTCOUNT              // for test function, such a multithread!
  void  DisplayInfo(void);
  virtual void DisplayContext(void);
#endif  // _TESTCOUNT
};

class CMemoryListCriticalSection : public CMemoryAlloc
{
private:
  virtual INT                   GetOneList(ADDR &nlist);
  virtual INT                   FreeOneList(ADDR nlist);
  inline void                   FreeListToTail(ADDR nlist);
  inline void                   FreeListToHead(ADDR nlist);
  virtual INT                   AddToUsed(ADDR nlist);
};

class CMemoryListLockFree: public CMemoryAlloc
{
private:
  virtual INT                   GetOneList(ADDR &nlist);
  virtual INT                   FreeOneList(ADDR nlist);
  virtual INT                   AddToUsed(ADDR nlist);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// the class i used in this project,                                                               //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
typedef struct threadMemoryInfo {
  INT   maxSize;
  INT   getSize;
  ADDR  localArrayStart;
  ADDR  freeLocalStart;
  ADDR  localArrayEnd;
  ADDR  usedLocalStart;
  INT   threadFlag;
}threadListInfo;

#define THREAD_FLAG_GETMEMORY   0x1
#define THREAD_FLAG_SCHEDULE    0x2

class CMemoryListArray: public CMemoryAlloc
{
private:
  volatile INT nowThread;       // thread have SetThreadArea
  ADDR  memoryArea;             // used for mmap
  ADDR  memoryArrayStart;       // array start
  ADDR  memoryArrayFree;        // free now
  ADDR  memoryArrayEnd;         // array end
  INT   threadNum;              // thread number in init
  ADDR  threadArea;             // start of threadListInfo
  INT   threadAreaSize;         // pad sizeof(threadListInfo)
  INT   threadArraySize;        // local array size
private:
  virtual INT                   GetOneList(ADDR &nlist);
  virtual INT                   FreeOneList(ADDR nlist);
  virtual INT                   AddToUsed(ADDR nlist);

public:
  INT   GetListGroup(ADDR &groupbegin, INT number);
  INT   FreeListGroup(ADDR &groupbegin, INT number);
  INT   SetThreadLocalArray(INT threadnum, INT maxsize, INT getsize);
  INT   SetThreadArea(INT flag);

  virtual INT TimeoutAll(void);

#ifdef  _TESTCOUNT              // for test function, such a multithread!
  virtual void DisplayContext(void);
  void  DisplayLocal(threadListInfo* info);
  void  DisplayArray(void);
#endif  // _TESTCOUNT
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// thread for get/free and countdown                                                               //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
int     ThreadItem(void *para);
int     ThreadSchedule(void *para);


