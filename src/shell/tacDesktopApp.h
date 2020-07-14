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
    void                PushEventCursorUnobscured( DesktopWindowHandle );
    void                PushEventCreateWindow( DesktopWindowHandle,
                                               int width,
                                               int height,
                                               int x,
                                               int y,
                                               void* nativeWindowHandle );
    void                PushEventMoveWindow( DesktopWindowHandle,
                                             int x,
                                             int y );
    void                PushEventResizeWindow( DesktopWindowHandle,
                                               int w,
                                               int h );

    void ProcessStuff( bool* createdWindows );
  }

  // stuff thread
  extern DesktopWindowStates gDesktopWindowStates;
  DesktopWindowState* FindDesktopWindowState( DesktopWindowHandle );

  struct DesktopApp
  {
    static DesktopApp* Instance;
    DesktopApp();
    virtual ~DesktopApp();
    virtual void           Init( Errors& );
    virtual void           Poll( Errors& ) {}
    void                   Run();
    virtual void           SpawnWindow( DesktopWindowHandle handle, int x, int y, int width, int height ) = 0;
    void                   SpawnWindow( DesktopWindow* );
    virtual void           GetPrimaryMonitor( Monitor* monitor, Errors& errors ) = 0;
    virtual void           SpawnWindowAux( const WindowParams&, DesktopWindow**, Errors& ) {};
    virtual void           CreateControllerInput( Errors& ) {};
    DesktopWindow*         FindDesktopWindow( DesktopWindowHandle );

    // maybe these dont have to be pointers
    DesktopWindow*         mDesktopWindows[ kMaxDesktopWindowStateCount ] = {};
    Errors                 mErrorsMainThread;
    Errors                 mErrorsStuffThread;
  };

  //struct WindowState
  //{
  //  String mName; // replace with handle
  //  int mWidth;
  //  int mHeight;
  //  void* mhwnd;
  //};


}
