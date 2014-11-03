
#ifndef INCLUDE_EPOLLPOOL_HPP
#define INCLUDE_EPOLLPOOL_HPP

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>  
#include <netinet/in.h>

#include "rmemory.hpp"
#include "rthread.hpp"

#define MAX_RPOLL_ACCEPT_THREAD         1
#define MAX_RPOLL_READ_THREAD           1
#define MAX_RPOLL_WRITE_THREAD          1
#define MAX_WORK_THREAD                 1

#define MAX_LISTEN_QUERY                128

#define MAX_EV_NUMBER                   20

#define RUN_WITH_CONSOLE                0x0000
#define RUN_WITH_DAEMON                 0x0001

class   RpollGlobalApp;

class RpollThread : public RThread {
private:
  CMemoryAlloc *contentMemory;
  CMemoryAlloc *bufferMemory;

protected:
  RpollGlobalApp* pApp;
  struct epoll_event waitEv[MAX_EV_NUMBER];

public:
  int epollHandle;

protected:
  virtual INT RThreadInit(void);
  virtual INT RpollThreadInit(void) = 0;
  virtual INT RThreadDoing(void) = 0;
  inline INT GetContent(ADDR &item) { return contentMemory->GetMemoryList(item); };
  inline INT GetBuffer(ADDR &item) { return bufferMemory->GetMemoryList(item); };
  inline INT FreeContent(ADDR item) { return contentMemory->FreeMemoryList(item); };
  inline INT FreeBuffer(ADDR item) { return bufferMemory->FreeMemoryList(item); };

public:
  INT CreateRpollHandle(void);
};

class RpollAcceptThread : public RpollThread {
private:
  ADDR  listenAddr;
  virtual INT RpollThreadInit(void);
  virtual INT RThreadDoing(void);
public:
  INT CreateListen(struct sockaddr &serveraddr);
  INT BeginListen(int query);
};

class RpollScheduleThread : public RpollThread {
private:
  virtual INT RpollThreadInit(void);
  virtual INT RThreadDoing(void);
};

class RpollWorkThread : public RpollThread {
private:
  virtual INT RpollThreadInit(void);
  virtual INT RThreadDoing(void);
};

class RpollReadThread : public RpollThread {
private:
  virtual INT RpollThreadInit(void);
  virtual INT RThreadDoing(void);
};

class RpollWriteThread : public RpollThread {
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
  class RpollWorkThread WorkThreadGroup[MAX_WORK_THREAD];
  class RpollAcceptThread RpollAcceptGroup[MAX_RPOLL_ACCEPT_THREAD];
  class RpollReadThread RpollReadGroup[MAX_RPOLL_READ_THREAD];
  class RpollWriteThread RpollWriteGroup[MAX_RPOLL_WRITE_THREAD];

  // Schedule map the above to below
  int rpollScheduleAccept;
  int rpollScheduleRead;
  int rpollScheduleWrite;
  int workSche;

public:
  class RpollWorkThread *ScheduleWorkThread[MAX_WORK_THREAD];
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
