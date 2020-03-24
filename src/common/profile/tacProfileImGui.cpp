
#include "common/profile/tacProfile.h"
#include "common/profile/tacProfileImGui.h"
#include "common/graphics/imgui/tacImGuiState.h"
#include "common/graphics/tacUI2D.h"
#include "common/math/tacMath.h"


static const float miniMiliseconds = 0.0f;
static const float maxiMiliseconds = 100.0f;
struct TacImguiProfileWidgetData : public TacImGuiWindowResource
{
  float mLMiliseconds = miniMiliseconds;
  //float mRMiliseconds = 17.0f;
  float mRMiliseconds = 47.0f;
};

TacVector<TacImguiProfileWidgetData> gImguiProfileWidgetDatas;

static v3 v3mult( v3 a, v3 b )
{
  return {
    a[0] * b[0],
    a[1] * b[1],
    a[2] * b[2] };
}
static v3 v3cos( v3 v )
{
  return {
    std::cos( v[ 0 ] ),
    std::cos( v[ 1 ] ),
    std::cos( v[ 2 ] ) };
}

static v4 GetTacProfileFunctionColor( TacProfileFunction* profileFunction )
{
  v3 a( 0.8f, 0.5f, 0.4f );
  v3 b( 0.2f, 0.4f, 0.2f );
  v3 c( 2, 1, 1 );
  v3 d( 0, 0.25, 0.25 );

  uint32_t hash  = 0;
  for( char c : profileFunction->mStackFrame.mFunction )
    hash = hash * 31 + c;
  float t = ( std::sin( ( float )hash ) + 1.0f ) / 2.0f;

  v4 boxColor;
  boxColor.xyz() = a + v3mult( b, v3cos( 6.28318f * ( c * t + d ) ) );
  boxColor[ 3 ] = 1;
  return boxColor;
}

static void TacImGuiProfileWidgetFunction(
  TacImguiProfileWidgetData* profileWidgetData,
  TacProfileFunction* profileFunction,
  v2 timelineLeft,
  v2 timelineRight,
  TacTimepoint frameBeginTime,
  int depth )
{
  TacUI2DDrawData* drawData = TacImGuiGlobals::Instance.mUI2DDrawData;
  TacImGuiWindow* imguiWindow = TacImGuiGlobals::Instance.mCurrentWindow;
  if( !profileFunction )
    return;

  float functionLMiliseconds = TacTimepointSubtractMiliseconds( profileFunction->mBeginTime, frameBeginTime );
  float functionRMiliseconds = TacTimepointSubtractMiliseconds( profileFunction->mEndTime, frameBeginTime );
  float functionLPercent
    = ( functionLMiliseconds - profileWidgetData->mLMiliseconds )
    / ( profileWidgetData->mRMiliseconds - profileWidgetData->mLMiliseconds );
  float functionRPercent
    = ( functionRMiliseconds - profileWidgetData->mLMiliseconds )
    / ( profileWidgetData->mRMiliseconds - profileWidgetData->mLMiliseconds );
  profileWidgetData->mLMiliseconds;
  profileWidgetData->mRMiliseconds;

  v2 boxSize =
  {
    ( functionRPercent - functionLPercent ) * ( timelineRight.x - timelineLeft.x ),
    ( float )TacImGuiGlobals::Instance.mUIStyle.fontSize
  };
  v2 boxPos = timelineLeft + v2(
    functionLPercent * ( timelineRight.x - timelineLeft.x ),
    ( float )depth * ( TacImGuiGlobals::Instance.mUIStyle.fontSize + 5 ) );

  bool boxClipped;
  auto boxClipRect = TacImGuiRect::FromPosSize( boxPos, boxSize );
  imguiWindow->ComputeClipInfo( &boxClipped, &boxClipRect );
  
  profileFunction->mStackFrame.mFunction;
  v4 boxColor = GetTacProfileFunctionColor(profileFunction);;
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


  v4 textColor = v4( v3(1, 1, 1 ) * ( ( boxColor.x + boxColor.y + boxColor.z / 3.0f ) > 0.5f ? 0.0f : 1.0f ), 1);
  drawData->AddText(
    textPos,
    TacImGuiGlobals::Instance.mUIStyle.fontSize,
    profileFunction->mStackFrame.mFunction,
    textColor,
    &textClipRect );

  TacImGuiProfileWidgetFunction(
    profileWidgetData,
    profileFunction->mNext,
    timelineLeft,
    timelineRight,
    frameBeginTime,
    depth );
  TacImGuiProfileWidgetFunction(
    profileWidgetData,
    profileFunction->mChildren,
    timelineLeft,
    timelineRight,
    frameBeginTime,
    depth + 1 );
}

void TacImGuiProfileWidget( TacProfileFunction* profileFunction )
{
  if( !profileFunction )
    return;


  TacUI2DDrawData* drawData = TacImGuiGlobals::Instance.mUI2DDrawData;
  TacImGuiWindow* imguiWindow = TacImGuiGlobals::Instance.mCurrentWindow;
  auto profileWidgetData = imguiWindow->GetOrCreateResource<TacImguiProfileWidgetData>();

  static bool outputWindowFrameTimes;
  TacImGuiCheckbox("output window frame times", &outputWindowFrameTimes);
  TacImGuiDragFloat( "l miliseconds", &profileWidgetData->mLMiliseconds );
  TacImGuiDragFloat( "r miliseconds", &profileWidgetData->mRMiliseconds );
  profileWidgetData->mLMiliseconds = TacMax( profileWidgetData->mLMiliseconds, miniMiliseconds );
  profileWidgetData->mRMiliseconds = TacMin( profileWidgetData->mRMiliseconds, maxiMiliseconds );

  // TacImGuiBeginGroup();

  float itemWidth = imguiWindow->mContentRect.mMaxi.x - imguiWindow->mCurrCursorDrawPos.x;
  float itemHeight = ( float )TacImGuiGlobals::Instance.mUIStyle.fontSize;

  v2 timeScalePos = imguiWindow->mCurrCursorDrawPos;
  v2 timeScaleSize = { itemWidth, itemHeight * 3 };
  imguiWindow->ItemSize(timeScaleSize);
  
  v2 timeScaleOffset = { 0, 8 };

  float timelineVOffset = itemHeight * 1.5f ;
  float timelineHOffset = itemHeight * 1.5f ;
  v2 timelineLeft = timeScalePos + v2( timelineHOffset, timelineVOffset);
  v2 timelineRight = timeScalePos + v2( itemWidth - timelineHOffset, timelineVOffset );
  v4 timelineColor( 1, 1, 1, 1 );
   
  drawData->AddLine( timelineLeft, timelineRight, 2, timelineColor );

  TacAssert( ( int )( profileWidgetData->mRMiliseconds - profileWidgetData->mLMiliseconds ) < 100 );
  for( int i = ( int )profileWidgetData->mLMiliseconds; i < 1 + ( int )profileWidgetData->mRMiliseconds; ++i )
  {
    float t = ( ( float )i - profileWidgetData->mLMiliseconds )
      / ( profileWidgetData->mLMiliseconds + profileWidgetData->mRMiliseconds );
    v2 tickBottom = TacLerp( timelineLeft, timelineRight, t );
    v2 tickTop = tickBottom - v2(0, 10);
    drawData->AddLine(tickBottom, tickTop, 2.0f, timelineColor);

    TacStringView timestampSV = TacVa( "%dms", i );
    TacString timestamp( timestampSV.data(), timestampSV.size() );

    v2 rSize = drawData->CalculateTextSize(timestamp, TacImGuiGlobals::Instance.mUIStyle.fontSize);
    drawData->AddText(
      tickTop - v2(rSize.x / 2.0f, TacImGuiGlobals::Instance.mUIStyle.fontSize),
      TacImGuiGlobals::Instance.mUIStyle.fontSize,
      timestamp,
      TacImGuiGlobals::Instance.mUIStyle.textColor, nullptr );
  }

  TacTimepoint frameBeginTime = profileFunction->mBeginTime;
  TacImGuiProfileWidgetFunction(
    profileWidgetData,
    profileFunction,
    timelineLeft,
    timelineRight,
    frameBeginTime,
    0 );

  if(outputWindowFrameTimes)
  {
    static int iFrame = 0;
    iFrame++;
    int frameMs = (int)TacTimepointSubtractMiliseconds(profileFunction->mEndTime, profileFunction->mBeginTime );
    auto str = TacVa("frame %-10i %ims\n", iFrame, frameMs);
    std::cout << str.data();
  }
}



