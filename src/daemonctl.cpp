
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>  
#include <sys/param.h>  
#include <sys/types.h>  
#include <sys/stat.h> 
#include "../include/daemonctl.h"

char CONTENT_HELP[]     = "content for linux\n";
char CONTENT_START[]    = "start content ... \n";
char CONTENT_STOP[]     = "stop content ...\n";
char CONTENT_PAUSE[]    = "pause content\n";
char CONTENT_STATUS[]   = "show status\n";
char CONTENT_ERROR[]    = "error parameter\n";

int
initDaemon(void){
  int pidFirst, pidSecond;  
  int i;  

  // the first child thread, for relese console
  pidFirst = fork();
  if (pidFirst > 0) exit(0);                                                    // is parent thread, exit
  else if (pidFirst < 0) exit(1);                                               // error in fork
  setsid();                                                                     // first child be leader  

  // the second child thread, for could not get console again
  pidSecond = fork();
  if (pidSecond > 0) exit(0);                                                   // second thread could not get console
  else if (pidSecond < 0) exit(1);                                              // the second never be leader

  for (i=0; i < NOFILE; ++i) close(i);                                          // close all handle from parent
  
  if (chdir("/tmp")) exit(1);                                                // change working dirctory to /tmp  
  umask(0);                                                                     // remove file mask
  return 0;
}

int
helpContent(void) {
  printf("%s", CONTENT_HELP);
  return 0;
}

int
startContent(void) {
  printf("%s", CONTENT_START);
  initDaemon();
  return 0;
}

int
stopContent(void) {
  printf("%s", CONTENT_STOP);
  return 0;
}

int
pauseContent(void) {
  printf("%s", CONTENT_PAUSE);
  return 0;
}

int
statusContent(void) {
  printf("%s", CONTENT_STATUS);
  return 0;
}

int
errorContent(void) {
  printf("%s", CONTENT_ERROR);
  return 0;
}

int 
parseOptions(int argc, char * const *argv) {
  int charcode = 0;
  struct option longopts[] = {
    { "help",	  0, NULL, 'H'},
    { "start",	  0, NULL, 'S'},
    { "stop",	  0, NULL, 'K'},
    { "restart",  0, NULL, 'R'},
    { "pause",	  0, NULL, 'P'},
    { "status",	  0, NULL, 'T'},
    { NULL,       0, NULL, 0}
  };
  if (argc == 1) charcode = 'S';
  if (argc == 2)
    charcode = getopt_long(argc, argv, "HSKRPT", longopts, (int *) 0);

  switch (charcode) {
  case 'H': 
    helpContent();
    break;
  case 'S': 
    startContent();
    break;
  case 'K': 
    stopContent();
    break;  
  case 'R': 
    stopContent();
    startContent();
    break;
  case 'P': 
    pauseContent();
    break;
  case 'T': 
    statusContent();
    break;
  default:
    errorContent();
    helpContent();
    exit(0);
  }
  return 0;
}

int
main (int argc, char **argv)
{
  int result = parseOptions(argc, argv);
  sleep(30000);
  return result;
}
