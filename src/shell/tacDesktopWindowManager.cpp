#include "shell/tacDesktopWindowManager.h"

#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacUI2D.h"
#include "common/graphics/tacUI.h"
#include "common/tacOS.h"
#include "common/tackeyboardinput.h" // temp
#include <thread>

TacDesktopWindowManager::TacDesktopWindowManager()
{
  Instance = this;
}
TacDesktopWindowManager* TacDesktopWindowManager::Instance = nullptr;

void TacDesktopWindowManager::SetWindowCreationData(
  const TacStringView& windowName,
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

WindowCreationData* TacDesktopWindowManager::FindWindowCreationData( const TacStringView& windowName )
{
  for( WindowCreationData& data : mWindowCreationData )
    if( data.mWindowName == windowName )
      return &data;
  return nullptr;
}

