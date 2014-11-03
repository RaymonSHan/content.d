#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      
#include <strings.h>               
#include <sched.h>
#include <signal.h>
#include <errno.h>  
#include <fcntl.h> 
#include <sys/epoll.h>
#include <sys/wait.h>
#include "../include/raymoncommon.h"
#include "../include/rmemory.hpp"
#include "../include/epollpool.hpp"

#include "../include/testmain.hpp"

#define GETSIZE                 4
#define FREESIZE                4
#define MAXSIZE                 8

INT RpollThread::RThreadInit(void)
{
  __TRY
    pApp = ::GetApplication();
    contentMemory = pApp->ReturnContent();
    bufferMemory = pApp->ReturnBuffer();
    contentMemory->SetThreadArea(GETSIZE, MAXSIZE, FREESIZE, THREAD_FLAG_GETMEMORY);
    bufferMemory->SetThreadArea(GETSIZE, MAXSIZE, FREESIZE, THREAD_FLAG_GETMEMORY);
    RpollThreadInit();
  __CATCH
}

INT RpollThread::CreateRpollHandle(void)
{
  __TRY
    epollHandle = epoll_create(1);                              // size is ignored, greater than zero
  __CATCH
}

INT RpollAcceptThread::CreateListen(struct sockaddr &serveraddr)
{
  struct epoll_event ev;
  int listenfd, status;  

  __TRY
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    __DO_(listenfd == -1, "Error in create socket");
    __DO_(GetContent(listenAddr), "Error in getcontent");
    ev.data.fd = listenAddr.BHandle = listenfd;
    ev.events = EPOLLIN | EPOLLET;
    status = epoll_ctl(epollHandle, EPOLL_CTL_ADD, listenfd, &ev);
    __DO_(status == -1, "Error in epoll ctl");
    listenAddr.ServerSocket = serveraddr;
    status = bind(listenfd, &serveraddr, sizeof(sockaddr_in));
    __DO_(status == -1, "Error in bind");
  __CATCH
}

INT RpollAcceptThread::BeginListen(int query)
{
  int   status;
  __TRY
    status = listen(listenAddr.BHandle, query);
    __DO_(status == -1, "Error in begin listen");
  __CATCH
}

INT RpollAcceptThread::RpollThreadInit(void)
{
  __TRY
    __DO_(CreateRpollHandle(), "Error in CreatePolllHandless");
    CreateListen(pApp->ServerListen);
  __CATCH
}

INT RpollAcceptThread::RThreadDoing(void)
{
  int evNumber, i, clifd;
  socklen_t clilen;
  struct sockaddr_in cliaddr;
  struct epoll_event ev;

  __TRY
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
 __CATCH
}

INT RpollReadThread::RpollThreadInit(void)
{
  __TRY
    __DO_(CreateRpollHandle(), "Error in CreatePolllHandless");
  __CATCH
}

INT RpollReadThread::RThreadDoing(void)
{
  int evNumber, i;
  char line[1000];
  int readed;
  __TRY
    evNumber = epoll_wait(epollHandle, waitEv, MAX_EV_NUMBER, -1);
    for(i=0; i<evNumber; i++) {
      readed = read(waitEv->data.fd, line, 1000);
      printf("read %d\n", readed);
    }
  __CATCH
}

INT RpollWriteThread::RpollThreadInit(void)
{
  __TRY
  __CATCH
}

INT RpollWriteThread::RThreadDoing(void)
{
  __TRY
  __CATCH
}

INT RpollWorkThread::RpollThreadInit(void)
{
  __TRY
  __CATCH
}

INT RpollWorkThread::RThreadDoing(void)
{
  __TRY
  __CATCH
}

INT RpollScheduleThread::RpollThreadInit(void)
{
  int i;
  __TRY
    for (i=0; i<MAX_WORK_THREAD; i++)
      pApp->ScheduleWorkThread[i] = &(pApp->WorkThreadGroup[i]);
    for (i=0; i<MAX_RPOLL_ACCEPT_THREAD; i++)
      pApp->ScheduleAccept[i] = &(pApp->RpollAcceptGroup[i]);
    for (i=0; i<MAX_RPOLL_READ_THREAD; i++)
      pApp->ScheduleRead[i] = &(pApp->RpollReadGroup[i]);
    for (i=0; i<MAX_RPOLL_WRITE_THREAD; i++)
      pApp->ScheduleWrite[i] = &(pApp->RpollWriteGroup[i]);
  __CATCH
}

INT RpollScheduleThread::RThreadDoing(void)
{
  __TRY
  __CATCH
}

RpollGlobalApp::RpollGlobalApp()
{
}

INT RpollGlobalApp::InitRpollGlobalApp(void)
{
  __TRY
    __DO(ContentMemory.SetMemoryBuffer(NUMBER_CONTENT, sizeof(CContentItem), SIZE_CACHE, false));
    __DO(BufferMemory.SetMemoryBuffer(NUMBER_BUFFER, sizeof(CBufferItem), SIZE_NORMAL_PAGE, true));
  __CATCH
}

int RpollGlobalApp::StartRpoll(int flag, struct sockaddr serverlisten)
{
  int i;

  ServerListen = serverlisten;
  if (flag == RUN_WITH_CONSOLE) {
    for (i=0; i<MAX_RPOLL_ACCEPT_THREAD; i++) 
      { RpollAcceptGroup[i].RThreadClone(); }
    for (i=0; i<MAX_RPOLL_READ_THREAD; i++) 
      { RpollReadGroup[i].RThreadClone(); }
    for (i=0; i<MAX_RPOLL_WRITE_THREAD; i++) 
      { RpollWriteGroup[i].RThreadClone(); }
    { RScheduleGroup.RThreadClone(); }
__D
    RThread::RThreadCloneFinish();
    RpollAcceptGroup[0].BeginListen(10);
  }
  printf("OK\n");
  return 0;
}

/*
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
*/

