// This file wraps a windows application,
// sets up the major systems and passes them
// to the game loop

#pragma once

#include "src/common/tacShell.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacEvent.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/windows/tacWindows.h"
#include "src/shell/windows/tacWindowsMouseEdge.h"

namespace Tac
{


  struct Win32DesktopWindow : public DesktopWindow
  {
    ~Win32DesktopWindow();
    LRESULT HandleWindowProc( UINT uMsg, WPARAM wParam, LPARAM lParam );
    void Poll( Errors& errors );

    WindowsApplication2* app = nullptr;
    HWND mHWND = NULL;
    bool mIsMouseInWindow = false;
    Errors mWindowProcErrors;
  };


  struct WindowsApplication2 : public DesktopApp
  {
    static WindowsApplication2* Instance;
    WindowsApplication2();
    ~WindowsApplication2();
    void Init( Errors& errors ) override;
    void Poll( Errors& errors ) override;

    // Note: The first window spawned will be the parent window,
    //       combining them all into one tab group
    void SpawnWindowAux( const WindowParams&, DesktopWindow**, Errors& ) override;
    void GetPrimaryMonitor( Monitor* monitor, Errors& errors ) override;
    void RemoveWindow( Win32DesktopWindow* );
    Win32DesktopWindow* FindWindow( HWND hwnd );

    Win32DesktopWindow* GetCursorUnobscuredWindow();

    // WinMain arguments
    HINSTANCE mHInstance = nullptr;
    HINSTANCE mhPrevInstance = nullptr;
    LPSTR mlpCmdLine = nullptr;
    int mNCmdShow = 0;

    bool mShouldWindowHaveBorder;
    Vector< Win32DesktopWindow* > mWindows;
    HWND mParentHWND = NULL;
    Win32MouseEdgeHandler* mMouseEdgeHandler = nullptr;
    Win32Cursors* mCursors = new Win32Cursors();
  };

}
