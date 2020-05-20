#pragma once

#include "src/common/tacUtility.h"
#include "src/common/tacShell.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacMemory.h"
#include <thread>

namespace Tac
{


  enum class ThreadType
  {
    Unknown,
    Main,
    Stuff
  };

  extern thread_local ThreadType gThreadType;

  //struct ProcessStuffOutput
  //{
  //  bool mCreatedWindow = false;
  //  DesktopWindowState mCreatedWindowState;
  //};

  namespace DesktopEvent
  {
    void                PushEventCursorUnobscured( DesktopWindowHandle desktopWindowHandle );
    void                PushEventCreateWindow( DesktopWindowHandle desktopWindowHandle,
                                               int width,
                                               int height,
                                               void* nativeWindowHandle );

    void ProcessStuff( bool* createdWindows );
  }

  // stuff thread
  extern DesktopWindowStates gDesktopWindowStates;
  DesktopWindowState* FindDeskopWindowState( DesktopWindowHandle );

  struct DesktopApp
  {
    static DesktopApp* Instance;
    DesktopApp();
    virtual ~DesktopApp();
    virtual void   Init( Errors& );
    virtual void   Poll( Errors& ) {}
    void           Run();
    virtual void   SpawnWindow( DesktopWindowHandle handle, int x, int y, int width, int height ) = 0;
    //void           KillDeadWindows();
    virtual void   GetPrimaryMonitor( Monitor* monitor, Errors& errors ) = 0;
    virtual void   SpawnWindowAux( const WindowParams& , DesktopWindow** , Errors& ) {};
    virtual void   CreateControllerInput(Errors&) {};
    //DesktopWindow* FindWindow( StringView windowName );

    //Vector< DesktopWindow* > mMainWindows;
    Errors mErrorsMainThread;
    Errors mErrorsStuffThread;

    DesktopWindowState mDesktopWindowStates[ 10 ];
  };

  //struct WindowState
  //{
  //  String mName; // replace with handle
  //  int mWidth;
  //  int mHeight;
  //  void* mhwnd;
  //};


}
