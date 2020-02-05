
#include "common/profile/tacProfile.h"
#include "common/profile/tacProfileImGui.h"
#include "common/graphics/imgui/tacImGuiState.h"
#include "common/graphics/tacUI2D.h"
#include "common/math/tacMath.h"

struct TacImguiProfileWidgetData : public TacImGuiWindowResource
{
  const float miniMiliseconds = 0.0f;
  const float maxiMiliseconds = ( 1.0f / 60.0f ) * 1.3f * 1000.0f;
  float mLMiliseconds = miniMiliseconds;
  float mRMiliseconds = maxiMiliseconds;
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
  v2 timeScaleSize = { itemWidth, itemHeight * 3 };
  imguiWindow->ItemSize(timeScaleSize);
  
  v4 boxColor = v4( v3( 1, 1, 1 ) * 0.1f, 1.0f );
  v2 timeScaleOffset = { 0, 8 };

  float timelineVOffset = itemHeight * 1.5f ;
  float timelineHOffset = itemHeight * 1.5f ;
  v2 timelineLeft = timeScalePos + v2( timelineHOffset, timelineVOffset);
  v2 timelineRight = timeScalePos + v2( itemWidth - timelineHOffset, timelineVOffset );
   
  drawData->AddLine( timelineLeft, timelineRight, 2, boxColor );

  TacAssert( ( int )( profileWidgetData->mRMiliseconds - profileWidgetData->mLMiliseconds ) < 30 );
  for( int i = ( int )profileWidgetData->mLMiliseconds; i < 1 + ( int )profileWidgetData->mRMiliseconds; ++i )
  {
    float t = ( ( float )i - profileWidgetData->mLMiliseconds )
      / ( profileWidgetData->mLMiliseconds + profileWidgetData->mRMiliseconds );
    v2 tickBottom = TacLerp( timelineLeft, timelineRight, t );
    v2 tickTop = tickBottom - v2(0, 10);

    TacStringView timestampSV = TacVa( "%d ms", i );
    TacString timestamp( timestampSV.data(), timestampSV.size() );

    v2 rSize = drawData->CalculateTextSize(timestamp, TacImGuiGlobals::Instance.mUIStyle.fontSize);
    drawData->AddText(
      tickTop - v2(rSize.x / 2.0f, TacImGuiGlobals::Instance.mUIStyle.fontSize),
      TacImGuiGlobals::Instance.mUIStyle.fontSize,
      timestamp,
      TacImGuiGlobals::Instance.mUIStyle.textColor, nullptr );
    drawData->AddLine(tickBottom, tickTop, 2.0f, boxColor);
  }


  v2 boxSize = { itemWidth, itemHeight };
  v2 boxPos = imguiWindow->mCurrCursorDrawPos;

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



