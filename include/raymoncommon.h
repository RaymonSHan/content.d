
#define NORMAL_PAGE_SIZE     (1LL << 12)

#define STACK_START (0x53LL << 40)           // 'S'
#define STACK_SIZE  (16*1024*1024)

union assignAddress
{
  long long int addressLong;
  char *addressChar;
  void *addressVoid;
  long long int *addressLPointer;
};

void perrorexit(const char* s);
char* getStack(void);
