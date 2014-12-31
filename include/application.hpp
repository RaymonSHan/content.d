#ifndef INCLUDE_APPLICATION_HPP
#define INCLUDE_APPLICATION_HPP

#include "rtype.hpp"

class CApplication
{
private:
  CApplication *nextApplication;
public:
  isThis isThisApp;
public:
  CApplication(isThis func)
    { isThisApp = func; };
  inline void SetNextApplication(CApplication *app)
    { nextApplication = app; };
  inline CApplication* GetNextApplication(void)
    { return nextApplication; };
  virtual INT DoApplication(ADDR mContent) = 0;
};

#define DefineApplicationBegin(app)			\
  class app : public CApplication			\
  {

#define DefineApplicationEnd(app)			\
  public:						\
    app(isThis func);					\
    virtual INT DoApplication(ADDR mContent);		\
  };

#define DefineApplication(app, other)		\
  DefineApplicationBegin(app);			\
  other						\
  DefineApplicationEnd(app);


DefineApplication(CEchoApplication, );
DefineApplication(CWriteApplication, );

#endif  // INCLUDE_APPLICATION_HPP
