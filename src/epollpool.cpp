#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      
#include <strings.h>               
#include <sched.h>
//#include <signal.h>
#include <errno.h>  
#include <fcntl.h> 
#include <arpa/inet.h>  
#include <netinet/in.h> 
#include <sys/socket.h>  
#include <sys/epoll.h>
#include "../include/raymoncommon.h"

#define MAX_RPOLL_ACCEPT_THREAD         3
#define MAX_RPOLL_READ_THREAD           4
#define MAX_RPOLL_WRITE_THREAD          2
#define MAX_WORK_THREAD                 16

#define RPOLL_INIT_NULL                 0
#define RPOLL_INIT_START                1         // do epoll thingsSS
#define RPOLL_INIT_PREPARE              2         // do scheduleThread
#define RPOLL_INIT_OK                   (0x10)

#define ENOUGH_TIME_FOR_LISTEN          10000
#define MAX_EV_NUMBER                   20

#define RUN_WITH_CONSOLE                11
#define RUN_WITH_DAEMON                 12

class RBaseThread;
class RAcceptThread;

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
  inline pid_t ReturnWorkID(void) { return workId; };
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
protected:
  //  WorkFunc *RpollWorkFunc;
  struct epoll_event waitEv[MAX_EV_NUMBER];
  int epollHandle;
public:
  //  int SetEpollHandle(int handle);
  int AddEpollWait(int handle, int flag);
};

class RpollAcceptThread : public RpollThread {
public:
  virtual int RpollFunc(void);
};

class RpollReadThread : public RpollThread {
public:
  virtual int RpollFunc(void);
};

class RpollWriteThread : public RpollThread {
public:
  virtual int RpollFunc(void);
};

int RBaseThread::RpollClone(void *init)
{
  struct initStruct *place = (struct initStruct*)init;
  class RBaseThread *run = place->runThread;
  run->pApp = place->globalApp;
  run->workId = getpid();
  run->RpollFunc();
  return 0;
}

int RpollReadThread::RpollFunc(void)
{
  printf("in read, %d\n", ReturnWorkID());
  return 2;
}

int RpollWriteThread::RpollFunc(void)
{
  printf("in write, %d\n", ReturnWorkID());
  return 3;
}

class RpollGlobalApp {
private:
  int rpollInitSignal;
  int rpollNowAccept;
  int rpollNowRead;
  int rpollNowWrite;
  int workNow;

  //  class RScheduleThread RpollScheduleId;
  //  class RWorkThread WorkThreadGroup[MAX_WORK_THREAD];
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

  int aa;
public:
  RpollGlobalApp();
  int StartRpoll(int flag);             // start in daemon or not
  int GetRpollStatue(void) { return rpollInitSignal; };
  int SetRpollStatue(int oldstatue, int newstatue);
};

int RpollGlobalApp::SetRpollStatue(int oldstatue, int newstatue)
{
  return __sync_val_compare_and_swap
    (&rpollInitSignal, oldstatue, newstatue);
}

RpollGlobalApp::RpollGlobalApp()
{
  rpollInitSignal =							\
    rpollNowAccept = rpollNowRead = rpollNowWrite = workNow = 0;
}

#define CLONETHREAD(acts)						\
  childStack = getStack();						\
  init = (struct initStruct*)(childStack+STACK_SIZE-NORMAL_PAGE_SIZE);	\
  init->globalApp = this;						\
  init->runThread = &acts[i];						\
  clone(&(RBaseThread::RpollClone), childStack+STACK_SIZE,		\
	CLONE_VM | CLONE_FILES, init);
 
int RpollGlobalApp::StartRpoll(int flag)
{
  int i;
  char *childStack;
  struct initStruct *init;

  if (flag == RUN_WITH_CONSOLE) {
    for (i=0; i<MAX_RPOLL_ACCEPT_THREAD; i++) 
      { CLONETHREAD(RpollAcceptGroup) }
    for (i=0; i<MAX_RPOLL_READ_THREAD; i++) 
      { CLONETHREAD(RpollReadGroup) }
    for (i=0; i<MAX_RPOLL_WRITE_THREAD; i++) 
      { CLONETHREAD(RpollWriteGroup) }
  }
  sleep(1);
  printf("OK\n");
  return 0;
}

int RpollAcceptThread::RpollFunc(void)
{
  int rpollInit;
  printf("in accept\n");
  rpollInit = pApp->SetRpollStatue(RPOLL_INIT_NULL, RPOLL_INIT_START);
  if (rpollInit == RPOLL_INIT_NULL) {
    printf("in first thread, should do listen\n");
    epollHandle = 0xaabbccdd;
    rpollInit = pApp->SetRpollStatue(RPOLL_INIT_START, RPOLL_INIT_OK);
  }
  else {
    while (pApp->GetRpollStatue() != RPOLL_INIT_OK) usleep(ENOUGH_TIME_FOR_LISTEN);
    printf("in second thread, listen is ok\n");
  }
  return 0;
}


int main (int, char**)
{

  class RpollGlobalApp RpollApp;
  printf("main %x\n", RpollApp.GetRpollStatue());
  RpollApp.StartRpoll(RUN_WITH_CONSOLE);

  // the listen() add at last
  return (EXIT_SUCCESS);
}
