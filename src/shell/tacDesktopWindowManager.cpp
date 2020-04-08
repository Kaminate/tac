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


  DesktopWindowManager::DesktopWindowManager()
  {
    Instance = this;
  }
  DesktopWindowManager* DesktopWindowManager::Instance = nullptr;

  void DesktopWindowManager::SetWindowParams( WindowParams windowParams )
  {
    WindowParams* pParams = FindWindowParams( windowParams.mName );
    if( pParams )
      *pParams = windowParams;
    else
      mWindowParams.push_back( windowParams );
  }

  WindowParams* DesktopWindowManager::FindWindowParams( const StringView& windowName )
  {
    for( WindowParams& data : mWindowParams )
      if( data.mName == windowName )
        return &data;
    return nullptr;
  }

  void DesktopWindowManager::DoWindow( const StringView& windowName )
  {
    DesktopWindow* window = DesktopApp::Instance->FindWindow( windowName );
    if( !window )
      mWantSpawnWindows.push_back( windowName );
  }

  void DesktopWindowManager::Update( Errors& errors )
  {
    for( String windowName : mWantSpawnWindows )
    {
      DesktopWindow* window = DesktopApp::Instance->FindWindow( windowName );
      if( window )
        continue;

      WindowParams* pParams = FindWindowParams( windowName );

      WindowParams params;
      params.mHeight = pParams ? pParams->mHeight : 800;
      params.mName = windowName;
      params.mWidth = pParams ? pParams->mWidth : 600;
      params.mX = pParams ? pParams->mX : 50;
      params.mY = pParams ? pParams->mY : 50;
      DesktopApp::Instance->SpawnWindow( params, &window, errors );
      TAC_HANDLE_ERROR( errors );
    }
    mWantSpawnWindows.clear();
  }
}
