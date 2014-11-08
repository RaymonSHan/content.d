
#ifndef INCLUDE_RMEMORY_HPP
#define INCLUDE_RMEMORY_HPP

#include "raymoncommon.h"
#include "rtype.hpp"
#include "rthread.hpp"

//#define  _TESTCOUNT
#define  __DIRECT
/////////////////////////////////////////////////////////////////////////////////////////////////////
// const for memory alloc                                                                          //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define MARK_USED_END           0x30
#define MARK_UNUSED             0x28
#define MARK_FREE_END           0x20
#define MARK_USED               0x10
#define MARK_MAX	        0x100

/////////////////////////////////////////////////////////////////////////////////////////////////////
// memory alloc base class                                                                         //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
INT     getMemory(ADDR &addr, INT size, INT flag = 0);
INT     getStack(ADDR &addr);

class CListItem
{
public:
  // LIST for used item, start with CMemoryAlloc::UsedItem; the most use is NonDirectly free
  ADDR  usedList;
  // In NonDirectly free, free the item when equal TIMEOUT_QUIT
  INT   countDown;
  // point to large buffer
  ADDR  linkBuffer;
  // the behavior the item, for these const start with FLAG_
  INT   listFlag;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// per-thread info for memory alloc                                                                //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //

#define MAX_LOCAL_CACHE         16                              // by test, 10 cache is enough
//#define MAX_SHARE_THREAD        128                             // an array save threadMemoryInfo addr

typedef struct threadMemoryInfo {
  INT   getSize;
  INT   freeSize;
  INT   threadFlag;
  ADDR  localArrayStart;
  ADDR  localFreeStart;
  ADDR  localArrayEnd;
  ADDR  localUsedList;
  threadMemoryInfo *threadListNext;
  ADDR  localCache[MAX_LOCAL_CACHE];
}threadMemoryInfo;

#define THREAD_FLAG_GETMEMORY   0x1
#define THREAD_FLAG_SCHEDULE    0x2

#define GetThreadMemoryInfo()			\
  threadMemoryInfo *info;			\
  getThreadInfo(info, nowOffset);

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
  threadMemoryInfo *threadListStart;
  ADDR  memoryArrayStart;       // array start
  ADDR  memoryArrayFree;        // free now
  ADDR  memoryArrayEnd;         // array end

private:                        // lock
  HANDLE_LOCK          InProcess;
  HANDLE_LOCK          *pInProcess;

private:                        // info should setup when init
  INT   DirectFree;
  INT   TimeoutInit;
  INT   BufferSize;

public:
  CMemoryAlloc();
  ~CMemoryAlloc();

private:
  INT   GetOneList(ADDR &nlist);
  INT   FreeOneList(ADDR nlist);
  INT   AddToUsed(ADDR nlist);
  INT   GetListGroup(ADDR &groupbegin, INT number);
  INT   FreeListGroup(ADDR &groupbegin, INT number);
  INT   CountTimeout(ADDR usedStart);

public:
  INT   SetThreadArea(INT getsize, INT maxsize, INT freesize, INT flag);
  INT   SetMemoryBuffer(INT number, INT size, INT border, INT direct, INT buffer = 0);
  INT   DelMemoryBuffer(void);
  INT   GetMemoryList(ADDR &nlist);                             // For DirectFree mode, same as GetOneList
  INT   FreeMemoryList(ADDR nlist);                             // For NON-Direct mode, only make a mark
  INT   TimeoutAll(void);
  INT   GetNumber()             { return TotalNumber; };
  
  void  DisplayFree(void);

#ifdef  _TESTCOUNT              // for test function
public:                         // statistics info for debug
  INT   GetCount, GetSuccessCount;
  INT   FreeCount, FreeSuccessCount;
  INT   MinFree;  
  void  DisplayLocal(threadMemoryInfo* info);
  void  DisplayArray(void);
  void  DisplayInfo(void);
  void  DisplayContext(void);
#endif  // _TESTCOUNT
};

#define NUMBER_CONTENT  100
#define NUMBER_BUFFER   150

/////////////////////////////////////////////////////////////////////////////////////////////////////
// main struct                                                                                     //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
class CContentItem : public CListItem
{
public:
  int   cliHandle;
  int   serHandle;
  SOCKADDR serverSocket;
  SOCKADDR clientSocket;
  //  CContentItem *nextPeer;
  //  CContentItem *prevPeer;
  ADDR  moreContent;
  ADDR  nowBuffer;
  ADDR  moreBuffer;
  ADDR  otherBuffer;
  //  CMemoryAlloc *contentType;
  RThread *workThread;;
};
 
class CBufferItem : public CListItem
{
public:
  INT   nSize;
  INT   nOper;
  //  CMemoryAlloc *bufferType;
  char  *realStart;
  ADDR  nextBuffer;
  char  bufferName[CHAR_SMALL];
  char  ahead[SIZE_NORMAL_PAGE - CHAR_SMALL - 4*8];             // make following page border
  char  bufferData[SIZE_DATA_BUFFER];                           // CBufferItem total size 4M
};

#define UsedList                pList->usedList
#define CountDown               pList->countDown
#define ListFlag                pList->listFlag
#define LinkBuffer              pList->linkBuffer

#define CHandle                 pCont->cliHandle
#define SHandle                 pCont->serHandle
#define ServerSocket            pCont->serverSocket
#define ClientSocket            pCont->clientSocket
#define NextPeer                pCont->nextPeer
#define PrevPeer                pCont->prevPeer
#define MoreContent             pCont->moreContent
#define NowBuffer               pCont->nowBuffer
#define MoreBuffer              pCont->moreBuffer
#define OtherBuffer             pCont->otherBuffer
#define ContentType             pCont->ContentTypess
#define WorkThread              pCont->workThread

#define NSize                   pBuff->nSize
#define NOper                   pBuff->nOper
#define BufferType              pBuff->bufferType
#define RealStart               pBuff->realStart
#define NextBuffer              pBuff->nextBuffer
#define BufferName              pBuff->bufferName
#define BufferData              pBuff->bufferData
#endif
