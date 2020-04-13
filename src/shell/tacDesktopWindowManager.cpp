#include "src/shell/tacDesktopWindowManager.h"
#include "src/shell/tacDesktopApp.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/tacOS.h"
#include "src/common/tacKeyboardinput.h" // temp
#include <thread>

namespace Tac
{

  struct WantSpawnInfo
  {
    DesktopWindowHandle mHandle;
    int mX;
    int mY;
    int mWidth;
    int mHeight;
  };

  static std::mutex gWindowSpawnInfoMutex;
  Vector< WantSpawnInfo > gWantSpawnWindows;
  static int gWindowCount;

  DesktopWindowManager::DesktopWindowManager()
  {
    Instance = this;
  }
  DesktopWindowManager* DesktopWindowManager::Instance = nullptr;

  //void DesktopWindowManager::SetWindowParams( WindowParams windowParams )
  //{
  //  WindowParams* pParams = FindWindowParams( windowParams.mName );
  //  if( pParams )
  //    *pParams = windowParams;
  //  else
  //    mWindowParams.push_back( windowParams );
  //}

  //WindowParams* DesktopWindowManager::FindWindowParams( const StringView& windowName )
  //{
  //  for( WindowParams& data : mWindowParams )
  //    if( data.mName == windowName )
  //      return &data;
  //  return nullptr;
  //}

  //void DesktopWindowManager::DoWindow( const StringView& windowName )
  //{
  //  DesktopWindow* window = DesktopApp::Instance->FindWindow( windowName );
  //  if( !window )
  //    mWantSpawnWindows.push_back( windowName );
  //}

  void DesktopWindowManager::Update( Errors& errors )
  {
    Vector< WantSpawnInfo > wantSpawnWindows;
    {
      std::lock_guard< std::mutex > lock( gWindowSpawnInfoMutex );
      wantSpawnWindows = gWantSpawnWindows;
      gWantSpawnWindows.clear();
    }
    for( WantSpawnInfo info : wantSpawnWindows )
    {
      DesktopApp::Instance->SpawnWindow(
        info.mHandle,
        info.mX,
        info.mY,
        info.mWidth,
        info.mHeight );
      TAC_HANDLE_ERROR( errors );
    }
  }

  DesktopWindowHandle DesktopWindowManager::CreateWindow( int x, int y, int width, int height )
  {
    std::lock_guard< std::mutex > lock( gWindowSpawnInfoMutex );
    DesktopWindowHandle handle = { gWindowCount++ };
    WantSpawnInfo info;
    info.mX = x;
    info.mY = y;
    info.mWidth = width;
    info.mHeight = height;
    info.mHandle = handle;
    gWantSpawnWindows.push_back( info );
    return handle;
  }
}
