
#ifndef INCLUDE_EPOLLPOOL_HPP
#define INCLUDE_EPOLLPOOL_HPP

#include <sys/types.h>

#include "rmemory.hpp"
#include "application.hpp"
#include "rthread.hpp"

#define MAX_RPOLL_ACCEPT_THREAD         1
#define MAX_RPOLL_READ_THREAD           1
#define MAX_RPOLL_WRITE_THREAD          1
#define MAX_WORK_THREAD                 1
#define MAX_SCHEDULE_THREAD             1

#define MAX_LISTEN_QUERY                128

#define MAX_EV_NUMBER                   20

#define RUN_WITH_CONSOLE                0x0000
#define RUN_WITH_DAEMON                 0x0001

class   RpollGlobalApp;

__class_(RpollThread, RThread)
public:
  CMemoryAlloc *contentMemory;
  CMemoryAlloc *bufferMemory;
protected:
  RMultiEvent *firstEvent;
  RMultiEvent *waitEvent;
  RpollGlobalApp* pApp;
  struct epoll_event waitEv[MAX_EV_NUMBER];

public:
  int epollHandle;

protected:
  virtual INT RThreadInit(void);
  virtual INT RpollThreadInit(void) = 0;
  virtual INT RThreadDoing(void) = 0;
  inline INT GetContent(ADDR &item) { 
    INT ret = contentMemory->GetMemoryList(item);
    if (ret) return ret;                                        // error, not get
    bzero(item.pVoid, sizeof(CContentItem));
    return ret;
  };
  inline INT GetBuffer(ADDR &item) {
    INT ret = bufferMemory->GetMemoryList(item);
    if (ret) return ret;                                        // error, not get
    item.NSize = item.NOper = 0;
    item.RealStart = item.BufferData;
    item.NextBuffer = 0;
    return ret;
  };
  inline INT FreeContent(ADDR item) {
    ADDR nextitem, thisitem;
    if (item.CHandle) close(item.CHandle);
    if (item.SHandle) close(item.SHandle);
    if (item.MoreContent != 0) FreeContent(item.MoreContent);
    if (item.NowBuffer != 0) FreeBuffer(item.NowBuffer);
    if (item.MoreBuffer != 0) FreeBuffer(item.MoreBuffer);
    if (item.OtherBuffer != 0) {
      nextitem = item.OtherBuffer;
      do {
	thisitem = nextitem;
	nextitem = thisitem.NextBuffer;
	FreeBuffer(thisitem);
      } while (nextitem != 0);
    }
    return contentMemory->FreeMemoryList(item); 
  };
  inline INT FreeBuffer(ADDR item) {
    return bufferMemory->FreeMemoryList(item); 
  };
  INT SendToNextThread(ADDR item);

public:
  RpollThread();
  INT CreateRpollHandle(void);
  inline void AttachEvent(RMultiEvent *event)                   // for SendToNextThread
    { firstEvent = event; };
  inline void SetWaitfd(RMultiEvent *fd)                        // for Wait
    { waitEvent = fd; };
};

__class_(RpollAcceptThread, RpollThread)
private:
  ADDR  listenAddr;
  virtual INT RpollThreadInit(void);
  virtual INT RThreadDoing(void);
public:
  INT CreateListen(struct sockaddr &serveraddr);
  INT BeginListen(int query);
};

__class_(RpollScheduleThread, RpollThread)
private:
  virtual INT RpollThreadInit(void);
  virtual INT RThreadDoing(void);
};

__class_(RpollWorkThread, RpollThread)
private:
  virtual INT RpollThreadInit(void);                    // called after clone
  virtual INT RThreadDoing(void);
public:
  RpollWorkThread();
  INT   RpollWorkThreadInit(void);                      // called in main, before clone
};

__class_(RpollReadThread, RpollThread)
private:
  virtual INT RpollThreadInit(void);
  virtual INT RThreadDoing(void);
};

__class_(RpollWriteThread, RpollThread)
private:
  virtual INT RpollThreadInit(void);
  virtual INT RThreadDoing(void);
};

class RpollGlobalApp {
private:
  int rpollInitSignal;
  int rpollNowAccept;
  int rpollNowRead;
  int rpollNowWrite;
  int workNow;

public:
  class RpollScheduleThread RScheduleGroup;
  class RpollWorkThread RpollWorkGroup[MAX_WORK_THREAD];
  class RpollAcceptThread RpollAcceptGroup[MAX_RPOLL_ACCEPT_THREAD];
  class RpollReadThread RpollReadGroup[MAX_RPOLL_READ_THREAD];
  class RpollWriteThread RpollWriteGroup[MAX_RPOLL_WRITE_THREAD];

  // Schedule map the above to below
  int rpollScheduleAccept;
  int rpollScheduleRead;
  int rpollScheduleWrite;
  int workSche;

public:
  class RpollWorkThread *ScheduleWork[MAX_WORK_THREAD];
  class RpollAcceptThread *ScheduleAccept[MAX_RPOLL_ACCEPT_THREAD];
  class RpollReadThread *ScheduleRead[MAX_RPOLL_READ_THREAD];
  class RpollWriteThread *ScheduleWrite[MAX_RPOLL_WRITE_THREAD];

private:
  CMemoryAlloc ContentMemory;
  CMemoryAlloc BufferMemory;

public:
  CMemoryAlloc* ReturnContent(void) { return &ContentMemory; };
  CMemoryAlloc* ReturnBuffer(void) { return &BufferMemory; };

public:
  struct sockaddr ServerListen;

  RpollGlobalApp();
  INT InitRpollGlobalApp(void);
  INT StartRpoll(int flag, struct sockaddr serverlisten);   // start in daemon or not
  INT KillAllChild(void);

};

#endif  // INCLUDE_EPOLLPOOL_HPP
