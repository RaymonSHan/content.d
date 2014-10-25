
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

#define MARK_FREE_END           (CListItem*)(void*)0x20
#define MARK_USED               (CListItem*)(void*)0x10

// The item -> nextList is pointed to next item, but if the value less than MARK_MAX, it means some other
#define MARK_MAX_INT	        0x100
#define MARK_MAX                (CListItem*)(void*)MARK_MAX_INT
#define BUFFER_SIZE             16*1024*1024

typedef union assignAddress
{
  INT   aLong;
  char  *pChar;
  void  *pVoid;
  INT   *pLong;
  CListItem *pList;
}ADDR;

BOOL inline operator == (const ADDR &one, const ADDR &two) {
  return (one.aLong == two.aLong);
}

BOOL inline operator != (const ADDR &one, const ADDR &two) {
  return (one.aLong != two.aLong);
}

BOOL inline operator == (const ADDR &one, const void* &two) {
  return (one.pVoid == two);
}

BOOL inline operator != (const ADDR &one, const void* &two) {
  return (one.pVoid != two);
}

BOOL inline operator == (const ADDR &one, const INT &two) {
  return (one.aLong == two);
}

BOOL inline operator != (const ADDR &one, const INT &two) {
  return (one.aLong != two);
}

BOOL inline operator > (const ADDR &one, const INT &two) {
  return (one.aLong > two);
}

BOOL inline operator >= (const ADDR &one, const INT &two) {
  return (one.aLong >= two);
}

BOOL inline operator < (const ADDR &one, const INT &two) {
  return (one.aLong < two);
}

BOOL inline operator <= (const ADDR &one, const INT &two) {
  return (one.aLong <= two);
}

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
  //  volatile HANDLE_LOCK          InUsedListProcess;

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
  //  void                          ResetCount();
  //  virtual void                  CheckEmptyList(void);

  //  void                          TestList(INT threadNumber, INT getNumber, INT fullTimes, BOOL wait=0);
#endif  // _TESTCOUNT
};

class CMemoryListCriticalSection : public CMemoryAlloc
{
private:
  volatile HANDLE_LOCK          InProcess;
  volatile HANDLE_LOCK          usedProcess;


  virtual INT                   GetOneList(ADDR &nlist);
  virtual INT                   FreeOneList(ADDR nlist);
  virtual INT                   AddToUsed(ADDR nlist);
public:
  CMemoryListCriticalSection();
};

int                             ThreadItem(void *para);

class CMemoryListLockFree: public CMemoryAlloc
{
private:
  virtual INT                   GetOneList(ADDR &nlist);
  virtual INT                   FreeOneList(ADDR nlist);
  virtual INT                   AddToUsed(ADDR nlist);
public:
  INT                   GetOneList2(ADDR &nlist);
  INT                   FreeOneList2(ADDR nlist);

};

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


#ifdef _TESTCOUNT
typedef struct threadPara
{
	CMemoryAlloc*					pMemoryList;
	MYINT							threadID;
	MYINT							getnumber;
	MYINT							fullnumber;
}threadPara, *pthreadPara;
void								TestListDemo(void);
#endif _TESTCOUNT
*/
