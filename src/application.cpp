// application.cpp

#include "../include/raymoncommon.h"
#include "../include/application.hpp"

#define InheritApplication(app)			\
  app::app() : CApplication() {}

InheritApplication(CEchoApplication);
InheritApplication(CWriteApplication);

INT CEchoApplication::DoApplication(ADDR mContent)
{
  printf("in echo %p\n", mContent.pVoid);
  return 0;
}

INT CWriteApplication::DoApplication(ADDR mContent)
{
  printf("in write %p\n", mContent.pVoid);
  return 0;
}
