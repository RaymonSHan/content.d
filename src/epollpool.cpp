// epollpool.cpp

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
    setClassName();
    pApp = ::GetApplication();
    contentMemory = pApp->ReturnContent();
    bufferMemory = pApp->ReturnBuffer();
    __DO (contentMemory->SetThreadArea(GETSIZE, MAXSIZE, FREESIZE, THREAD_FLAG_GETMEMORY));
    __DO (bufferMemory->SetThreadArea(GETSIZE, MAXSIZE, FREESIZE, THREAD_FLAG_GETMEMORY));
    __DO (CreateRpollHandle());
    __DO (RpollThreadInit());
  __CATCH
}

INT RpollThread::CreateRpollHandle(void)
{
  __TRY
    // size is ignored, greater than zero
    __DO1_(epollHandle, epoll_create(1), "Error in create epoll");
  __CATCH
}

INT RpollAcceptThread::CreateListen(struct sockaddr &serveraddr)
{
  struct epoll_event ev;
  int listenfd, status;  

  __TRY
    __DO1_(listenfd, 
	   socket(AF_INET, SOCK_STREAM, 0), 
	   "Error in create socket");
    __DO_ (GetContent(listenAddr), 
	   "Error in getcontent");

    ev.data.fd = listenAddr.BHandle = listenfd;
    ev.events = EPOLLIN | EPOLLET;
    __DO1_(status, 
	   epoll_ctl(epollHandle, EPOLL_CTL_ADD, listenfd, &ev), 
	   "Error in epoll ctl");
    listenAddr.ServerSocket.saddr = serveraddr;
    __DO1_(status, 
	   bind(listenfd, &serveraddr, sizeof(sockaddr_in)), 
	   "Error in bind");
  __CATCH
}

INT RpollAcceptThread::BeginListen(int query)
{
  int   status;
  __TRY
    __DO1_(status, 
	   listen(listenAddr.BHandle, query), 
	   "Error in begin listen");
  __CATCH
}

INT RpollAcceptThread::RpollThreadInit(void)
{
  __TRY
    __DO (CreateListen(pApp->ServerListen));
  __CATCH
}

INT RpollAcceptThread::RThreadDoing(void)
{
  int evNumber, i, clifd;
  socklen_t clilen;
  struct sockaddr_in cliaddr;
  struct epoll_event ev;

  __TRY
    __DO1_(evNumber, 
	   epoll_wait(epollHandle, waitEv, MAX_EV_NUMBER, 1000*100), 
	   "Error in epoll wait");
    for(i=0; i<evNumber; i++) {
      __DO1_(clifd, 
	     accept(waitEv->data.fd, (sockaddr*)&cliaddr, &clilen), 
	     "Error in accept");
      ev.data.fd = clifd;
      ev.events = EPOLLIN | EPOLLET;
      __DO_ (epoll_ctl(pApp->ScheduleRead[0]->epollHandle, EPOLL_CTL_ADD, clifd, &ev), 
	     "Error in epoll ctl");
    }
 __CATCH
}

INT RpollReadThread::RpollThreadInit(void)
{
  __TRY
    __DO(CreateRpollHandle());
  __CATCH
}

INT RpollReadThread::RThreadDoing(void)
{
  int evNumber, i;
  char line[1000];
  int readed;
  __TRY
    __DO1_(evNumber, 
	   epoll_wait(epollHandle, waitEv, MAX_EV_NUMBER, 1000*100), 
	   "Error in epoll wait");
    for(i = 0; i < evNumber; i++) {
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
    sleep(1);

  __CATCH
}

INT RpollWorkThread::RpollThreadInit(void)
{
  __TRY
  __CATCH
}

INT RpollWorkThread::RThreadDoing(void)
{
  ADDR buff;

  __TRY
    __DO_(eventFd->EventRead(buff), "error reading");
  //    printf("in work, %p, %lld\n", &buff, buff.aLong);
  __CATCH
}

INT RpollScheduleThread::RpollThreadInit(void)
{
  int i;
  __TRY
    for (i=0; i<MAX_WORK_THREAD; i++)
      pApp->ScheduleWork[i] = &(pApp->RpollWorkGroup[i]);
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
    contentMemory->DisplayFree();
    sleep(2);
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

INT RpollGlobalApp::StartRpoll(int flag, struct sockaddr serverlisten)
{
  int i;

  __TRY
    ServerListen = serverlisten;
    if (flag == RUN_WITH_CONSOLE) {
      for (i=0; i<MAX_RPOLL_ACCEPT_THREAD; i++) 
	__DO(RpollAcceptGroup[i].RThreadClone());
      for (i=0; i<MAX_RPOLL_READ_THREAD; i++) 
        __DO(RpollReadGroup[i].RThreadClone());
      for (i=0; i<MAX_RPOLL_WRITE_THREAD; i++) 
        __DO(RpollWriteGroup[i].RThreadClone());

      for (i=0; i<MAX_WORK_THREAD; i++) 
        __DO(RpollWorkGroup[i].RThreadClone());

      if (MAX_SCHEDULE_THREAD) __DO(RScheduleGroup.RThreadClone());

      RThread::RThreadWaitInit();
      if (MAX_RPOLL_ACCEPT_THREAD) __DO(RpollAcceptGroup[0].BeginListen(10));
      RThread::RThreadCloneFinish();
    }
    printf("OK\n");
  __CATCH
}

INT RpollGlobalApp::KillAllChild(void)
{
  static int havedone = 0;
  int i;
  if (havedone) return 0;
  else havedone = 1;
  __TRY
    for (i=0; i<MAX_RPOLL_ACCEPT_THREAD; i++)
      kill(RpollAcceptGroup[i].ReturnWorkId(), SIGTERM);
    for (i=0; i<MAX_RPOLL_READ_THREAD; i++)
      kill(RpollReadGroup[i].ReturnWorkId(), SIGTERM);
    for (i=0; i<MAX_RPOLL_WRITE_THREAD; i++)
      kill(RpollWriteGroup[i].ReturnWorkId(), SIGTERM);
    for (i=0; i<MAX_WORK_THREAD; i++)
      kill(RpollWorkGroup[i].ReturnWorkId(), SIGTERM);

    kill(RScheduleGroup.ReturnWorkId(), SIGTERM);

  __CATCH
}

