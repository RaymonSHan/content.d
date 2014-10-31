#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      
#include <strings.h>               
#include <sched.h>
#include <signal.h>
#include <errno.h>  
#include <fcntl.h> 
#include <arpa/inet.h>  
#include <netinet/in.h> 
#include <sys/socket.h>  
#include <sys/epoll.h>
#include <sys/wait.h>
#include "../include/raymoncommon.h"
#include "../include/rmemory.hpp"
#include "../include/epollpool.hpp"

int RBaseThread::RpollClone(void *init)
{
  struct initStruct *place = (struct initStruct*)init;
  class RBaseThread *run = place->runThread;
  run->pApp = place->globalApp;
  run->workId = getpid();
  run->workSignal = 0;
  run->workSignalPointer = &run->workSignal;
  run->RpollFunc();
  return 0;
}

int RpollWriteThread::RpollFunc(void)
{
  //  printf("in write, %d\n", ReturnWorkID());
  sleep(1000);
  return 3;
}

int RWorkThread::RpollFunc(void)
{
  //  printf("in work, %d\n", ReturnWorkID());
  sleep(1000);
  return 3;
}

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
  cStack = getStack();							\
  init = (struct initStruct*)						\
    (cStack.aLong + SIZE_THREAD_STACK - SIZE_NORMAL_PAGE);		\
  init->globalApp = this;						\
  init->runThread = &acts;						\
  clone(&(RBaseThread::RpollClone), cStack.pChar + SIZE_THREAD_STACK,	\
	CLONE_VM | CLONE_FILES, init);

#define WAITFORSIGNAL(pApp, mode)					\
  while (pApp->GetRpollStatue() != mode) usleep(ENOUGH_TIME_FOR_LISTEN);

#define SETSIGNAL(pApp, oldmode, newmode)			\
  pApp->SetRpollStatue(oldmode, newmode);			\
  if (pApp->GetRpollStatue() != newmode) exit(-1);

int RpollGlobalApp::StartRpoll(int flag, struct sockaddr_in *serveraddr)
{
  int i;
  ADDR cStack;
  struct initStruct *init;

  ServerAddr = *serveraddr;

  if (flag == RUN_WITH_CONSOLE) {
    for (i=0; i<MAX_RPOLL_READ_THREAD; i++) 
      { CLONETHREAD(RpollReadGroup[i]) }
    for (i=0; i<MAX_RPOLL_WRITE_THREAD; i++) 
      { CLONETHREAD(RpollWriteGroup[i]) }
    for (i=0; i<MAX_RPOLL_ACCEPT_THREAD; i++) 
      { CLONETHREAD(RpollAcceptGroup[i]) }
    { CLONETHREAD(RScheduleGroup) }

    WAITFORSIGNAL(this, RPOLL_INIT_OK);
    listen(ServerListen, MAX_LISTEN_QUERY);
  }
  printf("OK\n");
  return 0;
}

int RpollThread::AddRpollHandle(void)
{
  epollHandle = epoll_create(1);        // size is ignored, greater than zero
  return epollHandle;
}

int RpollAcceptThread::RpollFunc(void)
{
  int rpollInit;
  AddRpollHandle();
  rpollInit = pApp->SetRpollStatue(RPOLL_INIT_NULL, RPOLL_INIT_START);
  if (rpollInit == RPOLL_INIT_NULL) {
    if (epollHandle == -1) perrorexit("epoll_create error");
    CreateListen(&pApp->ServerAddr);
    SETSIGNAL(pApp, RPOLL_INIT_START, RPOLL_INIT_PREPARE);
  }

  WAITFORSIGNAL(pApp, RPOLL_INIT_SCHEDULE);
  SETSIGNAL(pApp, RPOLL_INIT_SCHEDULE, RPOLL_INIT_OK);
  int evNumber, i, clifd;
  socklen_t clilen;
  struct sockaddr_in cliaddr;
  struct epoll_event ev;
  for(;;) {
    evNumber = epoll_wait(epollHandle, waitEv, MAX_EV_NUMBER, -1);
    for(i=0; i<evNumber; i++) {
      printf("in accept\n");
      clifd = accept(waitEv->data.fd, (sockaddr*)&cliaddr, &clilen);
      ev.data.fd = clifd;
      ev.events = EPOLLIN | EPOLLET;
      epoll_ctl(pApp->ScheduleRead[0]->epollHandle, EPOLL_CTL_ADD, clifd, &ev);
      printf("after accept\n");
    }
  }
  return 0;
}

int RpollReadThread::RpollFunc(void)
{
  int evNumber, i;
  AddRpollHandle();
  WAITFORSIGNAL(pApp, RPOLL_INIT_SCHEDULE);
  char line[1000];
  int readed;
  for(;;) {
    evNumber = epoll_wait(epollHandle, waitEv, MAX_EV_NUMBER, -1);
    for(i=0; i<evNumber; i++) {
      readed = read(waitEv->data.fd, line, 1000);
      printf("read %d\n", readed);
    }
  }
  //  printf("in read, %d\n", ReturnWorkID());
  return 2;
}

int RpollAcceptThread::CreateListen(struct sockaddr_in *serveraddr)
{
  struct epoll_event ev;
  int listenfd;  

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  pApp->ServerListen = listenfd;
  ev.data.fd = pApp->ServerListen;
  ev.events = EPOLLIN | EPOLLET;
  epoll_ctl(epollHandle, EPOLL_CTL_ADD, listenfd, &ev);
  bind(listenfd,(sockaddr*)serveraddr, sizeof(sockaddr_in));
  printf("after bind\n");
  return 0;
}

int RScheduleThread::RpollFunc(void)
{
  int i;

  WAITFORSIGNAL(pApp, RPOLL_INIT_PREPARE);

  for (i=0; i<MAX_WORK_THREAD; i++)
    pApp->ScheduleWorkThread[i] = &(pApp->WorkThreadGroup[i]);
  for (i=0; i<MAX_RPOLL_ACCEPT_THREAD; i++)
    pApp->ScheduleAccept[i] = &(pApp->RpollAcceptGroup[i]);
  for (i=0; i<MAX_RPOLL_READ_THREAD; i++)
    pApp->ScheduleRead[i] = &(pApp->RpollReadGroup[i]);
  for (i=0; i<MAX_RPOLL_WRITE_THREAD; i++)
    pApp->ScheduleWrite[i] = &(pApp->RpollWriteGroup[i]);

  SETSIGNAL(pApp, RPOLL_INIT_PREPARE, RPOLL_INIT_SCHEDULE);
  sleep(1000);
  return 0;
}

class RpollGlobalApp RpollApp;

void killAllChild(int)
{
  static int havedone= 0;
  int i;
  if (havedone) return;
  else havedone = 1;
  for (i=0; i<MAX_WORK_THREAD; i++)
    kill(RpollApp.WorkThreadGroup[i].ReturnWorkId(), SIGTERM);
  for (i=0; i<MAX_RPOLL_ACCEPT_THREAD; i++)
    kill(RpollApp.RpollAcceptGroup[i].ReturnWorkId(), SIGTERM);
  for (i=0; i<MAX_RPOLL_READ_THREAD; i++)
    kill(RpollApp.RpollReadGroup[i].ReturnWorkId(), SIGTERM);
  for (i=0; i<MAX_RPOLL_WRITE_THREAD; i++)
    kill(RpollApp.RpollWriteGroup[i].ReturnWorkId(), SIGTERM);
}

/*
int main (int, char**)
{
  struct sockaddr_in addr;
  char local_addr[] = "127.0.0.1";    
  bzero(&addr, sizeof(sockaddr_in));   
  addr.sin_family = AF_INET; 
  inet_aton(local_addr,&(addr.sin_addr));
  addr.sin_port=htons(8998);  
  RpollApp.StartRpoll(RUN_WITH_CONSOLE, &addr);

  int status;
  pid_t retthread;

  if (signal(SIGTERM, killAllChild) == SIG_ERR) exit(1);

  retthread = waitpid(-1, &status, __WCLONE);
  printf("after wait %d\n", retthread);
  killAllChild(0);
  printf("after kill\n");
  // the listen() add at last
  return (EXIT_SUCCESS);
}
*/
