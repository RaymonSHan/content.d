#ifndef INCLUDE_APPLICATION_HPP
#define INCLUDE_APPLICATION_HPP

#include "rtype.hpp"

class CApplication
{
private:
  CApplication *nextApplication;
public:
   inline void SetNextApplication(CApplication *app)
    { nextApplication = app; };
   inline CApplication* GetNextApplication(void)
    { return nextApplication; };
  virtual INT DoApplication(ADDR mContent) = 0;
};

class CEchoApplication : public CApplication
{
public:
  virtual INT DoApplication(ADDR mContent);
};

class CWriteApplication : public CApplication
{
public:
  virtual INT DoApplication(ADDR mContent);
};

#endif  // INCLUDE_APPLICATION_HPP
