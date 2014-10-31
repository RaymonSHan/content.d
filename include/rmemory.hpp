
#ifndef INCLUDE_RMEMORY_HPP
#define INCLUDE_RMEMORY_HPP

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "raymoncommon.h"
#include "rtype.hpp"
#include "rthread.hpp"

#define  _TESTCOUNT

/////////////////////////////////////////////////////////////////////////////////////////////////////
// const for memory alloc                                                                          //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define MARK_USED_END           0x30
#define MARK_UNUSED             0x28
#define MARK_FREE_END           0x20
#define MARK_USED               0x10
#define MARK_MAX	        0x100

#define TIMEOUT_TCP             2
#define TIMEOUT_INFINITE        0xffffffff
#define TIMEOUT_QUIT            2

/////////////////////////////////////////////////////////////////////////////////////////////////////
// memory alloc base class                                                                         //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
ADDR    getMemory(INT size, INT flag);
ADDR    getStack(void);

class CListItem
{
public:
  // LIST for used item, start with CMemoryAlloc::UsedItem; the most use is NonDirectly free
  ADDR  usedList;
  // In NonDirectly free, free the item when equal TIMEOUT_QUIT
  INT   countDown;
  // the behavior the item, for these const start with FLAG_
  INT   listFlag;
  // point to large buffer
  ADDR  otherBuffer;
};

#define UsedList                pList->usedList
#define CountDown               pList->countDown
#define ListFlag                plist->listFlag
#define OtherBuffer             plist->otherBuffer

/////////////////////////////////////////////////////////////////////////////////////////////////////
// per-thread info for memory alloc                                                                //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //

#define MAX_LOCAL_CACHE         16                              // by test, 10 cache is enough
#define MAX_SHARE_THREAD        128                             // an array save threadMemoryInfo addr

typedef struct threadMemoryInfo {
  INT   getSize;
  INT   freeSize;
  INT   threadFlag;
  ADDR  localArrayStart;
  ADDR  localFreeStart;
  ADDR  localArrayEnd;
  ADDR  localUsedList;
  INT   pad;
  ADDR  localCache[MAX_LOCAL_CACHE];
}threadListInfo;

#define THREAD_FLAG_GETMEMORY   0x1
#define THREAD_FLAG_SCHEDULE    0x2

/////////////////////////////////////////////////////////////////////////////////////////////////////
// malloc alloc class                                                                              //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
class CMemoryAlloc : public RThreadResource
{
private:                        // for total memory
  ADDR  RealBlock;              // address for memory start
  INT   BorderSize;             // real byte size for one item
  INT   ArraySize;              // number of sizeof(ADDR)
  INT   TotalSize;              // Total memory size in byte

private:                        // for thread info
  INT   TotalNumber;
  volatile INT nowThread;       // thread have SetThreadArea
  threadListInfo                *threadListAddr[MAX_SHARE_THREAD];
  ADDR  memoryArrayStart;       // array start
  ADDR  memoryArrayFree;        // free now
  ADDR  memoryArrayEnd;         // array end

private:                        // lock
  volatile HANDLE_LOCK          InProcess;
  volatile HANDLE_LOCK          *pInProcess;

private:                        // info should setup when init
  INT   DirectFree;
  INT   TimeoutInit;
  INT   BufferSize;

#ifdef _TESTCOUNT
public:                         // statistics info for debug
  INT   GetCount, GetSuccessCount;
  INT   FreeCount, FreeSuccessCount;
  INT   MinFree;
#endif  // _TESTCOUNT

public:
  CMemoryAlloc();
  ~CMemoryAlloc();
  INT   SetThreadArea(INT getsize, INT maxsize, INT freesize, INT flag);

  void  SetListDetail(char *listname, INT directFree, INT timeout);
  INT   SetMemoryBuffer(INT number, INT size, INT border, INT direct);
  INT   DelMemoryBuffer(void);

  INT   GetOneList(ADDR &nlist);
  INT   FreeOneList(ADDR nlist);
  INT   AddToUsed(ADDR nlist);
  INT   GetMemoryList(ADDR &nlist);                             // For DirectFree mode, same as GetOneList
  INT   FreeMemoryList(ADDR nlist);                             // For NON-Direct mode, only make a mark

  void  CountTimeout(ADDR usedStart);
  ADDR  SetBuffer(INT number, INT itemsize, INT bordersize);
  INT   GetNumber()             { return TotalNumber; };
//  INT   GetFreeNumber()         { return FreeNumber; };

public:
  INT   GetListGroup(ADDR &groupbegin, INT number);
  INT   FreeListGroup(ADDR &groupbegin, INT number);
  INT   SetThreadLocalArray();
  INT   SetThreadArea(INT flag);

  INT   TimeoutAll(void);
#ifdef  _TESTCOUNT              // for test function, such a multithread!
  void  DisplayInfo(void);
  void  DisplayContext(void);
  void  DisplayLocal(threadListInfo* info);
  void  DisplayArray(void);
#endif  // _TESTCOUNT
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// thread for get/free and countdown                                                               //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
int     ThreadItem(void *para);
int     ThreadSchedule(void *para);

#endif  // INCLUDE_RMEMORY_HPP
