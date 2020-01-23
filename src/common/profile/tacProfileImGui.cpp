
#include "common/profile/tacProfile.h"
#include "common/profile/tacProfileImGui.h"
#include "common/graphics/imgui/tacImGuiState.h"
#include "common/graphics/tacUI2D.h"

void TacImGuiProfileWidget( TacProfileFunction* profileFunction )
{
  if( !profileFunction )
    return;

  TacImGuiWindow* imguiWindow = TacImGuiGlobals::Instance.mCurrentWindow;
  imguiWindow->mContentRect;
  imguiWindow->mCurrCursorDrawPos;

  float itemWidth = imguiWindow->mContentRect.mMaxi.x - imguiWindow->mCurrCursorDrawPos.x;
  float itemHeight = TacImGuiGlobals::Instance.mUIStyle.fontSize;

  v2 boxSize = { itemWidth, itemHeight };
  v2 boxPos = imguiWindow->mCurrCursorDrawPos;
  v4 boxColor = v4( v3( 1, 1, 1 ) * 0.1f, 1.0f );

  bool boxClipped;
  auto boxClipRect = TacImGuiRect::FromPosSize( boxPos, boxSize );
  imguiWindow->ComputeClipInfo( &boxClipped, &boxClipRect );
  
  TacUI2DDrawData* drawData = TacImGuiGlobals::Instance.mUI2DDrawData;
  drawData->AddBox( boxPos, boxPos + boxSize, boxColor, nullptr, &boxClipRect );

  v2 textSize = drawData->CalculateTextSize(
    profileFunction->mStackFrame.mFunction,
    TacImGuiGlobals::Instance.mUIStyle.fontSize );
  v2 textPos = 
  {
    boxPos.x + ( boxSize.x - textSize.x ) / 2,
    boxPos.y,
  };

  bool textClipped;
  auto textClipRect = TacImGuiRect::FromPosSize( textPos, textSize );
  imguiWindow->ComputeClipInfo( &textClipped, &textClipRect );

  drawData->AddText(
    textPos,
    TacImGuiGlobals::Instance.mUIStyle.fontSize,
    profileFunction->mStackFrame.mFunction,
    TacImGuiGlobals::Instance.mUIStyle.textColor,
    &textClipRect );

  imguiWindow->ItemSize( boxSize );
}



