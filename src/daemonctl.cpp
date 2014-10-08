
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

//#define HUGE_PAGE_USED

// for x64 page table is 9-9-9-9-12 level, for huge page with size of 2M,
// L3 page table contain 1G, L2 for 512G, L1 for total 256T = 2^40.
// In my huge segment, which size if 512G, with boundary 0x0000 ??80 0000 0000
// should use 512+1 page table which size about 2M
// But it seems HUGETLB not support MADV_DONTNEED, 
// I try MADV_DONTNEED by normal page first
#define HUGE_SEGMENT_START (0x52LL << 40)  // 'R'
#define HUGE_PAGE_SIZE     (1LL << 21)

// for x64 page table is 9-9-9-9-12 level, for normal page with size of 4k
// test for DONTNEED behavior
union assignAddress
{
  long long int addressLong;
  char *addressChar;
  void *addressVoid;
  long long int *addressLPointer;
};
#define NORMAL_SEGMENT_START (0x52LL << 40)  // 'R'
#define NORMAL_PAGE_SIZE     (1LL << 12)

#define WRITE_SELF(add) *add.addressLPointer = add.addressLong
#define VALID_SELF(add) *add.addressLPointer == add.addressLong
#define NEXT_HUGE_PAGE(add)  add.addressLong += HUGE_PAGE_SIZE
#define NEXT_NORMAL_PAGE(add) add.addressLong += NORMAL_PAGE_SIZE
#define FREE_HUGE_PAGE(add, num) madvise(add.addressVoid, num<<21, MADV_DONTNEED)
#define FREE_NORMAL_PAGE(add, num) madvise(add.addressVoid, num<<12, MADV_DONTNEED)

#ifdef HUGE_PAGE_USED
#define SEGMENT_START HUGE_SEGMENT_START
#define PAGE_SIZE HUGE_PAGE_SIZE
#define NEXT_PAGE NEXT_HUGE_PAGE
#define FREE_PAGE FREE_HUGE_PAGE
#define MMFLAG    MAP_HUGETLB | MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE
#else //HUGE_PAGE_USED
#define SEGMENT_START NORMAL_SEGMENT_START
#define PAGE_SIZE NORMAL_PAGE_SIZE
#define NEXT_PAGE NEXT_NORMAL_PAGE
#define FREE_PAGE FREE_NORMAL_PAGE
#define MMFLAG    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE
#endif //HUGE_PAGE_USED

#define FILEPATH "/home/raymon/mapfile"
//#define NUMINTS  (1000)
//#define FILESIZE (NUMINTS * sizeof(int))

union assignAddress assignStart, realStart, usedp;
long utmpsize = 0;
timespec ts1, ts2, ts3, ts4, ts5;

unsigned long long difftime (timespec &t2, timespec &t1)
{
  return ((t2.tv_sec-t1.tv_sec)*1000000000 + (t2.tv_nsec-t1.tv_nsec));
}

void perrorexit(const char* s)
{
  perror(s);
  exit(-1);
}

/*
int read_utmp (const char *filename, int *n_entries, utmp **utmp_buf)
{
  FILE *utmpfile;
  struct stat file_stats;
  size_t n_read;
  size_t size;
  struct utmp *buf;

  utmpfile = fopen (filename, "r");
  if (utmpfile == NULL) return 1;

  fstat (fileno (utmpfile), &file_stats);
  size = file_stats.st_size;
  if (size > 0) buf = (struct utmp*) malloc (size);
  else {
    fclose (utmpfile);
    return 1;
  }

//   Use < instead of != in case the utmp just grew.  
  n_read = fread (buf, 1, size, utmpfile);
  if (ferror (utmpfile) || fclose (utmpfile) == EOF
      || n_read < size)
    return 1;

  *n_entries = size / sizeof (struct utmp);
  *utmp_buf = buf;

  return 0;
}
*/

int mmapUTMP(int utmpfile, utmp **utmp_buf)
{
  //  assignStart.addressLong = SEGMENT_START;
  if (!utmpfile) utmpfile = open(UTMP_FILE, O_RDONLY);
  realStart.addressVoid = mmap (0, NORMAL_PAGE_SIZE*100, 
				PROT_READ, MAP_SHARED, utmpfile, 0);
  if (realStart.addressVoid == MAP_FAILED) perrorexit("Error in mmap file");

  *utmp_buf = static_cast<utmp*>(realStart.addressVoid);
  //  munmap(realStart.addressVoid, NORMAL_PAGE_SIZE*100);
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
    //    printf("Read %ld bytes from inotify fd\n", (long) numRead);

    /* Process all of the events in buffer returned by read() */
    for (p = buf; p < buf + numRead; ) {
      event = (struct inotify_event *) p;
      if (event->mask & IN_MODIFY)        printf("IN_MODIFY\n ");
      p += sizeof(struct inotify_event) + event->len;
    }
  }
  exit(EXIT_SUCCESS);
}

void manyIncrease(void)
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

static void displayInotifyEvent(struct inotify_event *i)
{
  printf("    wd =%2d; ", i->wd);
  if (i->cookie > 0)
    printf("cookie =%4d; ", i->cookie);

  printf("mask = ");
  if (i->mask & IN_ACCESS)        printf("IN_ACCESS ");
  if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
  if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
  if (i->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
  if (i->mask & IN_CREATE)        printf("IN_CREATE ");
  if (i->mask & IN_DELETE)        printf("IN_DELETE ");
  if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
  if (i->mask & IN_IGNORED)       printf("IN_IGNORED ");
  if (i->mask & IN_ISDIR)         printf("IN_ISDIR ");
  if (i->mask & IN_MODIFY)        printf("IN_MODIFY ");
  if (i->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
  if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
  if (i->mask & IN_MOVED_TO)      printf("IN_MOVED_TO ");
  if (i->mask & IN_OPEN)          printf("IN_OPEN ");
  if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
  if (i->mask & IN_UNMOUNT)       printf("IN_UNMOUNT ");
  printf("\n");

  if (i->len > 0)
    printf("        name = %s\n", i->name);
}

int inotifytest(void)
{
  int inotifyFd, wd;
  char buf[BUF_LEN] __attribute__ ((aligned(8)));
  ssize_t numRead;
  char *p;
  struct inotify_event *event;

  inotifyFd = inotify_init();                 /* Create inotify instance */
  if (inotifyFd == -1) perrorexit("inotify_init");

  //  wd = inotify_add_watch(inotifyFd, UTMP_FILE, IN_MODIFY);
  wd = inotify_add_watch(inotifyFd, FILEPATH, IN_ALL_EVENTS);
  if (wd == -1) perrorexit("inotify_add_watch");

  for (;;) {                                  /* Read events forever */
    numRead = read(inotifyFd, buf, BUF_LEN);
    if (numRead == 0) perror("read() from inotify fd returned 0!");

    if (numRead == -1) perrorexit("read");
    printf("Read %ld bytes from inotify fd\n", (long) numRead);

    /* Process all of the events in buffer returned by read() */
    for (p = buf; p < buf + numRead; ) {
      event = (struct inotify_event *) p;
      displayInotifyEvent(event);
      p += sizeof(struct inotify_event) + event->len;
    }
  }
  exit(EXIT_SUCCESS);
}

int mmapfile(void)
{
  int fd;
  assignStart.addressLong = SEGMENT_START;

  int result;
  char buf[] = "this is test little long";
  fd = open(FILEPATH, O_RDWR);
  if (fd == -1) perrorexit("Error opening file for writing");

  realStart.addressVoid = mmap (assignStart.addressVoid, NORMAL_PAGE_SIZE, 
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (realStart.addressVoid == MAP_FAILED) perrorexit("Error in mmap file");

  result = lseek(fd, sizeof(buf)-1, SEEK_SET);
  if (result == -1) perrorexit("Error calling lseek()");

  result = write(fd, "", 1);
  if (result == -1) perrorexit("Error calling write()");

  for (int i=0; i<100; i++) {
    printf ("%s\n", (realStart.addressChar));
    sleep(5);
  }
  munmap(realStart.addressVoid, 10000);
  close(fd);
  return 0;
}

void map128M(union assignAddress add)
{
  add.addressVoid = mmap (add.addressVoid, 128*1024*1024, PROT_READ | PROT_WRITE, 
			  MMFLAG | MAP_FIXED, -1, 0);
  for (unsigned int i=0; i<(128*1024*1024/sizeof(long long)); i++) {
    WRITE_SELF(add);
    add.addressLong += sizeof(long long);
  }
}

void unmap128M(union assignAddress add)
{
  munmap(add.addressVoid, 128*1024*1024);
}

// time ./content form mmap & munmap
// real 7.8s, user 0.2s, sys 7.6s for 400*128M alloc and page fault 
// it same as MADV_DONTNEED
// avg for 4k page is 595ns
int mapunmap() 
{
  usedp.addressLong = SEGMENT_START;
  for (int i=0; i<400; i++) {
    //    printf("128 Loop %d, addr:%llx\n", i, usedp.addressLong);
    map128M(usedp);
    unmap128M(usedp);
    usedp.addressLong += 128*1024*1024;
  }
  return 0;
}

void get128M(union assignAddress add) {
  for (unsigned int i=0; i<(128*1024*1024/sizeof(long long)); i++) {
    //    if (VALID_SELF(add)) printf("IS VALID %llx\n", add.addressLong);
    WRITE_SELF(add);
    add.addressLong += sizeof(long long);
    //    NEXT_PAGE(add);
  }
}

// madvise for HUGEPAGE will fail
void back128M(union assignAddress add)
{
  FREE_PAGE(add, 128*1024*1024/PAGE_SIZE);
}

// time ./content for MADV_DONTNEED
// real 7.8s, user 0.2s, sys 7.6s for 400*128M alloc and page fault
// avg for 4k page is 595ns
int madviseDontNeed ()
{
  assignStart.addressLong = SEGMENT_START;
  long long int memsize = 1LL << 39;
  //  printf("memsize: 0x%llx\n", memsize);

  realStart.addressVoid = (mmap (assignStart.addressVoid, memsize, PROT_READ | PROT_WRITE, 
				 MMFLAG, -1, 0));
  //  printf("after mmap: error:%d,realStart:0x%llx\n", errno, realStart.addressLong);
  usedp = realStart;

  for (int i=0; i<400; i++) {
    //    printf("128 Loop %d, addr:%llx\n", i, usedp.addressLong);
    get128M(usedp);
    if (i>2) back128M(usedp);
    usedp.addressLong += 128*1024*1024;
  }

  usedp = realStart;
  for (int i=0; i<400; i++) {
    //    printf("Valid view Loop %d, addr:%llx, val:%llx\n", i, usedp.addressLong, *usedp.addressLPointer);
    usedp.addressLong += 128*1024*1024;
  }

  munmap(realStart.addressVoid, memsize);
  //  printf("after munmap: error:%d,realStart:0x%llx\n", errno, realStart.addressLong);

  return 0;
}

#define SIZE_RAY 10

int hugePageAlloc ()
{
  char *pagestart1;
  char *nowstart, *nowend;

  clock_gettime(CLOCK_MONOTONIC, &(ts1));
  pagestart1 = static_cast<char*>
    (mmap (0, SIZE_RAY << 21, PROT_READ | PROT_WRITE,
	   MAP_HUGETLB | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  perror("after huge mmap 10");

  clock_gettime(CLOCK_MONOTONIC, &(ts2));
  nowstart = pagestart1;
  nowend = nowstart + (SIZE_RAY << 21) - 1;
  do {
    *nowstart = 0x67;
    nowstart ++;
  }
  while (nowstart < nowend);
  perror("after write huge first");

  clock_gettime(CLOCK_MONOTONIC, &(ts3));
  nowstart = pagestart1 + 1;
  nowend = nowstart + (SIZE_RAY << 21) - 1;
  do {
    *nowstart = 0x67;
    nowstart ++;
  }
  while (nowstart < nowend);
  perror("after write huge second");

  clock_gettime(CLOCK_MONOTONIC, &(ts4));
  nowstart = pagestart1 + 1;
  nowend = nowstart + (SIZE_RAY << 21) - 1;
  do {
    *nowstart = 0x67;
    nowstart ++;
  }
  while (nowstart < nowend);
  perror("after write huge third");

  clock_gettime(CLOCK_MONOTONIC, &(ts5));
  munmap(pagestart1, SIZE_RAY << 21);
  perror("after unmap huge");

  printf("%llu,%llu,%llu,%llu\n", 
	 difftime(ts2, ts1), difftime(ts3, ts2), difftime(ts4, ts3), difftime(ts5, ts4));
  return 0;
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

  manyIncrease();                                 // Real work

  return 0;
}

char myutmp[] = UTMP_FILE;

int main(int , char **)
{
  realStart.addressLong = 0;
  utmpsize = 0;
  //  inotifyUTMP(myutmp);
  initDaemon();
  //  manyIncrease();
  //  inotifytest();
  //  mmapfile();
  //  mapunmap();
  //  madviseDontNeed();
  //  hugePageAlloc();
  return 0;
}
