
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>  
#include <netinet/in.h>

#define MAX_RPOLL_ACCEPT_THREAD         3
#define MAX_RPOLL_READ_THREAD           4
#define MAX_RPOLL_WRITE_THREAD          2
#define MAX_WORK_THREAD                 16

#define MAX_LISTEN_QUERY                128

#define RPOLL_INIT_NULL                 0
#define RPOLL_INIT_START                1         // do epoll things
#define RPOLL_INIT_PREPARE              2
#define RPOLL_INIT_SCHEDULE             3
#define RPOLL_INIT_OK                   (0x10)

#define ENOUGH_TIME_FOR_LISTEN          10000
#define MAX_EV_NUMBER                   20

#define RUN_WITH_CONSOLE                0x0000
#define RUN_WITH_DAEMON                 0x0001

class RBaseThread;

typedef int WorkFunc(void*);
typedef int (RBaseThread::*ClassWorkFunc)(void);

struct initStruct {
  class RpollGlobalApp *globalApp;
  class RBaseThread *runThread;
};

class RBaseThread {
public:
  int *workSignalPointer;               // it point to next field workSignal normally
  static WorkFunc RpollClone;
  virtual int RpollFunc(void) = 0;
private:
  int workSignal;                       // schedule use piror pointer for share thread
  pid_t workId;
protected:
  class RpollGlobalApp *pApp;

public:
  inline pid_t ReturnWorkId(void) { return workId; };
  int InitRWorkThread(class RBaseThread *from);
};

class RScheduleThread : public RBaseThread {
public:
  virtual int RpollFunc(void);
 
  //  static WorkFunc ScheduleCloneFunc;
};

class RWorkThread : public RBaseThread {
public:
  virtual int RpollFunc(void);
  //  static WorkFunc WorkCloneFunc;
};

class RpollThread : public RBaseThread {
public:
  virtual int RpollFunc(void) = 0;
public:
  struct epoll_event waitEv[MAX_EV_NUMBER];
  int epollHandle;
public:
  int AddRpollHandle(void);
  int AddRpollWait(int handle, int flag);
};

class RpollAcceptThread : public RpollThread {
public:
  virtual int RpollFunc(void);
  int CreateListen(struct sockaddr_in *serveraddr);
private:
};

class RpollReadThread : public RpollThread {
public:
  virtual int RpollFunc(void);
};

class RpollWriteThread : public RpollThread {
public:
  virtual int RpollFunc(void);
};

class RpollGlobalApp {
private:
  int rpollInitSignal;
  int rpollNowAccept;
  int rpollNowRead;
  int rpollNowWrite;
  int workNow;

public:
  class RScheduleThread RScheduleGroup;
  class RWorkThread WorkThreadGroup[MAX_WORK_THREAD];
  class RpollAcceptThread RpollAcceptGroup[MAX_RPOLL_ACCEPT_THREAD];
  class RpollReadThread RpollReadGroup[MAX_RPOLL_READ_THREAD];
  class RpollWriteThread RpollWriteGroup[MAX_RPOLL_WRITE_THREAD];

  // Schedule map the above to below
  int rpollScheduleAccept;
  int rpollScheduleRead;
  int rpollScheduleWrite;
  int workSche;

  class RWorkThread *ScheduleWorkThread[MAX_WORK_THREAD];
  class RpollThread *ScheduleAccept[MAX_RPOLL_ACCEPT_THREAD];
  class RpollThread *ScheduleRead[MAX_RPOLL_READ_THREAD];
  class RpollThread *ScheduleWrite[MAX_RPOLL_WRITE_THREAD];

public:
  struct sockaddr_in ServerAddr;
  int ServerListen;

  RpollGlobalApp();
  int StartRpoll(int flag, struct sockaddr_in *serveraddr);   // start in daemon or not
  int GetRpollStatue(void) { return rpollInitSignal; };
  int SetRpollStatue(int oldstatue, int newstatue);
};
