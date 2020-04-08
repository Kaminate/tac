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

  void DesktopWindowManager::SetWindowCreationData(
    const StringView& windowName,
    int width,
    int height,
    int x,
    int y )
  {
    WindowCreationData* pData = FindWindowCreationData( windowName );
    if( !pData )
    {
      mWindowCreationData.resize( mWindowCreationData.size() + 1 );
      pData = &mWindowCreationData.back();
      pData->mWindowName = windowName;
    }

    pData->mWidth = width;
    pData->mHeight = height;
    pData->mX = x;
    pData->mY = y;
  }

  WindowCreationData* DesktopWindowManager::FindWindowCreationData( const StringView& windowName )
  {
    for( WindowCreationData& data : mWindowCreationData )
      if( data.mWindowName == windowName )
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

      WindowCreationData* pData = FindWindowCreationData( windowName );

      WindowParams params;
      params.mHeight = pData ? pData->mHeight : 800;
      params.mName = windowName;
      params.mWidth = pData ? pData->mWidth : 600;
      params.mX = pData ? pData->mX : 50;
      params.mY = pData ? pData->mY : 50;
      DesktopApp::Instance->SpawnWindow( params, &window, errors );
      TAC_HANDLE_ERROR( errors );
    }
    mWantSpawnWindows.clear();
  }
}
