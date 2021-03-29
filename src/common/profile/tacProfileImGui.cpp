#include "src/common/profile/tacProfile.h"
#include "src/common/profile/tacProfileBackend.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/tacOS.h"
#include "src/common/tackeyboardinput.h"
#include "src/common/graphics/imgui/tacImGuiState.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacHash.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/math/tacMath.h"

#include <sstream>
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

  static float             sMilisecondsToDisplay = 20.0f;
  static ProfileTimepoint  sPauseSec;
  //static ProfileFunction*  sPauseMouseHoveredFunction;
  static ProfiledFunctions sProfiledFunctions;
  //static float             sPanSec;

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

  static int CalculateProfileHeight( ProfileFunction* profileFunction )
  {
    if( !profileFunction )
      return 0;

    int childDepthMax = 0;
    for( ProfileFunction* child = profileFunction->mChildren; child; child = child->mNext )
    {
      const int childDepth = CalculateProfileHeight( child );
      childDepthMax = childDepth > childDepthMax ? childDepth : childDepthMax;
    }

    return 1 + childDepthMax;
  }

  static int GetFPS()
  {
    ProfileTimepoint now = ProfileTimepointGet();
    static ProfileTimepoint prev = now;

    const float sec = ProfileTimepointSubtract( now, prev );
    static int frames;

    static int fps;
    frames++;
    if( sec > 0.1f )
    {
      fps = ( int )( ( float )frames / sec );
      frames = 0;
      prev = now;
    }

    //static int    fpsFrameAccum;
    //static double fpsFrameAccumSeconds;
    //static float  fpsFrameAccumSecondsMax = 0.1f;
    //static int    fps;
    //const double  elapsedSeconds = ShellGetElapsedSeconds();
    //const float   fpsDeltaSeconds = ( float )( elapsedSeconds - fpsFrameAccumSeconds );

    //++fpsFrameAccum;
    //if( fpsDeltaSeconds > fpsFrameAccumSecondsMax )
    //{
    //  fps = ( int )( fpsFrameAccum / fpsDeltaSeconds );
    //  fpsFrameAccumSeconds = elapsedSeconds;
    //  fpsFrameAccum = 0;
    //}

    return fps;
  }

  static struct
  {
    int GetProfileThreadCount() { return ( int )sThreadNumberMap.size(); }
    int GetProfileThreadNumber( const std::thread::id threadId )
    {
      //static std::map< std::thread::id, int > sThreadNumberMap;
      auto it = sThreadNumberMap.find( threadId );
      if( it == sThreadNumberMap.end() )
      {
        const int threadNumber = ( int )sThreadNumberMap.size();
        sThreadNumberMap[ threadId ] = threadNumber;
        return threadNumber;
      }
      return ( *it ).second;
    }

    std::map< std::thread::id, int > sThreadNumberMap;
  } sProfileThreadManager;


  static v4 GetProfileFunctionColor( const ProfileFunction* profileFunction )
  {
    const HashedValue hash = HashAddString( profileFunction->mName );
    const float t = std::sin( ( float )hash ) * 0.5f + 0.5f;
    //              a      b                             c          d
    const float r = 0.5f + 0.5f * std::cos( 6.28318f * ( 1.0f * t + 0.0f ) );
    const float g = 0.5f + 0.5f * std::cos( 6.28318f * ( 1.0f * t + 0.33f ) );
    const float b = 0.5f + 0.5f * std::cos( 6.28318f * ( 1.0f * t + 0.66f ) );
    return { r, g, b, 1 };
  }

//   static void ImGuiProfileWidgetFunction( const ImguiProfileWidgetData* profileWidgetData,
//                                           UI2DDrawData* drawData,
//                                           const ProfileFunction* profileFunction,
//                                           const v2 timelineLeft,
//                                           const v2 timelineRight,
//                                           const double frameBeginTime, // Timepoint frameBeginTime,
//                                           const int depth )
//   {
// 
//     ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
//     if( !profileFunction )
//       return;
// 
//     const float functionLMiliseconds = 0;
//     const float functionRMiliseconds = 0;
//     const float functionLPercent
//       = ( functionLMiliseconds - profileWidgetData->mLMiliseconds )
//       / ( profileWidgetData->mRMiliseconds - profileWidgetData->mLMiliseconds );
//     const float functionRPercent
//       = ( functionRMiliseconds - profileWidgetData->mLMiliseconds )
//       / ( profileWidgetData->mRMiliseconds - profileWidgetData->mLMiliseconds );
//     profileWidgetData->mLMiliseconds;
//     profileWidgetData->mRMiliseconds;
// 
//     const v2 boxSize =
//     {
//       ( functionRPercent - functionLPercent ) * ( timelineRight.x - timelineLeft.x ),
//       ( float )ImGuiGlobals::Instance.mUIStyle.fontSize
//     };
//     const v2 boxPos = timelineLeft + v2(
//       functionLPercent * ( timelineRight.x - timelineLeft.x ),
//       ( float )depth * ( ImGuiGlobals::Instance.mUIStyle.fontSize + 5 ) );
// 
//     bool boxClipped;
//     auto boxClipRect = ImGuiRect::FromPosSize( boxPos, boxSize );
//     imguiWindow->ComputeClipInfo( &boxClipped, &boxClipRect );
// 
//     const v4 boxColor = GetProfileFunctionColor( profileFunction );
// 
//     Render::TextureHandle texture;
//     drawData->AddBox( boxPos, boxPos + boxSize, boxColor, texture, &boxClipRect );
// 
//     const v2 textSize = CalculateTextSize( profileFunction->mStackFrame.mFunction,
//                                            ImGuiGlobals::Instance.mUIStyle.fontSize );
//     const v2 textPos ( boxPos.x + ( boxSize.x - textSize.x ) / 2,
//                        boxPos.y );
// 
//     bool textClipped;
//     auto textClipRect = ImGuiRect::FromPosSize( textPos, textSize );
//     imguiWindow->ComputeClipInfo( &textClipped, &textClipRect );
// 
// 
//     v4 textColor = v4( v3( 1, 1, 1 ) * ( ( boxColor.x + boxColor.y + boxColor.z / 3.0f ) > 0.5f ? 0.0f : 1.0f ), 1 );
//     drawData->AddText( textPos,
//                        ImGuiGlobals::Instance.mUIStyle.fontSize,
//                        profileFunction->mStackFrame.mFunction,
//                        textColor,
//                        &textClipRect );
// 
//     ImGuiProfileWidgetFunction( profileWidgetData,
//                                 drawData,
//                                 profileFunction->mNext,
//                                 timelineLeft,
//                                 timelineRight,
//                                 frameBeginTime,
//                                 depth );
//     ImGuiProfileWidgetFunction( profileWidgetData,
//                                 drawData,
//                                 profileFunction->mChildren,
//                                 timelineLeft,
//                                 timelineRight,
//                                 frameBeginTime,
//                                 depth + 1 );
//   }

  void ImGuiProfileWidgetCamera( const v2 cameraViewportPos,
                                 const v2 cameraViewportSize )
  {
    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = imguiWindow->mDrawData;


    //int iProfiledFunction = 0;

    drawData->AddBox( cameraViewportPos,
                      cameraViewportPos + cameraViewportSize,
                      v4( 0.2f, 0, 0, 1 ), Render::TextureHandle(), nullptr );

    struct IndexedThreadProfileData
    {
      const ProfiledFunctionList* mProfiledFunctionList;
      int                         mTreeHeight;
    } indexedThreadProfileDatas[ 10 ] = {};
    //int maxDepths[ 10 ] = {};

    for( auto& pair : sProfiledFunctions )
    {
      const ProfiledFunctionList& profiledFunctionList = pair.second;
      const int iThread = sProfileThreadManager.GetProfileThreadNumber( pair.first );
      int treeHeight = 0;
      for( ProfileFunction* profileFunction : profiledFunctionList )
      {
        const int depth = CalculateProfileHeight( profileFunction );
        treeHeight = depth > treeHeight ? depth : treeHeight;
      }
      IndexedThreadProfileData* indexedThreadProfileData = &indexedThreadProfileDatas[ iThread ];
      indexedThreadProfileData->mProfiledFunctionList = &profiledFunctionList;
      indexedThreadProfileData->mTreeHeight = treeHeight;
    }

    float threadY = cameraViewportPos.y;
    const float boxHeight = ( float )ImGuiGlobals::Instance.mUIStyle.fontSize;

    struct ProfileFunctionVisitor
    {
      ProfileFunction* mProfileFunction;
      int              mDepth;
    };
    static Vector< ProfileFunctionVisitor > visitors;
    //visitors.clear();


    for( int iThread = 0; iThread < sProfileThreadManager.GetProfileThreadCount(); ++iThread )
    {
      IndexedThreadProfileData* indexedThreadProfileData = &indexedThreadProfileDatas[ iThread ];

      TAC_ON_DESTRUCT(
        threadY += Max( indexedThreadProfileData->mTreeHeight, 1 ) * boxHeight
        + 5;// padding between thread call stacks
      );

      for( ProfileFunction* profileFunction : *indexedThreadProfileData->mProfiledFunctionList )
        visitors.push_back( { profileFunction, 0 } );


      const ProfileTimepoint gameBeginTimepoint = ProfileTimepointGetLastGameFrameBegin();

      const float timeScaleSeconds = sMilisecondsToDisplay / 1000.0f;
      //const double timeScaleSecRight = ProfileGetIsRuning() ? ShellGetElapsedSeconds() : sPauseSec;
      //const double timeScaleSecLeft = timeScaleSecRight - ( double )timeScaleSeconds;

      const ProfileTimepoint timepointLeft = ProfileGetIsRuning() ? gameBeginTimepoint : sPauseSec;

      //ProfileFunction* hovered = nullptr;

      while( !visitors.empty() )
      {
        ProfileFunctionVisitor visitor = visitors.back();
        ProfileFunction* profileFunction = visitor.mProfileFunction;
        visitors.pop_back();

        if( profileFunction->mNext )
          visitors.push_back( { profileFunction->mNext, visitor.mDepth } );
        if( profileFunction->mChildren )
          visitors.push_back( { profileFunction->mChildren, visitor.mDepth + 1 } );


//          if( true && !StrCmp( profileFunction->mName, "wait render" ) )
//          {
//            ProfileFunction* parent = profileFunction->mParent;
//            if( parent )
//            {
//  
//  
//              float waitBeginSec = ProfileTimepointSubtract( profileFunction->mBeginTime, parent->mBeginTime );
//              if( waitBeginSec > 10.0f )
//                waitBeginSec = 10.0f;
//  
//  
//              float waitEndSec = ProfileTimepointSubtract( parent->mEndTime, profileFunction->mEndTime );
//              if( waitEndSec > 10.0f )
//                waitEndSec = 10.0f;
//  
//              static int asdf;
//              ++asdf;
//            }
//          }


        const float     boxDeltaSeconds = ProfileTimepointSubtract( profileFunction->mEndTime,
                                                                    profileFunction->mBeginTime );
        const float     boxDeltaPercent = boxDeltaSeconds / timeScaleSeconds;
        // Add a Min() to ensure things are still visible
        const float     boxWidthPx = Max( boxDeltaPercent * cameraViewportSize.x, 10.0f );
        const float     boxXMinPercent = ProfileTimepointSubtract( profileFunction->mBeginTime,
                                                                   timepointLeft ) / timeScaleSeconds;
        //const float     boxXMaxPercent = ProfileTimepointSubtract( profileFunction->mEndTime,
        //                                                           timepointLeft ) / timeScaleSeconds;
        const float     boxXMinPx = cameraViewportPos.x + cameraViewportSize.x * boxXMinPercent;
        const float     boxXMaxPx = boxXMinPx + boxWidthPx;
        //const float     boxXMaxPx = boxXMinPx + boxWidthPx;
        const float     boxY = threadY + visitor.mDepth * boxHeight;
        const v2        boxMin = { boxXMinPx, boxY };
        const v2        boxMax = { boxXMaxPx, boxY + boxHeight };
        const ImGuiRect boxRect = ImGuiRect::FromMinMax( boxMin, boxMax );
        const v4        boxColor = GetProfileFunctionColor( profileFunction );

        const StringView textPrefix( "Tac::" );
        const char*      textFunction = profileFunction->mName;
        const v4         textColor( 0, 0, 0, 1 );
        const char*      text = StringView( textFunction ).starts_with( textPrefix )
          ? textFunction + textPrefix.size()
          : textFunction;


        if( imguiWindow->IsHovered( ImGuiRect::FromMinMax( boxMin, boxMax ) ) )
        {
          const v2 textSize = CalculateTextSize( profileFunction->mName,
                                                 ImGuiGlobals::Instance.mUIStyle.fontSize );
          //ImGuiSetNextWindowHandle( DesktopWindowHandle() );
          ImGuiSetNextWindowPosition( gKeyboardInput.mCurr.mScreenspaceCursorPos );
          ImGuiSetNextWindowSize( textSize + v2( 1, 1 ) * 50.0f );
          ImGuiBegin( "hovered" );
          ImGuiText( profileFunction->mName );
          ImGuiEnd();
          //ImGuiSetNextWindowHandle()
        }

          //hovered = profileFunction;
        //imguiWindow->IsHovered();

        drawData->AddBox( boxMin, boxMax, boxColor, Render::TextureHandle(), nullptr );
        drawData->AddText( boxMin, ImGuiGlobals::Instance.mUIStyle.fontSize,
                           text,
                           textColor,
                           &boxRect );
      }
    }

    //if( hovered )
    //{

    //}

    // Pan camera
    {
      static double mouseMovement;
      const ImGuiRect viewRect = ImGuiRect::FromPosSize( cameraViewportPos, cameraViewportSize );
      //const v2        viewMouse = imguiWindow->GetMousePosViewport();
      if( imguiWindow->IsHovered( viewRect ) ) // viewRect.ContainsPoint( viewMouse ) )
      {
        TryConsumeMouseMovement( &mouseMovement, TAC_STACK_FRAME );
        if( mouseMovement )
        {
          if( gKeyboardInput.IsKeyDown( Key::MouseLeft ) )
          {
            const float movePixels = gKeyboardInput.mMouseDeltaPos.x;
            const float movePercent = movePixels / cameraViewportSize.x;
            const float moveSeconds = movePercent * ( sMilisecondsToDisplay / 1000 );
            //sPanSec += moveSeconds;
            sPauseSec = ProfileTimepointAddSeconds( sPauseSec, -moveSeconds );
          }
          //if( gKeyboardInput.mMouseDeltaScroll )
          //{
          sMilisecondsToDisplay -= gKeyboardInput.mMouseDeltaScroll * 0.4f;
        //}
        }
      }
    }
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


    //const ProfileTimepoint gameBeginTimepoint = ProfileTimepointGetLastGameFrameBegin();
    //const ProfileTimepoint timeScaleSec = ProfileGetIsRuning() ? gameBeginTimepoint : sPauseSec;
    //const double timeScaleMs = timeScaleSec * 1000;
    //const double rightTickMs = std::floor( timeScaleMs );
    //const float rightTickMsOffset = ( float )( timeScaleMs - rightTickMs ) * pxPerMs;


    for( int iMs = 0; iMs < ( int )sMilisecondsToDisplay; ++iMs )
    {
      const float msOffset =
        //rightTickMsOffset + 
        pxPerMs * iMs;

      const v2    tickBot( timelinePos.x +/* timelineSize.x -*/ msOffset,
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
    //drawData->AddText( timelinePos, 12, FormatFrameTime( timeScaleSec ), v4( 1, 1, 1, 1 ), nullptr );

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

    //int iProfiledFunction = 0;


    const int   fps = GetFPS();
    const char* fpsText = FrameMemoryPrintf( "FPS: %i", fps );
    const float fpsT = Saturate( ( float )fps / 400.0f ); // 1 = fast, 0 = slow
    const v2    fpsBoxMin( imguiWindow->mCurrCursorViewport.x + 80.0f,
                           imguiWindow->mCurrCursorViewport.y + 7.0f );
    const v2    fpsBoxMax( fpsBoxMin.x + ( imguiWindow->mContentRect.mMaxi.x - fpsBoxMin.x ) * fpsT,
                           fpsBoxMin.y + 2.0f );
    const v4    fpsColor( Lerp( 1.0f, 0.0f, fpsT ),
                          Lerp( 0.0f, 1.0f, fpsT ),
                          0,
                          1 );
    imguiWindow->mDrawData->AddBox( fpsBoxMin, fpsBoxMax, fpsColor, Render::TextureHandle(), nullptr );

    ImGuiText( fpsText );

    if( ProfileGetIsRuning() )
    {
      const bool hasData = !sProfiledFunctions.empty();
      if( hasData )
        ProfiledFunctionFree( sProfiledFunctions );


      sProfiledFunctions = ProfiledFunctionCopy();

      if( ImGuiButton( "Pause Profiling" ) )
      {
        ProfileSetIsRuning( false );
        sPauseSec = ProfileTimepointGetLastGameFrameBegin();
        //sPanSec = 0;
        // sPauseSec = ShellGetElapsedSeconds();
      }
    }
    else
    {

      //sPauseMouseHoveredFunction

      if( ImGuiButton( "Resume Profiling" ) )
      {
        ProfileSetIsRuning( true );
      }
    }


    //profileWidgetData->mLMiliseconds = Max( profileWidgetData->mLMiliseconds, miniMiliseconds );
    //profileWidgetData->mRMiliseconds = Min( profileWidgetData->mRMiliseconds, maxiMiliseconds );


    const v2    timelinePos = imguiWindow->mCurrCursorViewport;
    const v2    timelineSize = v2( imguiWindow->mContentRect.mMaxi.x - imguiWindow->mCurrCursorViewport.x,
                                   ImGuiGlobals::Instance.mUIStyle.fontSize * 3.0f );
    imguiWindow->mCurrCursorViewport.y += timelineSize.y;

    const v2    cameraViewportPos = imguiWindow->mCurrCursorViewport;
    const v2    cameraViewportSize = imguiWindow->mContentRect.mMaxi - cameraViewportPos;
    imguiWindow->mCurrCursorViewport.y += timelineSize.y;

    ImGuiProfileWidgetTimeScale( timelinePos, timelineSize );
    ImGuiProfileWidgetCamera( cameraViewportPos, cameraViewportSize );
  }
}

