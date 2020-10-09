// This file wraps a windows application,
// sets up the major systems and passes them
// to the game loop

#pragma once

// -- old --
#include "src/common/tacShell.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacEvent.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/windows/tacWin32.h"
// -- 

namespace Tac
{
  // -- new --

  void WindowsManagerInit( Errors& );
  void WindowsManagerPoll( Errors& );
  void WindowsManagerSpawnWindow( DesktopWindowHandle handle,
                                  int x,
                                  int y,
                                  int width,
                                  int height );
  //void CreateWin32ControllerInput( Errors& );

  // -- old --

  //struct Win32DesktopWindow : public DesktopWindow
  //{
  //  ~Win32DesktopWindow();
  //  LRESULT HandleWindowProc( UINT uMsg,
  //                            WPARAM wParam,
  //                            LPARAM lParam );
  //  void    Poll( Errors& errors );
  //  void*   GetOperatingSystemHandle() override;
  //  HWND    mHWND = NULL;
  //  bool    mIsMouseInWindow = false;
  //  Errors  mWindowProcErrors;
  //};


  //struct WindowsApplication2
  //{
  //  //void                RemoveWindow( Win32DesktopWindow* );

  //  //Win32DesktopWindow* FindWin32DesktopWindow( HWND );
  //  //Win32DesktopWindow* GetCursorUnobscuredWindow();

  //  // WinMain arguments
  //  HINSTANCE           mHInstance = nullptr;
  //  HINSTANCE           mhPrevInstance = nullptr;
  //  LPSTR               mlpCmdLine = nullptr;
  //  int                 mNCmdShow = 0;

  //  //Vector< Win32DesktopWindow* > mWindows;
  //  DesktopWindowHandle mUnobscuredDesktopWindowHandle;
  //};

}
