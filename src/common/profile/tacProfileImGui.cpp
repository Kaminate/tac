
#include "common/profile/tacProfile.h"
#include "common/profile/tacProfileImGui.h"
#include "common/graphics/imgui/tacImGuiState.h"
#include "common/graphics/tacUI2D.h"

struct TacImguiProfileWidgetData : public TacImGuiWindowResource
{
  float mLMiliseconds = 0.0f;
  float mRMiliseconds = ( 1.0f / 60.0f ) * 1.3f * 1000.0f;
};

TacVector<TacImguiProfileWidgetData> gImguiProfileWidgetDatas;

void TacImGuiProfileWidget( TacProfileFunction* profileFunction )
{
  if( !profileFunction )
    return;

  TacUI2DDrawData* drawData = TacImGuiGlobals::Instance.mUI2DDrawData;
  TacImGuiWindow* imguiWindow = TacImGuiGlobals::Instance.mCurrentWindow;
  auto profileWidgetData = imguiWindow->GetOrCreateResource<TacImguiProfileWidgetData>();
  // TacImGuiBeginGroup();

  float itemWidth = imguiWindow->mContentRect.mMaxi.x - imguiWindow->mCurrCursorDrawPos.x;
  float itemHeight = ( float )TacImGuiGlobals::Instance.mUIStyle.fontSize;

  v2 timeScalePos = imguiWindow->mCurrCursorDrawPos;
  v2 timeScaleSize = { itemWidth, itemHeight };
  imguiWindow->ItemSize(timeScaleSize);
  
  TacString lTimestamp = TacToString( profileWidgetData->mLMiliseconds );
  TacString rTimestamp = TacToString( profileWidgetData->mRMiliseconds );
  drawData->AddText(
    timeScalePos,
    TacImGuiGlobals::Instance.mUIStyle.fontSize,
    lTimestamp,
    TacImGuiGlobals::Instance.mUIStyle.textColor, nullptr );

  v2 rSize = drawData->CalculateTextSize(rTimestamp, TacImGuiGlobals::Instance.mUIStyle.fontSize);
  drawData->AddText(
    timeScalePos + v2(itemWidth - rSize.x, 0),
    TacImGuiGlobals::Instance.mUIStyle.fontSize,
    rTimestamp,
    TacImGuiGlobals::Instance.mUIStyle.textColor, nullptr );


  v2 boxSize = { itemWidth, itemHeight };
  v2 boxPos = imguiWindow->mCurrCursorDrawPos;
  v4 boxColor = v4( v3( 1, 1, 1 ) * 0.1f, 1.0f );

  bool boxClipped;
  auto boxClipRect = TacImGuiRect::FromPosSize( boxPos, boxSize );
  imguiWindow->ComputeClipInfo( &boxClipped, &boxClipRect );
  
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



