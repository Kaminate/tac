#include "src/shell/tacDesktopWindowManager.h"
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

}
