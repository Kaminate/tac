#pragma once

#include "src/common/tacUtility.h"
#include "src/common/tacShell.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacMemory.h"
#include "src/common/tackeyboardinput.h"

namespace Tac
{


  enum class ThreadType
  {
    Unknown,
    Main,
    Stuff
  };

  extern thread_local ThreadType gThreadType;

  struct DesktopEventQueue
  {
    static DesktopEventQueue Instance;
    void Init();
    void PushEventCursorUnobscured( DesktopWindowHandle );
    void PushEventCreateWindow( DesktopWindowHandle,
                                int width,
                                int height,
                                int x,
                                int y,
                                void* nativeWindowHandle );
    void PushEventMoveWindow( DesktopWindowHandle,
                              int x,
                              int y );
    void PushEventResizeWindow( DesktopWindowHandle,
                                int w,
                                int h );
    void PushEventKeyState( Key, bool );
    void PushEventKeyInput( Codepoint );
    void PushEventMouseWheel( int ticks );

    void ApplyQueuedEvents( DesktopWindowStateCollection* );
  };


  struct DesktopApp // final
  {
    static DesktopApp* Instance;
    DesktopApp();
    ~DesktopApp();
    void                   Run();
    virtual void           Init( Errors& );
    virtual void           Poll( Errors& ) {}
    //void                   Run();
    virtual void           SpawnWindow( DesktopWindowHandle handle,
                                        int x,
                                        int y,
                                        int width,
                                        int height ) = 0;
    void                   SpawnWindow( DesktopWindow* );
    virtual void           GetPrimaryMonitor( Monitor* monitor, Errors& errors ) = 0;
    virtual void           CreateControllerInput( Errors& ) {};
    DesktopWindow*         FindDesktopWindow( DesktopWindowHandle );

    // maybe these dont have to be pointers
    DesktopWindow*         mDesktopWindows[ kMaxDesktopWindowStateCount ] = {};
    Errors                 mErrorsMainThread;
    Errors                 mErrorsStuffThread;
    String                 mAppName;
    String                 mPrefPath;
    String                 mInitialWorkingDir;
    void( *mProjectInit )( Errors& ) = 0;
    void( *mProjectUpdate )( Errors& ) = 0;
  };

  //struct WindowState
  //{
  //  String mName; // replace with handle
  //  int mWidth;
  //  int mHeight;
  //  void* mhwnd;
  //};


}
