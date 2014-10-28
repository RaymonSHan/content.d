
// #undef boolean	// for oratypes.h define boolean as int
// #include	<openssl/ssl.h>
// #include	<openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "raymoncommon.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory alloc                                                                                    //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
class   CListItem;
class   CContextItem;

#define MARK_USED_END           0x30
#define MARK_FREE_END           0x20
#define MARK_USED               0x10
//#define MARK_FREE_END           (CListItem*)(void*)0x20
//#define MARK_USED               (CListItem*)(void*)0x10

// The item -> nextList is pointed to next item, but if the value less than MARK_MAX, it means some other
#define MARK_MAX_INT	        0x100ll
#define MARK_MAX                (CListItem*)(void*)MARK_MAX_INT
#define BUFFER_SIZE             16*1024*1024

struct  threadMemoryInfo;
typedef struct perThreadInfo
{
  threadMemoryInfo *memoryListInfo;
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

#define PAD_SIZE(p, pad, bord)			\
  ((sizeof(p) + pad - 1) & (-1 * bord)) + bord
#define PAD_INT(p, pad, bord)				\
  ((p + pad - 1) & (-1 * bord)) + bord
#define PAD_ADDR(p, pad, bord)				\
  ((p.aLong + pad - 1) & (-1 * bord)) + bord

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
  //   ADDR_PVOID_SELF_OPERATION(=)

  // ADDR_PVOID_SELF_OPERATION
  assignAddress& operator = (void *&one) { this->pVoid = one; return *this; };
  assignAddress& operator = (CListItem *&one) { this->pList = one; return *this; };
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
  BOOL inline operator op (ADDR &one, const void* &two) {	\
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

ADDR    getMemory(INT size, INT flag);
ADDR    getStack(void);

class CMemoryAlloc
{
private:
  //  ADDR  StartBlock;
  ADDR  RealBlock;
  INT   BorderSize;                                             // real byte size
  INT   TotalSize;                                              // Total memory size in byte
protected:
  INT   TotalNumber;
  volatile INT                  FreeNumber;
  //  INT   m_ItemSaveRemain;
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

  void  TimeoutContext(void);
  void  DisplayContext(void);
  ADDR  SetBuffer(INT number, INT itemsize, INT bordersize);
  INT   GetNumber()             { return TotalNumber; };
  INT   GetFreeNumber()         { return FreeNumber; };

#ifdef  _TESTCOUNT              // for test function, such a multithread!
  void  DisplayInfo(void);
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

typedef struct threadMemoryInfo {
  INT   maxSize;
  INT   getSize;
  ADDR  localArrayStart;
  ADDR  freeLocalStart;
  ADDR  localArrayEnd;
  ADDR  usedLocalStart;
}threadListInfo;

class CMemoryListArray: public CMemoryAlloc
{
private:
  volatile INT nowThread;
  ADDR  memoryArea;             // used for mmap
  ADDR  memoryArrayStart;       // array start
  ADDR  memoryArrayFree;
  ADDR  memoryArrayEnd;         // array end
  INT   threadNum;
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
  INT   SetThreadArea();

#ifdef  _TESTCOUNT              // for test function, such a multithread!
  void  DisplayArray(void);
#endif  // _TESTCOUNT
};


int                             ThreadItem(void *para);

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




/////////////////////////////////////////////////////////////////////////////////////////////////////
// Used memory struct                                                                              //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
/*
class CBufferItem : public CListItem
{
public:
	WSAOVERLAPPED					oLapped;							//	perI/O must start with OVERLAPPED							//
	CWorkflowNode*					nOperation;							//	perI/O OPERATION, now it is Workflow node					//
	DWORD							nSide;								//	bit 0 is 1 for doloop
// 	DWORD							nHandle;
// 	DWORD							operationSide;																						//
	DWORD							nProcessSize;						//	DATA size of this I/O										//
	CMemoryListUsed*				bufferType;							//	the MEMORYLIST it used										//
	HANDLE							cliHandle;							//	for OnAccept, get the CONNECTED SOCKET						//
// 	__int64							totalLength;						//	size for all linkBuffer										//
	CBufferItem*					nextBuffer;																							//
	CBufferItem*					linkBuffer;																							//
	short							offsetStart;																						//
	unsigned char					offsetData[MINI_CHAR];				//	mini data add to head, most for tunnel						//
// 	long							forTunnelEncap;																						//
// 	long							forTunnelDataSize;					//	MUST save 8 byte for tunnel ahead							//
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define OVERLAP_(p)					__JOIN(p, Buffer->oLapped)
#define OVERLAP						OVERLAP_(m)
#define OPERATION_(p)				__JOIN(p, Buffer->nOperation)
#define OPERATION					OPERATION_(m)
#define OPSIDE_(p)					__JOIN(p, Buffer->nSide)
#define OPSIDE						OPSIDE_(m)
#define PROCESSSIZE_(p)				__JOIN(p, Buffer->nProcessSize)
#define PROCESSSIZE					PROCESSSIZE_(m)
#define BUFFERTYPE_(p)				__JOIN(p, Buffer->bufferType)
#define BUFFERTYPE					BUFFERTYPE_(m)
#define BUFFERSIZE_(p)				__JOIN(p, Buffer->bufferType->BufferSize)
#define BUFFERSIZE					BUFFERSIZE_(m)
#define CLIHANDLE_(p)				__JOIN(p, Buffer->cliHandle)
#define CLIHANDLE					CLIHANDLE_(m)
#define NEXTBUFFER_(p)				__JOIN(p, Buffer->nextBuffer)
#define NEXTBUFFER					NEXTBUFFER_(m)
#define OFFSETSTART_(p)				__JOIN(p, Buffer->offsetStart)
#define OFFSETSTART					OFFSETSTART_(m)

#define BUFFER_(p)					__JOIN(p, Buffer)
#define REALBUFFER_(p)				((CHAR*)(BUFFER_(p)+1))+BUFFER_(p)->offsetStart
#define REALBUFFER					REALBUFFER_(m)
#define REALUBUFFER_(p)				((UCHAR*)(BUFFER_(p)+1))+BUFFER_(p)->offsetStart
#define REALUBUFFER					REALUBUFFER_(m)

#define ORIBUFFER_(p)				(CHAR*)(BUFFER_(p)+1)
#define ORIBUFFER					ORIBUFFER_(m)
#define ORIUBUFFER_(p)				(UCHAR*)(BUFFER_(p)+1)
#define ORIUBUFFER					ORIUBUFFER_(m)

struct HandleStruct
{
	HANDLE							bHandle;							//	the HANDLE/SOCKET it used									//
	CContextItem*					hostContext;						//	pointer to the ContextList it in
	CProtocol*						handleProtocol;
	__int64							overlapOffset;						//	the PACKET OFFSET of the SESSION							//
	CBufferItem*					moreBuffer;							//	some app should use HandleStruct only because will use moreBuffer
	DWORD							contentMode;						//	for CONTEXT_LENGTH CONTEXT_HEAD								//
};

#define BHANDLE_(p)					__JOIN(p, Handle->bHandle)
#define BHANDLE						BHANDLE_(m)
#define HOSTCONTEXT_(p)				__JOIN(p, Handle->hostContext)
#define HOSTCONTEXT					HOSTCONTEXT_(m)
#define HANDLEPROTOCOL_(p)			__JOIN(p, Handle->handleProtocol)
#define HANDLEPROTOCOL				HANDLEPROTOCOL_(m)
#define OFFSET_(p)					__JOIN(p, Handle->overlapOffset)
#define OFFSET						OFFSET_(m)
#define CONTENTMODE_(p)				__JOIN(p, Handle->contentMode)
#define CONTENTMODE					CONTENTMODE_(m)
#define MOREBUFFER_(p)				__JOIN(p, Handle->moreBuffer)
#define MOREBUFFER					MOREBUFFER_(m)

#define WORKFLOW_LOOP				OPSIDE |= WORKFLOW_GOON;
#define WORKFLOW_NOLOOP				OPSIDE &= ~WORKFLOW_GOON;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		//

class CContextItem : public CListItem																									//
{
public:
	LOCK_HANDLE						inHandleProcess;					//	maybe sometime should lock context and buffer info			//
	CWorkflow*						pWorkflow;																							//
// 	MYINT							nowWorkflowNode;					//	In CWorkflow::nodeNumber																//
// 	CProtocol*						pProtocol;							//	the PROTOCOL it used										//
// 	CApplication*					pApplication;						//	the APPLICATION it used										//
// 	CContextItem*					pPeerNext;							//	peer CONTEXT												//
// 	CContextItem*					pPeerPrev;																							//
	CMemoryListUsed*				contextType;						//	the MEMORYLIST it used										//
	MYINT							nowHandle;
	HandleStruct					bHandleStruct[MAX_HANDLE_STRUCT];	//
// 	__int64							bodyRemain;							//	for HTTP style, just for the Content-Length					//
// 	CBufferItem*					moreBuffer;							//	normal DATA in perI/O struct, but more DATA here			//
//																		//	moreBuffer is for every Worknode, not for Context			//
// 	CBufferItem*					firstBuffer;						//	data have read												//
	CBufferItem*					infoBuffer;							//	there are ContentPad info, WSABUF, call stack				//
	DWORD							transferEncoding;					//	for ENCODING_LENGTH, ENCODING_CHUNKED						//
	DWORD							tagID;								//	now for tunnel tag id										//
	sockaddr_in						clientAddr;							//	for UDP & TCP												//
};																																		//

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LISTFLAG_(p)				__JOIN(p, Context->listFlag)
#define LISTFLAG					LISTFLAG_(m)

#define	WORKFLOW_(p)				__JOIN(p, Context->pWorkflow)
#define	WORKFLOW					WORKFLOW_(m)
#define NOWNODE_(p)					__JOIN(p, Context->nowWorkflowNode)
#define NOWNODE						NOWNODE_(m)
#define CONTEXTTYPE_(p)				__JOIN(p, Context->contextType)
#define CONTEXTTYPE					CONTEXTTYPE_(m)
#define NOWHANDLE_(p)				__JOIN(p, Context->nowHandle)
#define NOWHANDLE					NOWHANDLE_(m)
#define	HANDLESTRUCT_(p,i)			__JOIN(p, Context->bHandleStruct[i])
#define	HANDLESTRUCT(i)				HANDLESTRUCT_(m,i)
#define INFOBUFFER_(p)				__JOIN(p, Context->infoBuffer)
#define INFOBUFFER					INFOBUFFER_(m)
#define CLIENTADDR_(p)				__JOIN(p, Context->clientAddr)
#define CLIENTADDR					CLIENTADDR_(m)

#endif _TESTCOUNT
*/
