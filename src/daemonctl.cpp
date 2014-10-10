
//#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
//#include <getopt.h>
#include <signal.h>  
#include <sys/param.h>  
#include <sys/types.h>  
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h> 
#include <sys/mman.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <limits.h>    // for NAME_MAX is 255
#include <signal.h>
#include <utmp.h> 
#include <pwd.h>
#include <sched.h>
//#include "../include/daemonctl.h"

union assignAddress
{
  long long int addressLong;
  char *addressChar;
  void *addressVoid;
  long long int *addressLPointer;
};

#define NORMAL_SEGMENT_START (0x52LL << 40)  // 'R'
#define STACK_START (0x53LL << 40)           // 'S'
#define STACK_SIZE  (16*1024*1024)
#define NORMAL_PAGE_SIZE     (1LL << 12)

void perrorexit(const char* s)
{
  perror(s);
  exit(-1);
}

char* getStack(void)
{
  static union assignAddress totalStackStart = {STACK_START};

  union assignAddress nowstack, realstack;
  nowstack.addressLong = __sync_fetch_and_add(&(totalStackStart.addressLong), STACK_SIZE);
  realstack.addressVoid = mmap (nowstack.addressVoid, STACK_SIZE, PROT_READ | PROT_WRITE, 
				MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED | MAP_GROWSDOWN, -1, 0);
  if (realstack.addressVoid == MAP_FAILED ) {
    printf("error in mmap%llx, %llx\n", nowstack.addressLong, realstack.addressLong);
    perrorexit("adsf");
    return 0;
  }
  return realstack.addressChar;
}

int mmapUTMP(int utmpfile, utmp **utmp_buf)
{
  union assignAddress utmpStart;

  if (!utmpfile) utmpfile = open(UTMP_FILE, O_RDONLY);
  utmpStart.addressVoid = mmap (0, NORMAL_PAGE_SIZE*100, 
				PROT_READ, MAP_SHARED, utmpfile, 0);
  if (utmpStart.addressVoid == MAP_FAILED) perrorexit("Error in mmap file");

  *utmp_buf = static_cast<utmp*>(utmpStart.addressVoid);
  return utmpfile;
}

int entriesUTMP(int utmpfile)
{
  struct stat file_stats;
  size_t size;

  fstat (utmpfile, &file_stats);
  size = file_stats.st_size;
  return( size / sizeof (struct utmp)); 
}

#define DEV_DIR_WITH_TRAILING_SLASH "/dev/"                                     
#define DEV_DIR_LEN (sizeof (DEV_DIR_WITH_TRAILING_SLASH) - 1) 
#define IS_ABSOLUTE_FILE_NAME(p) (*p == '/')

#define ESC_SAVE_CURSOR "\e[s"
#define ESC_RESTORE_CURSOR "\e[u"
#define ESC_SET_CURSOR "\e[1;48f"
#define ESC_RESTORE_COLOR "\e[0;37m"
#define ESC_SET_COLOR "\e[1;36m"

#define ESC_WRITE(str, mess) snprintf(str, sizeof(str), "%s%s%s%s%s%s", \
				      ESC_SAVE_CURSOR, ESC_SET_CURSOR, ESC_SET_COLOR, \
				      mess, ESC_RESTORE_COLOR, ESC_RESTORE_CURSOR)
void getScreenText(char **txt)
{
  time_t rtmtime;

  time(&rtmtime);
  *txt =  ctime(&rtmtime);
}

#define MAX_PTS 1024
int openedPTS[MAX_PTS];
int nowPTS = 0;

void writeToPTS(char *, char *str) 
{
  char screenstr[256];
  int result;

  if (nowPTS) {
    ESC_WRITE(screenstr, str);
    int len = strlen(screenstr);
    for (int i; i<nowPTS; i++) {
      result = write(openedPTS[i], screenstr, len);
    }
  }
  if (result) return;    // only for remove warning
}

void ReflushPTS(utmp *utmpbuf, int utmphandle, passwd *pw)
{
  int nowe;
  utmp *nowu;
  char line[sizeof (utmpbuf->ut_line) + DEV_DIR_LEN + 1];                      
  char *p = line, *modelstr;
  int lastPTS;

  lastPTS = nowPTS;
  nowPTS = 0;
  for (int i=0; i<lastPTS; i++) {
    close(openedPTS[i]);
  }
  nowe = entriesUTMP(utmphandle);
  nowu = utmpbuf;

  while (nowe--) {
    if (nowu->ut_type == USER_PROCESS) {
      if ( !strncmp(pw->pw_name, nowu->ut_user, sizeof(nowu->ut_user))) {

	if ( ! IS_ABSOLUTE_FILE_NAME (nowu->ut_line))                             
	  p = strncpy (p, DEV_DIR_WITH_TRAILING_SLASH, sizeof(DEV_DIR_WITH_TRAILING_SLASH));
	else
	  *p = 0;
	strncat (p, nowu->ut_line, sizeof(nowu->ut_line));
	openedPTS[nowPTS] = open(p, O_RDWR);
	nowPTS++;
      }
    }
    nowu++;
  }
  //  getScreenText(&modelstr);
  //  writeToPTS(p, modelstr);  
}

#define BUF_LEN (40 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int inotifyUTMP(void*)
{
  int inotifyFd, wd;
  char buf[BUF_LEN] __attribute__ ((aligned(8)));
  ssize_t numRead;
  char *p;
  struct inotify_event *event;

  struct passwd *pw;                                                         
  uid_t uid;                                                                 
  uid_t NO_UID = -1; 
  utmp *utmpbuf;
  int utmphandle; 

  errno = 0;                                                                 
  uid = geteuid ();                                                          
  pw = (uid == NO_UID && errno ? NULL : getpwuid (uid));
  if (!pw) perrorexit("cannot find user ID");
  utmphandle = mmapUTMP(0, &utmpbuf);

  inotifyFd = inotify_init();                 /* Create inotify instance */
  if (inotifyFd == -1) perrorexit("inotify_init");

  wd = inotify_add_watch(inotifyFd, UTMP_FILE, IN_MODIFY);
  if (wd == -1) perrorexit("inotify_add_watch");

  ReflushPTS(utmpbuf, utmphandle, pw);       // first start

  for (;;) {                                  /* Read events forever */
    numRead = read(inotifyFd, buf, BUF_LEN);
    if (numRead == 0) perror("read() from inotify fd returned 0!");

    if (numRead == -1) perrorexit("read");
    for (p = buf; p < buf + numRead; ) {
      event = (struct inotify_event *) p;
      if (event->mask & IN_MODIFY) {
	ReflushPTS(utmpbuf, utmphandle, pw);
	//	printf("IN_MODIFY\n");
      }
      p += sizeof(struct inotify_event) + event->len;
    }
  }
  exit(EXIT_SUCCESS);
}

pid_t Inotefythread, Writethread;

void getSign(int)
{
  if (Inotefythread) kill(Inotefythread, SIGTERM);
  if (Writethread) kill(Writethread, SIGTERM);
}

int ptsClock(void*)
{
  char *modelstr;
  for(;;) {
    getScreenText(&modelstr);
    writeToPTS(0, modelstr);
    usleep(250*1000);
  }
  return 0;
}

void threadPTSClock(void)
{
  int status;
  pid_t retthread;

  Inotefythread = clone(&inotifyUTMP, getStack()+STACK_SIZE, CLONE_VM | CLONE_FILES, NULL);
  Writethread = clone(&ptsClock, getStack()+STACK_SIZE, CLONE_VM | CLONE_FILES, NULL);

  if (signal(SIGTERM, getSign) == SIG_ERR) exit(1);

  retthread = waitpid(-1, &status, __WCLONE);
  if (Inotefythread == retthread) {
    if (Writethread) kill(Writethread, SIGTERM);
  }
  if (Writethread == retthread) {
    if (Inotefythread) kill(Inotefythread, SIGTERM);
  }
  return; 
}

int initDaemon(void)
{
  int pidFirst, pidSecond;  
  int i;  

  // the first child thread, for relese console
  pidFirst = fork();
  if (pidFirst > 0) exit(0);                      // is parent thread, exit
  else if (pidFirst < 0) exit(1);                 // error in fork
  setsid();                                       // first child be leader  

  // the second child thread, for could not get console again
  pidSecond = fork();
  if (pidSecond > 0) exit(0); 
                    // second thread could not get console
  else if (pidSecond < 0) exit(1);                // the second never be leader

  for (i=0; i < NOFILE; ++i) close(i);            // close all handle from parent
  
  if (chdir("/tmp")) exit(1);                     // change working dirctory to /tmp  
  umask(0);                                       // remove file mask

  threadPTSClock();                               // Real work

  return 0;
}



int main(int , char **)
{
  initDaemon();
  return 0;
}

