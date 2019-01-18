// This file wraps a windows application,
// sets up the major systems and passes them
// to the game loop

#pragma once

#include "common/tacShell.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"
#include "common/tacEvent.h"
#include "shell/tacDesktopApp.h"
#include "shell/windows/tacWindows.h"
#include "shell/windows/tacWindowsMouseEdge.h"

struct TacWin32Log : public TacEvent< const TacString& >::Handler
{
  void HandleEvent( const TacString& )override;
};

struct TacWin32DesktopWindow : public TacDesktopWindow
{
  LRESULT HandleWindowProc( UINT uMsg, WPARAM wParam, LPARAM lParam );
  void Poll( TacErrors& errors );

  TacWindowsApplication2* app = nullptr;
  HWND mHWND = NULL;
  bool mIsMouseInWindow = false;
  TacErrors mWindowProcErrors;
};


struct TacWindowsApplication2 : public TacDesktopApp
{
  TacWindowsApplication2();
  ~TacWindowsApplication2();
  void Init( TacErrors& errors ) override;
  void Poll( TacErrors& errors ) override;

  // Note: The first window spawned will be the parent window,
  //       combining them all into one tab group
  void SpawnWindow( const TacWindowParams& , TacDesktopWindow** , TacErrors& ) override;
  void GetPrimaryMonitor( TacMonitor* monitor, TacErrors& errors ) override;
  TacOwned< TacWin32MouseEdgeCommonData > mMouseEdgeCommonData;


  // WinMain arguments
  HINSTANCE mHInstance = nullptr;
  HINSTANCE mhPrevInstance = nullptr;
  LPSTR mlpCmdLine = nullptr;
  int mNCmdShow = 0;

  //std::map< uint8_t, TacKey > mKeyMap;

  bool mShouldWindowHaveBorder;
  TacVector< TacWin32DesktopWindow* > mWindows;
  HWND mParentHWND = NULL;
  TacWin32MouseEdgeHandler* mMouseEdgeHandler = nullptr;
};

