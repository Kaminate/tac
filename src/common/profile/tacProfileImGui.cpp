#include "src/common/profile/tacProfile.h"
#include "src/common/profile/tacProfileBackend.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/tacOS.h"
#include "src/common/graphics/imgui/tacImGuiState.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacHash.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/math/tacMath.h"

#include <iostream>
#include <math.h>

// remove these when finished debugging
#pragma warning( disable: 4505 )
#pragma warning( disable: 4189 )
#pragma warning( disable: 4100 )

namespace Tac
{
  //static double sElapsedSecondsWhenPaused;
  //static bool   sIsPaused;
  //static float  sSecondOffset;
  //static float  sZoomLevel;

  static float sMilisecondsToDisplay = 20.0f;

  // need to be able to zoom in at mouse cursor, and pan around
  //
  //
  // mRMiliseconds = curtime + offset



  //               When not paused
  //                                                  current elapsed seconds
  //                                                      |
  //    +-------------------+-----------------------------+---------> x axis ( time )
  //    |                   |                             |
  //    |                   |                             |
  //    |                   |           camera            |
  //    |                   |                             |
  //    |                   |                             |
  //    |                   +-----------------------------+
  //    |                                                 |
  //    |                                                 |
  //    |                                                 |
  //    |                                                 v
  //    |
  //    v
  // y axis ( pixels )
  // 
  //
  //


  //static float  sCameraSecondOffset;



  //static const float miniMiliseconds = 0.0f;
  //static const float maxiMiliseconds = 100.0f;
  struct ImguiProfileWidgetData
  {
    //float mLMiliseconds = miniMiliseconds;
    //float mRMiliseconds = 47.0f;
  };

  static ImguiProfileWidgetData sDefaultWidgetData;
  static const ImGuiIndex       sWidgetID = ImGuiRegisterWindowResource( TAC_STRINGIFY( ImguiProfileWidgetData ),
                                                                         &sDefaultWidgetData,
                                                                         sizeof( ImguiProfileWidgetData ) );


  static v4 GetProfileFunctionColor( const ProfileFunction* profileFunction )
  {
    const HashedValue hash = HashAddString( profileFunction->mStackFrame.mFunction );
    const float t = ( std::sin( ( float )hash ) + 1.0f ) / 2.0f;
    const float r = 0.8f + 0.2f * std::cos( 6.28318f * ( 2.0f * t + 0.0f ) );
    const float g = 0.5f + 0.4f * std::cos( 6.28318f * ( 1.0f * t + 0.25f ) );
    const float b = 0.4f + 0.2f * std::cos( 6.28318f * ( 1.0f * t + 0.25f ) );
    return { r, g, b, 1 };
  }

  static void ImGuiProfileWidgetFunction( const ImguiProfileWidgetData* profileWidgetData,
                                          UI2DDrawData* drawData,
                                          const ProfileFunction* profileFunction,
                                          const v2 timelineLeft,
                                          const v2 timelineRight,
                                          const double frameBeginTime, // Timepoint frameBeginTime,
                                          const int depth )
  {

    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
    if( !profileFunction )
      return;

    //const float functionLMiliseconds = 0;
    //const float functionRMiliseconds = 0;
    //const float functionLPercent
    //  = ( functionLMiliseconds - profileWidgetData->mLMiliseconds )
    //  / ( profileWidgetData->mRMiliseconds - profileWidgetData->mLMiliseconds );
    //const float functionRPercent
    //  = ( functionRMiliseconds - profileWidgetData->mLMiliseconds )
    //  / ( profileWidgetData->mRMiliseconds - profileWidgetData->mLMiliseconds );
    //profileWidgetData->mLMiliseconds;
    //profileWidgetData->mRMiliseconds;

    //const v2 boxSize =
    //{
    //  ( functionRPercent - functionLPercent ) * ( timelineRight.x - timelineLeft.x ),
    //  ( float )ImGuiGlobals::Instance.mUIStyle.fontSize
    //};
    //const v2 boxPos = timelineLeft + v2(
    //  functionLPercent * ( timelineRight.x - timelineLeft.x ),
    //  ( float )depth * ( ImGuiGlobals::Instance.mUIStyle.fontSize + 5 ) );

    //bool boxClipped;
    //auto boxClipRect = ImGuiRect::FromPosSize( boxPos, boxSize );
    //imguiWindow->ComputeClipInfo( &boxClipped, &boxClipRect );

    //const v4 boxColor = GetProfileFunctionColor( profileFunction );

    //Render::TextureHandle texture;
    //drawData->AddBox( boxPos, boxPos + boxSize, boxColor, texture, &boxClipRect );

    //const v2 textSize = CalculateTextSize( profileFunction->mStackFrame.mFunction,
    //                                       ImGuiGlobals::Instance.mUIStyle.fontSize );
    //const v2 textPos ( boxPos.x + ( boxSize.x - textSize.x ) / 2,
    //                   boxPos.y );

    //bool textClipped;
    //auto textClipRect = ImGuiRect::FromPosSize( textPos, textSize );
    //imguiWindow->ComputeClipInfo( &textClipped, &textClipRect );


    //v4 textColor = v4( v3( 1, 1, 1 ) * ( ( boxColor.x + boxColor.y + boxColor.z / 3.0f ) > 0.5f ? 0.0f : 1.0f ), 1 );
    //drawData->AddText( textPos,
    //                   ImGuiGlobals::Instance.mUIStyle.fontSize,
    //                   profileFunction->mStackFrame.mFunction,
    //                   textColor,
    //                   &textClipRect );

    //ImGuiProfileWidgetFunction( profileWidgetData,
    //                            drawData,
    //                            profileFunction->mNext,
    //                            timelineLeft,
    //                            timelineRight,
    //                            frameBeginTime,
    //                            depth );
    //ImGuiProfileWidgetFunction( profileWidgetData,
    //                            drawData,
    //                            profileFunction->mChildren,
    //                            timelineLeft,
    //                            timelineRight,
    //                            frameBeginTime,
    //                            depth + 1 );
  }

  void ImGuiProfileWidgetCamera( const v2 cameraViewportPos,
                                 const v2 cameraViewportSize )
  {
    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = imguiWindow->mDrawData;
    //ImGuiProfileWidgetFunction( profileWidgetData,
    //                            drawData,
    //                            profileFunction,
    //                            timelineLeft,
    //                            timelineRight,
    //                            profileFunction->mBeginTime,
    //                            0 );

    drawData->AddBox( cameraViewportPos,
                      cameraViewportPos + cameraViewportSize,
                      v4( 1, 1, 0, 1 ), Render::TextureHandle(), nullptr );
  }


  void ImGuiProfileWidgetTimeScale( const v2 timelinePos,
                                    const v2 timelineSize )
  {
    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = imguiWindow->mDrawData;

    drawData->AddBox( timelinePos,
                      timelinePos + timelineSize,
                      v4( 1, 0, 0, 1 ), Render::TextureHandle(), nullptr );

    const float pxPerMs = timelineSize.x / sMilisecondsToDisplay;


    const double elapsedMs = ShellGetElapsedSeconds() * 1000;
    const double rightMs = std::floor( elapsedMs );

    const float rightMsOffset = ( float )( elapsedMs - rightMs ) * pxPerMs;

    for( int iMs = 0; iMs < ( int )sMilisecondsToDisplay; ++iMs )
    {
      const float msOffset = rightMsOffset + pxPerMs * iMs;

      const v2    tickBot( timelinePos.x + timelineSize.x - msOffset,
                           timelinePos.y + timelineSize.y );
      const v2    tickTop = tickBot - v2( 0, 10 );
      const v4    tickColor( 1, 1, 1, 1 );
      const float tickRadiusPx = 2.0f;

      const char* text = FrameMemoryPrintf( "%i", iMs );
      const v2    textSize = CalculateTextSize( text, ImGuiGlobals::Instance.mUIStyle.fontSize );
      const v2    textPos( tickTop.x - textSize.x / 2,
                           tickTop.y - textSize.y );

      drawData->AddLine( tickBot, tickTop, tickRadiusPx, tickColor );

      drawData->AddText( textPos,
                         ImGuiGlobals::Instance.mUIStyle.fontSize,
                         text,
                         ImGuiGlobals::Instance.mUIStyle.textColor, nullptr );
    }

    //const float timelineVOffset = timelineSize.y * 1.5f;
    //const float timelineHOffset = timelineSize.y * 1.5f;
    //const v2    timelineLeft = timelinePos + v2( timelineHOffset, timelineVOffset );
    //const v2    timelineRight = timelinePos + v2( timelineWidth - timelineHOffset, timelineVOffset );
    //const v4    timelineColor( 1, 1, 1, 1 );

    //drawData->AddLine( timelineLeft, timelineRight, 2, timelineColor );

    //for( int i = ( int )profileWidgetData->mLMiliseconds; i < 1 + ( int )profileWidgetData->mRMiliseconds; ++i )
    //{
    //  const float t = ( ( float )i - profileWidgetData->mLMiliseconds )
    //    / ( profileWidgetData->mLMiliseconds + profileWidgetData->mRMiliseconds );
    //  const v2 tickBottom = Lerp( timelineLeft, timelineRight, t );
    //  const v2 tickTop = tickBottom - v2( 0, 10 );
    //  drawData->AddLine( tickBottom, tickTop, 2.0f, timelineColor );

    //  const StringView timestampSV = FrameMemoryPrintf( "%dms", i );

    //  const v2 rSize = CalculateTextSize( timestampSV, ImGuiGlobals::Instance.mUIStyle.fontSize );
    //  drawData->AddText( tickTop - v2( rSize.x / 2.0f, ( float )ImGuiGlobals::Instance.mUIStyle.fontSize ),
    //                     ImGuiGlobals::Instance.mUIStyle.fontSize,
    //                     timestampSV,
    //                     ImGuiGlobals::Instance.mUIStyle.textColor, nullptr );
    //}
  }

  void ImGuiProfileWidget()
  {
    //ProfilePerThreadData* ProfilePerThreadData = ProfilePerThreadData::Instance;
    //ProfileFunction* profileFunction = ProfilePerThreadData->mLastFrame
    //  if( !profileFunction )
    //    return;

    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
    auto profileWidgetData = ( ImguiProfileWidgetData* )imguiWindow->GetWindowResource( sWidgetID );

    //static bool outputWindowFrameTimes;
    //ImGuiCheckbox( "output window frame times", &outputWindowFrameTimes );
    //ImGuiDragFloat( "l miliseconds", &profileWidgetData->mLMiliseconds );
    //ImGuiDragFloat( "r miliseconds", &profileWidgetData->mRMiliseconds );


    //static double lastElapsed;
    //const double currElapsed = ShellGetElapsedSeconds();
    //const float frameDeltaSeconds = ( float )( currElapsed - lastElapsed );
    //if( frameDeltaSeconds * 1000.0f < 1.0f )
      //return;


    static int    frameAccum;
    static double frameAccumSeconds;
    static int    fps;

    ++frameAccum;
    if( frameAccum == 100 )
    {
      const double elapsedSeconds = ShellGetElapsedSeconds();
      const float deltaSeconds = ( float)(elapsedSeconds - frameAccumSeconds);
      fps = deltaSeconds > 0 ? ( int )( frameAccum / deltaSeconds ) : 0;
      frameAccumSeconds = elapsedSeconds;
      frameAccum = 0;
    }


    //lastElapsed = currElapsed;
    //const int fps = frameDeltaSeconds > 0.0f ? (int)(1.0f / frameDeltaSeconds) : 0;
    //ImGuiText( FrameMemoryPrintf( "FPS: %i", fps ) );
    ImGuiText( FrameMemoryPrintf( "%i", fps ) );
    //profileWidgetData->mLMiliseconds = Max( profileWidgetData->mLMiliseconds, miniMiliseconds );
    //profileWidgetData->mRMiliseconds = Min( profileWidgetData->mRMiliseconds, maxiMiliseconds );


    const v2    timelinePos = imguiWindow->mCurrCursorViewport;
    const v2    timelineSize = v2( imguiWindow->mContentRect.mMaxi.x - imguiWindow->mCurrCursorViewport.x,
                                   ImGuiGlobals::Instance.mUIStyle.fontSize * 3.0f );

    const v2    cameraViewportPos = timelinePos + v2( 0, timelineSize.y );
    const v2    cameraViewportSize = imguiWindow->mContentRect.mMaxi - cameraViewportPos;


    //ImGuiProfileWidgetTimeScale( timelinePos, timelineSize );
    //ImGuiProfileWidgetCamera( cameraViewportPos, cameraViewportSize );
  }
}

