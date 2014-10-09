
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
#include <fcntl.h>
#include <time.h> 
#include <sys/mman.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <limits.h>    // for NAME_MAX is 255
#include <signal.h>
#include <utmp.h> 
#include <pwd.h>
//#include "../include/daemonctl.h"

union assignAddress
{
  long long int addressLong;
  char *addressChar;
  void *addressVoid;
  long long int *addressLPointer;
};
#define NORMAL_SEGMENT_START (0x52LL << 40)  // 'R'
#define NORMAL_PAGE_SIZE     (1LL << 12)

union assignAddress realStart;
long utmpsize = 0;

void perrorexit(const char* s)
{
  perror(s);
  exit(-1);
}

int mmapUTMP(int utmpfile, utmp **utmp_buf)
{
  if (!utmpfile) utmpfile = open(UTMP_FILE, O_RDONLY);
  realStart.addressVoid = mmap (0, NORMAL_PAGE_SIZE*100, 
				PROT_READ, MAP_SHARED, utmpfile, 0);
  if (realStart.addressVoid == MAP_FAILED) perrorexit("Error in mmap file");

  *utmp_buf = static_cast<utmp*>(realStart.addressVoid);
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

void writeToPTS(char *pts, char *str)
{
  int fd, result;
  char screenstr[256];

  fd = open(pts, O_RDWR);
  if (fd == -1) perrorexit("Error opening file for writing");

  ESC_WRITE(screenstr, str);
  result =  write(fd, screenstr, strlen(screenstr));
  if (result == -1) perrorexit("Error calling write()"); 
  fdatasync(fd);
  close(fd);
}

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int inotifyUTMP(char *filename)
{
  int inotifyFd, wd;
  char buf[BUF_LEN] __attribute__ ((aligned(8)));
  ssize_t numRead;
  char *p;
  struct inotify_event *event;

  inotifyFd = inotify_init();                 /* Create inotify instance */
  if (inotifyFd == -1) perrorexit("inotify_init");

  wd = inotify_add_watch(inotifyFd, filename, IN_MODIFY);
  if (wd == -1) perrorexit("inotify_add_watch");

  for (;;) {                                  /* Read events forever */
    numRead = read(inotifyFd, buf, BUF_LEN);
    if (numRead == 0) perror("read() from inotify fd returned 0!");

    if (numRead == -1) perrorexit("read");
    for (p = buf; p < buf + numRead; ) {
      event = (struct inotify_event *) p;
      if (event->mask & IN_MODIFY)        printf("IN_MODIFY\n ");
      p += sizeof(struct inotify_event) + event->len;
    }
  }
  exit(EXIT_SUCCESS);
}

void ptsClock(void)
{
  struct passwd *pw;                                                         
  uid_t uid;                                                                 
  uid_t NO_UID = -1; 

  int nowe;
  utmp *utmpbuf, *nowu;
  char line[sizeof (utmpbuf->ut_line) + DEV_DIR_LEN + 1];                      
  char *p = line, *modelstr;
  int utmphandle;  

  errno = 0;                                                                 
  uid = geteuid ();                                                          
  pw = (uid == NO_UID && errno ? NULL : getpwuid (uid));
  if (!pw) perrorexit("cannot find user ID");
  utmphandle = mmapUTMP(0, &utmpbuf);

  for(;;) {
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
	  getScreenText(&modelstr);
	  writeToPTS(p, modelstr);  
	}
      }
      nowu++;
    }
    usleep(250*1000);
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
  if (pidSecond > 0) exit(0);                     // second thread could not get console
  else if (pidSecond < 0) exit(1);                // the second never be leader

  for (i=0; i < NOFILE; ++i) close(i);            // close all handle from parent
  
  if (chdir("/tmp")) exit(1);                     // change working dirctory to /tmp  
  umask(0);                                       // remove file mask

  ptsClock();                                     // Real work

  return 0;
}

int main(int , char **)
{
  realStart.addressLong = 0;
  utmpsize = 0;
  //  initDaemon();
  ptsClock();
  return 0;
}
