/*
  main include file
 */

int helpContent(void);
int startContent(void);
int stopContent(void);
int pauseContent(void);
int statusContent(void);
int errorContent(void);

int initDaemon(void);

int parseOptions(int argc, char * const *argv);
int main(int argc, char** argv);
