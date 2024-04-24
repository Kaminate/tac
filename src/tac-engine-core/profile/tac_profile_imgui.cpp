#include "tac-engine-core/profile/tac_profile.h"

#include "tac-engine-core/profile/tac_profile_backend.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-std-lib/math/tac_math.h"

namespace Tac
{
  static float             sMilisecondsToDisplay { 20.0f };
  static Timepoint         sPauseSec;
  static ProfileFrame      sProfiledFunctions;
  static SimKeyboardApi*   sKeyboardApi;

  // Totally unused, but keeping it because its the only place where ImGuiRegisterWindowResource is used
  // It would be gotten like this:
  //
  //   ImGuiWindow* imguiWindow = ImGuiGlobals::Instance.mCurrentWindow;
  //   auto profileWidgetData = ( ImguiProfileWidgetData* )imguiWindow->GetWindowResource( sWidgetID );
  //

  struct ImguiProfileWidgetData
  {
  };

  static ImguiProfileWidgetData sDefaultWidgetData;

  static const ImGuiIndex sWidgetID{ ImGuiRegisterWindowResource( TAC_STRINGIFY( ImguiProfileWidgetData ),
                                                                   &sDefaultWidgetData,
                                                                   sizeof( ImguiProfileWidgetData ) ) };

  static int CalculateProfileHeight( ProfileFunction* profileFunction )
  {
    if( !profileFunction )
      return 0;

    int childDepthMax { 0 };
    for( ProfileFunction* child { profileFunction->mChildren }; child; child = child->mNext )
    {
      const int childDepth { CalculateProfileHeight( child ) };
      childDepthMax = childDepth > childDepthMax ? childDepth : childDepthMax;
    }

    return 1 + childDepthMax;
  }

  static int GetFPS()
  {
    Timepoint now { Timepoint::Now() };
    static Timepoint prev { now };

    const float sec { now - prev };
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

  struct ProfileThreadManager
  {
    int GetProfileThreadCount()
    {
      return ( int )sThreadNumberMap.size();
    }

    int GetProfileThreadNumber( const std::thread::id threadId )
    {
      const int n { sThreadNumberMap.size() };
      for( int i{}; i < n; ++i )
        if( sThreadNumberMap[ i ] == threadId )
          return i;

      sThreadNumberMap.push_back( threadId );
      return n;
    }

    Vector< std::thread::id > sThreadNumberMap;
  };

  static ProfileThreadManager sProfileThreadManager;

  static v4 GetProfileFunctionColor( const ProfileFunction* profileFunction )
  {
    const HashValue hash { Hash( profileFunction->mName ) };
    const float t { Sin( ( float )hash ) * 0.5f + 0.5f };
    //              a      b                             c          d
    const float r { 0.5f + 0.5f * Cos( 6.28318f * ( 1.0f * t + 0.0f ) ) };
    const float g { 0.5f + 0.5f * Cos( 6.28318f * ( 1.0f * t + 0.33f ) ) };
    const float b { 0.5f + 0.5f * Cos( 6.28318f * ( 1.0f * t + 0.66f ) ) };
    return { r, g, b, 1 };
  }

  struct IndexedThreadProfileData
  {
    const List< ProfileFunction* >* mProfiledFunctionList;
    int                             mTreeHeight;
  };

  struct ProfileFunctionDepth
  {
    ProfileFunction* mProfileFunction;
    int              mDepth;
  };

  static void ImGuiProfileWidgetCamera( const v2 cameraViewportPos,
                                        const v2 cameraViewportSize )
  {
    const float fontSize { ImGuiGetFontSize() };
    ImGuiWindow* imguiWindow { ImGuiGlobals::Instance.mCurrentWindow };
    UI2DDrawData* drawData { imguiWindow->mDrawData };

    const UI2DDrawData::Box vpBox 
    {
      .mMini { cameraViewportPos },
      .mMaxi { cameraViewportPos + cameraViewportSize },
      .mColor { v4( 0.2f, 0, 0, 1 ) },
    };
    drawData->AddBox( vpBox );

    IndexedThreadProfileData indexedThreadProfileDatas[ 10 ]  {};

    for( PerThreadProfileFrame& threadFrame : sProfiledFunctions.mThreadFrames )
    {
      List< ProfileFunction* >& profiledFunctionList { threadFrame.mFunctions };
      const int iThread { sProfileThreadManager.GetProfileThreadNumber( threadFrame.mThreadId ) };

      int treeHeight { 0 };
      for( ProfileFunction* profileFunction : profiledFunctionList )
      {
        const int depth { CalculateProfileHeight( profileFunction ) };
        treeHeight = depth > treeHeight ? depth : treeHeight;
      }

      IndexedThreadProfileData* indexedThreadProfileData { &indexedThreadProfileDatas[ iThread ] };
      indexedThreadProfileData->mProfiledFunctionList = &profiledFunctionList;
      indexedThreadProfileData->mTreeHeight = treeHeight;
    }

    float threadY { cameraViewportPos.y };
    const float boxHeight { fontSize };


    static Vector< ProfileFunctionDepth > visitors;

    for( int iThread { 0 }; iThread < sProfileThreadManager.GetProfileThreadCount(); ++iThread )
    {
      IndexedThreadProfileData* indexedThreadProfileData = &indexedThreadProfileDatas[ iThread ];

      TAC_ON_DESTRUCT(
        threadY += Max( indexedThreadProfileData->mTreeHeight, 1 ) * boxHeight
        + 5;// padding between thread call stacks
      );

      for( ProfileFunction* profileFunction : *indexedThreadProfileData->mProfiledFunctionList )
        visitors.push_back( { profileFunction, 0 } );


      const float timeScaleSeconds { sMilisecondsToDisplay / 1000.0f };
      const Timepoint gameBeginTimepoint { ProfileTimepointGetLastGameFrameBegin() };
      const Timepoint timepointLeft { ProfileGetIsRuning() ? gameBeginTimepoint : sPauseSec };

      while( !visitors.empty() )
      {
        ProfileFunctionDepth visitor { visitors.back() };
        ProfileFunction* profileFunction { visitor.mProfileFunction };
        visitors.pop_back();

        if( profileFunction->mNext )
          visitors.push_back( { profileFunction->mNext, visitor.mDepth } );

        if( profileFunction->mChildren )
          visitors.push_back( { profileFunction->mChildren, visitor.mDepth + 1 } );


        const float     boxDeltaSeconds { profileFunction->mEndTime - profileFunction->mBeginTime };
        const float     boxDeltaPercent { boxDeltaSeconds / timeScaleSeconds };
        //                           Max() ensures things are still visible
        const float     boxWidthPx { Max( boxDeltaPercent * cameraViewportSize.x, 10.0f ) };
        const float     boxXMinPercent{ ( profileFunction->mBeginTime - timepointLeft ) / timeScaleSeconds };
        const float     boxXMinPx { cameraViewportPos.x + cameraViewportSize.x * boxXMinPercent };
        const float     boxXMaxPx { boxXMinPx + boxWidthPx };
        const float     boxY { threadY + visitor.mDepth * boxHeight };
        const v2        boxMin  { boxXMinPx, boxY };
        const v2        boxMax  { boxXMaxPx, boxY + boxHeight };
        const ImGuiRect boxRect { ImGuiRect::FromMinMax( boxMin, boxMax ) };
        const v4        boxColor { GetProfileFunctionColor( profileFunction ) };

        const StringView textPrefix( "Tac::" );
        const char* textFunction { profileFunction->mName };
        const v4         textColor( 0, 0, 0, 1 );
        const char* text{ StringView( textFunction ).starts_with( textPrefix )
          ? textFunction + textPrefix.size()
          : textFunction };

        if( imguiWindow->IsHovered( boxRect ) )
        {
          const v2 textSize { CalculateTextSize( profileFunction->mName, fontSize ) };
          const float boxDeltaMsec { ( float )( boxDeltaSeconds * 1000 ) };


          ImGuiSetNextWindowPosition( sKeyboardApi->GetMousePosScreenspace() );
          ImGuiSetNextWindowSize( textSize + v2( 1, 1 ) * 50.0f );
          ImGuiBegin( "hovered" );
          ImGuiText( profileFunction->mName );
          ImGuiText( String() + "msec: " + ToString( boxDeltaMsec ) );
          ImGuiEnd();
        }

        const UI2DDrawData::Box drawBox 
        {
          .mMini { boxMin },
          .mMaxi { boxMax },
          .mColor { boxColor },
        };
        drawData->AddBox( drawBox );

        const UI2DDrawData::Text drawText 
        {
          .mPos { boxMin },
          .mFontSize { fontSize },
          .mUtf8 { text },
          .mColor { textColor },
        };
        drawData->AddText( drawText );
      }
    }

    // Pan camera
    {
      const ImGuiRect viewRect { ImGuiRect::FromPosSize( cameraViewportPos, cameraViewportSize ) };
      if( imguiWindow->IsHovered( viewRect ) )
      {
        //static Timestamp mouseMovement;
        //Mouse::TryConsumeMouseMovement( &mouseMovement, TAC_STACK_FRAME );

        //if( mouseMovement )
        {
          if( sKeyboardApi->IsPressed( Key::MouseLeft ) )
          {
            const float movePixels { sKeyboardApi->GetMousePosDelta().x };
            const float movePercent { movePixels / cameraViewportSize.x };
            const float moveSeconds { movePercent * ( sMilisecondsToDisplay / 1000 ) };
            sPauseSec -= moveSeconds;
          }
          sMilisecondsToDisplay -= sKeyboardApi->GetMouseWheelDelta() * 0.4f;
        }
      }
    }
  }

  static void ImGuiProfileWidgetTimeScale( const v2 timelinePos,
                                           const v2 timelineSize )
  {
    const float fontSize { ImGuiGetFontSize() };
    const v4& textColor { ImGuiGetColor( ImGuiCol::Text ) };

    ImGuiWindow* imguiWindow { ImGuiGlobals::Instance.mCurrentWindow };
    UI2DDrawData* drawData { imguiWindow->mDrawData };

    const UI2DDrawData::Box box 
    {
      .mMini { timelinePos },
      .mMaxi { timelinePos + timelineSize },
      .mColor { v4( 1, 0, 0, 1  )}
    };
    drawData->AddBox( box );

    const float pxPerMs { timelineSize.x / sMilisecondsToDisplay };

    for( int iMs { 0 }; iMs < ( int )sMilisecondsToDisplay; ++iMs )
    {
      const float msOffset { pxPerMs * iMs };

      const v2    tickBot { timelinePos + v2( msOffset, timelineSize.y ) };
      const v2    tickTop { tickBot - v2( 0, 10 ) };

      auto text { ToString( iMs ) };
      const v2    textSize { CalculateTextSize( text, fontSize ) };
      const v2    textPos{ tickTop.x - textSize.x / 2,
                           tickTop.y - textSize.y };


      const UI2DDrawData::Line line 
      {
        .mP0 { tickBot },
        .mP1 { tickTop },
        .mLineRadius { 2 },
        .mColor { v4( 1, 1, 1, 1 ) },
      };
      drawData->AddLine( line );

      const UI2DDrawData::Text drawText 
      {
        .mPos { textPos },
        .mFontSize { fontSize },
        .mUtf8 { text },
        .mColor { textColor },
      };
      drawData->AddText( drawText );
    }
  }
}

void Tac::ImGuiProfileWidget( SimKeyboardApi* keyboardApi )
{
  sKeyboardApi = keyboardApi;

  const float fontSize { ImGuiGetFontSize() };
  ImGuiWindow* imguiWindow { ImGuiGlobals::Instance.mCurrentWindow };
  UI2DDrawData* drawData { imguiWindow->mDrawData };

  const int   fps { GetFPS() };
  const float fpsT { Saturate( ( float )fps / 400.0f ); // 1 = fast, 0 = slo }w
  const v2    fpsBoxMin { imguiWindow->mViewportSpaceCurrCursor + v2( 80.0f, 7.0f ) };
  const v2    fpsBoxMax{ fpsBoxMin + v2( ( imguiWindow->mViewportSpaceVisibleRegion.mMaxi.x - fpsBoxMin.x ) * fpsT,
                                          2.0f ) };
  const v4    fastColor( 0, 1, 0, 1 );
  const v4    slowColor( 1, 0, 0, 1 );
  const v4    fpsColor { Lerp( slowColor, fastColor, fpsT ) };

  const UI2DDrawData::Box fpsBox
  {
    .mMini { fpsBoxMin },
    .mMaxi { fpsBoxMax },
    .mColor { fpsColor },
  };
  drawData->AddBox( fpsBox );


  ImGuiText( String() + "FPS: " + ToString( fps ) );

  if( ProfileGetIsRuning() )
  {
    const bool hasData { !sProfiledFunctions.empty() };
    if( hasData )
      sProfiledFunctions.Clear();

    sProfiledFunctions = ProfileCopyFrame();
    if( ImGuiButton( "Pause Profiling" ) )
    {
      ProfileSetIsRuning( false );
      sPauseSec = ProfileTimepointGetLastGameFrameBegin();
    }
  }
  else if( ImGuiButton( "Resume Profiling" ) )
    ProfileSetIsRuning( true );

  static bool profileDrawGrid { true };
  ImGuiCheckbox( "Profile draw grid", &profileDrawGrid );

  const v2    timelinePos { imguiWindow->mViewportSpaceCurrCursor };
  const v2    timelineSize{ v2( imguiWindow->mViewportSpaceVisibleRegion.mMaxi.x - imguiWindow->mViewportSpaceCurrCursor.x,
                                 fontSize * 3.0f ) };
  imguiWindow->mViewportSpaceCurrCursor.y += timelineSize.y;

  const v2    cameraViewportPos { imguiWindow->mViewportSpaceCurrCursor };
  const v2    cameraViewportSize { imguiWindow->mViewportSpaceVisibleRegion.mMaxi - cameraViewportPos };
  imguiWindow->mViewportSpaceCurrCursor.y += timelineSize.y;

  if( profileDrawGrid )
  {
    ImGuiProfileWidgetTimeScale( timelinePos, timelineSize );
    ImGuiProfileWidgetCamera( cameraViewportPos, cameraViewportSize );
  }
}

