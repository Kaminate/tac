#include "src/common/profile/tacProfile.h"
#include "src/common/profile/tacProfileImGui.h"
#include "src/common/graphics/imgui/tacImGuiState.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/math/tacMath.h"

#include <iostream>

namespace Tac
{

  static const float miniMiliseconds = 0.0f;
  static const float maxiMiliseconds = 100.0f;
  struct ImguiProfileWidgetData
  {
    float mLMiliseconds = miniMiliseconds;
    float mRMiliseconds = 47.0f;
  };

  static const ImGuiindex profileWidgetId =
    ImGuiRegisterWindowResource(
      TAC_STRINGIFY( ImguiProfileWidgetData ),
      &( ImguiProfileWidgetData() ),
      sizeof( ImguiProfileWidgetData ) );

  Vector<ImguiProfileWidgetData> gImguiProfileWidgetDatas;

  static v3 v3mult( v3 a, v3 b )
  {
    return {
      a[ 0 ] * b[ 0 ],
      a[ 1 ] * b[ 1 ],
      a[ 2 ] * b[ 2 ] };
  }
  static v3 v3cos( v3 v )
  {
    return {
      std::cos( v[ 0 ] ),
      std::cos( v[ 1 ] ),
      std::cos( v[ 2 ] ) };
  }

  static v4 GetProfileFunctionColor( ProfileFunction* profileFunction )
  {
    v3 a( 0.8f, 0.5f, 0.4f );
    v3 b( 0.2f, 0.4f, 0.2f );
    v3 c( 2, 1, 1 );
    v3 d( 0, 0.25, 0.25 );

    uint32_t hash = 0;
    for( char c : StringView( profileFunction->mFrame.mFunction ) )
      hash = hash * 31 + c;
    float t = ( std::sin( ( float )hash ) + 1.0f ) / 2.0f;

    v4 boxColor;
    boxColor.xyz() = a + v3mult( b, v3cos( 6.28318f * ( c * t + d ) ) );
    boxColor[ 3 ] = 1;
    return boxColor;
  }

  static void ImGuiProfileWidgetFunction( ImguiProfileWidgetData* profileWidgetData,
                                          UI2DDrawData* drawData,
                                          ProfileFunction* profileFunction,
                                          v2 timelineLeft,
                                          v2 timelineRight,
                                          Timepoint frameBeginTime,
                                          int depth )
  {
    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
    if( !profileFunction )
      return;

    float functionLMiliseconds = TimepointSubtractMiliseconds( profileFunction->mBeginTime, frameBeginTime );
    float functionRMiliseconds = TimepointSubtractMiliseconds( profileFunction->mEndTime, frameBeginTime );
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
      ( float )ImGuiGlobals::Instance.mUIStyle.fontSize
    };
    v2 boxPos = timelineLeft + v2(
      functionLPercent * ( timelineRight.x - timelineLeft.x ),
      ( float )depth * ( ImGuiGlobals::Instance.mUIStyle.fontSize + 5 ) );

    bool boxClipped;
    auto boxClipRect = ImGuiRect::FromPosSize( boxPos, boxSize );
    imguiWindow->ComputeClipInfo( &boxClipped, &boxClipRect );

    profileFunction->mFrame.mFunction;
    v4 boxColor = GetProfileFunctionColor( profileFunction );

    Render::TextureHandle texture;
    drawData->AddBox( boxPos, boxPos + boxSize, boxColor, texture, &boxClipRect );

    v2 textSize = CalculateTextSize(
      profileFunction->mFrame.mFunction,
      ImGuiGlobals::Instance.mUIStyle.fontSize );
    v2 textPos =
    {
      boxPos.x + ( boxSize.x - textSize.x ) / 2,
      boxPos.y,
    };

    bool textClipped;
    auto textClipRect = ImGuiRect::FromPosSize( textPos, textSize );
    imguiWindow->ComputeClipInfo( &textClipped, &textClipRect );


    v4 textColor = v4( v3( 1, 1, 1 ) * ( ( boxColor.x + boxColor.y + boxColor.z / 3.0f ) > 0.5f ? 0.0f : 1.0f ), 1 );
    drawData->AddText( textPos,
                       ImGuiGlobals::Instance.mUIStyle.fontSize,
                       profileFunction->mFrame.mFunction,
                       textColor,
                       &textClipRect );

    ImGuiProfileWidgetFunction( profileWidgetData,
                                drawData,
                                profileFunction->mNext,
                                timelineLeft,
                                timelineRight,
                                frameBeginTime,
                                depth );
    ImGuiProfileWidgetFunction( profileWidgetData,
                                drawData,
                                profileFunction->mChildren,
                                timelineLeft,
                                timelineRight,
                                frameBeginTime,
                                depth + 1 );
  }

  void ImGuiProfileWidget( ProfileFunction* profileFunction )
  {
    if( !profileFunction )
      return;


    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = imguiWindow->mDrawData; // ImGuiGlobals::Instance.mUI2DDrawData;
    auto profileWidgetData = ( ImguiProfileWidgetData* )imguiWindow->GetWindowResource( profileWidgetId );

    static bool outputWindowFrameTimes;
    ImGuiCheckbox( "output window frame times", &outputWindowFrameTimes );
    ImGuiDragFloat( "l miliseconds", &profileWidgetData->mLMiliseconds );
    ImGuiDragFloat( "r miliseconds", &profileWidgetData->mRMiliseconds );
    profileWidgetData->mLMiliseconds = Max( profileWidgetData->mLMiliseconds, miniMiliseconds );
    profileWidgetData->mRMiliseconds = Min( profileWidgetData->mRMiliseconds, maxiMiliseconds );

    // ImGuiBeginGroup();

    float itemWidth = imguiWindow->mContentRect.mMaxi.x - imguiWindow->mCurrCursorViewport.x;
    float itemHeight = ( float )ImGuiGlobals::Instance.mUIStyle.fontSize;

    v2 timeScalePos = imguiWindow->mCurrCursorViewport;
    v2 timeScaleSize = { itemWidth, itemHeight * 3 };
    imguiWindow->ItemSize( timeScaleSize );

    v2 timeScaleOffset = { 0, 8 };

    float timelineVOffset = itemHeight * 1.5f;
    float timelineHOffset = itemHeight * 1.5f;
    v2 timelineLeft = timeScalePos + v2( timelineHOffset, timelineVOffset );
    v2 timelineRight = timeScalePos + v2( itemWidth - timelineHOffset, timelineVOffset );
    v4 timelineColor( 1, 1, 1, 1 );

    drawData->AddLine( timelineLeft, timelineRight, 2, timelineColor );

    TAC_ASSERT( ( int )( profileWidgetData->mRMiliseconds - profileWidgetData->mLMiliseconds ) < 100 );
    for( int i = ( int )profileWidgetData->mLMiliseconds; i < 1 + ( int )profileWidgetData->mRMiliseconds; ++i )
    {
      float t = ( ( float )i - profileWidgetData->mLMiliseconds )
        / ( profileWidgetData->mLMiliseconds + profileWidgetData->mRMiliseconds );
      v2 tickBottom = Lerp( timelineLeft, timelineRight, t );
      v2 tickTop = tickBottom - v2( 0, 10 );
      drawData->AddLine( tickBottom, tickTop, 2.0f, timelineColor );

      StringView timestampSV = Va( "%dms", i );
      String timestamp( timestampSV.data(), timestampSV.size() );

      v2 rSize = CalculateTextSize( timestamp, ImGuiGlobals::Instance.mUIStyle.fontSize );
      drawData->AddText( tickTop - v2( rSize.x / 2.0f, ( float )ImGuiGlobals::Instance.mUIStyle.fontSize ),
                         ImGuiGlobals::Instance.mUIStyle.fontSize,
                         timestamp,
                         ImGuiGlobals::Instance.mUIStyle.textColor, nullptr );
    }

    Timepoint frameBeginTime = profileFunction->mBeginTime;
    ImGuiProfileWidgetFunction( profileWidgetData,
                                drawData,
                                profileFunction,
                                timelineLeft,
                                timelineRight,
                                frameBeginTime,
                                0 );

    if( outputWindowFrameTimes )
    {
      static int iFrame = 0;
      iFrame++;
      int frameMs = ( int )TimepointSubtractMiliseconds( profileFunction->mEndTime, profileFunction->mBeginTime );
      auto str = Va( "frame %-10i %ims\n", iFrame, frameMs );
      std::cout << str.data();
    }
  }




}

