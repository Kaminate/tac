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

namespace Tac
{
  static float             sMilisecondsToDisplay = 20.0f;
  static ProfileTimepoint  sPauseSec;
  static ProfiledFunctions sProfiledFunctions;

  // Totally unused, but keeping it because its the only place where ImGuiRegisterWindowResource is used
  // It would be gotten like this:
  //
  //   ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
  //   auto profileWidgetData = ( ImguiProfileWidgetData* )imguiWindow->GetWindowResource( sWidgetID );
  //
  static const struct ImguiProfileWidgetData
  {
  } sDefaultWidgetData;
  static const ImGuiIndex sWidgetID = ImGuiRegisterWindowResource( TAC_STRINGIFY( ImguiProfileWidgetData ),
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

    return fps;
  }

  static struct
  {
    int GetProfileThreadCount()
    {
      return ( int )sThreadNumberMap.size();
    }
    int GetProfileThreadNumber( const std::thread::id threadId )
    {
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

  static void ImGuiProfileWidgetCamera( const v2 cameraViewportPos,
                                 const v2 cameraViewportSize )
  {
    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = imguiWindow->mDrawData;

    drawData->AddBox( cameraViewportPos,
                      cameraViewportPos + cameraViewportSize,
                      v4( 0.2f, 0, 0, 1 ), Render::TextureHandle(), nullptr );

    struct IndexedThreadProfileData
    {
      const ProfiledFunctionList* mProfiledFunctionList;
      int                         mTreeHeight;
    } indexedThreadProfileDatas[ 10 ] = {};

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

    for( int iThread = 0; iThread < sProfileThreadManager.GetProfileThreadCount(); ++iThread )
    {
      IndexedThreadProfileData* indexedThreadProfileData = &indexedThreadProfileDatas[ iThread ];

      TAC_ON_DESTRUCT(
        threadY += Max( indexedThreadProfileData->mTreeHeight, 1 ) * boxHeight
        + 5;// padding between thread call stacks
      );

      for( ProfileFunction* profileFunction : *indexedThreadProfileData->mProfiledFunctionList )
        visitors.push_back( { profileFunction, 0 } );


      const float timeScaleSeconds = sMilisecondsToDisplay / 1000.0f;
      const ProfileTimepoint gameBeginTimepoint = ProfileTimepointGetLastGameFrameBegin();
      const ProfileTimepoint timepointLeft = ProfileGetIsRuning() ? gameBeginTimepoint : sPauseSec;

      while( !visitors.empty() )
      {
        ProfileFunctionVisitor visitor = visitors.back();
        ProfileFunction* profileFunction = visitor.mProfileFunction;
        visitors.pop_back();

        if( profileFunction->mNext )
          visitors.push_back( { profileFunction->mNext, visitor.mDepth } );
        if( profileFunction->mChildren )
          visitors.push_back( { profileFunction->mChildren, visitor.mDepth + 1 } );


        const float     boxDeltaSeconds = ProfileTimepointSubtract( profileFunction->mEndTime,
                                                                    profileFunction->mBeginTime );
        const float     boxDeltaPercent = boxDeltaSeconds / timeScaleSeconds;
        //                           Max() ensures things are still visible
        const float     boxWidthPx = Max( boxDeltaPercent * cameraViewportSize.x, 10.0f );
        const float     boxXMinPercent = ProfileTimepointSubtract( profileFunction->mBeginTime,
                                                                   timepointLeft ) / timeScaleSeconds;
        const float     boxXMinPx = cameraViewportPos.x + cameraViewportSize.x * boxXMinPercent;
        const float     boxXMaxPx = boxXMinPx + boxWidthPx;
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
          const float boxDeltaMsec = ( float )( boxDeltaSeconds * 1000 );
          ImGuiSetNextWindowPosition( gKeyboardInput.mCurr.mScreenspaceCursorPos );
          ImGuiSetNextWindowSize( textSize + v2( 1, 1 ) * 50.0f );
          ImGuiBegin( "hovered" );
          ImGuiText( profileFunction->mName );
          ImGuiText( FrameMemoryPrintf( "msec: %.1f", boxDeltaMsec ) );
          ImGuiEnd();
        }

        drawData->AddBox( boxMin, boxMax, boxColor, Render::TextureHandle(), nullptr );
        drawData->AddText( boxMin, ImGuiGlobals::Instance.mUIStyle.fontSize,
                           text,
                           textColor,
                           &boxRect );
      }
    }

    // Pan camera
    {
      static double mouseMovement;
      const ImGuiRect viewRect = ImGuiRect::FromPosSize( cameraViewportPos, cameraViewportSize );
      if( imguiWindow->IsHovered( viewRect ) )
      {
        TryConsumeMouseMovement( &mouseMovement, TAC_STACK_FRAME );
        if( mouseMovement )
        {
          if( gKeyboardInput.IsKeyDown( Key::MouseLeft ) )
          {
            const float movePixels = gKeyboardInput.mMouseDeltaPos.x;
            const float movePercent = movePixels / cameraViewportSize.x;
            const float moveSeconds = movePercent * ( sMilisecondsToDisplay / 1000 );
            sPauseSec = ProfileTimepointAddSeconds( sPauseSec, -moveSeconds );
          }
          sMilisecondsToDisplay -= gKeyboardInput.mMouseDeltaScroll * 0.4f;
        }
      }
    }
  }

  static void ImGuiProfileWidgetTimeScale( const v2 timelinePos,
                                    const v2 timelineSize )
  {
    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = imguiWindow->mDrawData;

    drawData->AddBox( timelinePos,
                      timelinePos + timelineSize,
                      v4( 1, 0, 0, 1 ), Render::TextureHandle(), nullptr );

    const float pxPerMs = timelineSize.x / sMilisecondsToDisplay;

    for( int iMs = 0; iMs < ( int )sMilisecondsToDisplay; ++iMs )
    {
      const float msOffset = pxPerMs * iMs;

      const v2    tickBot( timelinePos.x + msOffset,
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
  }

  void ImGuiProfileWidget()
  {
    ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;

    const int   fps = GetFPS();
    const char* fpsText = FrameMemoryPrintf( "FPS: %i", fps );
    const float fpsT = Saturate( ( float )fps / 400.0f ); // 1 = fast, 0 = slow
    const v2    fpsBoxMin( imguiWindow->mCurrCursorViewport.x + 80.0f,
                           imguiWindow->mCurrCursorViewport.y + 7.0f );
    const v2    fpsBoxMax( fpsBoxMin.x + ( imguiWindow->mContentRect.mMaxi.x - fpsBoxMin.x ) * fpsT,
                           fpsBoxMin.y + 2.0f );
    const v4    fpsColor( Lerp( 1.0f, 0.0f, fpsT ),
                          Lerp( 0.0f, 1.0f, fpsT ),
                          0, 1 );
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
      }
    }
    else if( ImGuiButton( "Resume Profiling" ) )
      ProfileSetIsRuning( true );

    static bool profileDrawGrid = true;
    ImGuiCheckbox( "Profile draw grid", &profileDrawGrid );

    const v2    timelinePos = imguiWindow->mCurrCursorViewport;
    const v2    timelineSize = v2( imguiWindow->mContentRect.mMaxi.x - imguiWindow->mCurrCursorViewport.x,
                                   ImGuiGlobals::Instance.mUIStyle.fontSize * 3.0f );
    imguiWindow->mCurrCursorViewport.y += timelineSize.y;

    const v2    cameraViewportPos = imguiWindow->mCurrCursorViewport;
    const v2    cameraViewportSize = imguiWindow->mContentRect.mMaxi - cameraViewportPos;
    imguiWindow->mCurrCursorViewport.y += timelineSize.y;

    if( profileDrawGrid )
    {
    ImGuiProfileWidgetTimeScale( timelinePos, timelineSize );
    ImGuiProfileWidgetCamera( cameraViewportPos, cameraViewportSize );
    }
  }
}

