// TODO: this file should not be accessible out of imgui files.
// tac_profile_imgui.cpp uses ImGuiWindow::IsHovered(), but should instead define
// a new fn ImGuiIsHovered() in tac_imgui.h that returns true if the last item was hovered.

#pragma once

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/memory/tac_smart_ptr.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
//#include "tac-rhi/render/tac_render_handles.h" // DynamicBufferHandle2
#include "tac-rhi/render3/tac_render_api.h"

#undef FindWindow

namespace Tac
{
  typedef int ImGuiId;
  const int ImGuiIdNull = -1;

  struct ImGuiIDAllocator
  {
    ImGuiId mActiveID = ImGuiIdNull;
    ImGuiId mIDCounter = 0;
  };

  struct GroupData
  {
    //    Position of the cursor when starting the group
    v2    mSavedCursorDrawPos;
    float mSavedLineHeight = 0;
  };


  typedef int ImGuiIndex;

  ImGuiIndex ImGuiRegisterWindowResource( StringView name,
                                          const void* initialDataBytes,
                                          int initialDataByteCount );

  struct ImGuiWindowResource
  {
    ImGuiId                       mImGuiId = ImGuiIdNull;
    ImGuiIndex                    mIndex = -1;
    Vector< char >                mData;
  };

  struct ImGuiWindow
  {
    ImGuiWindow();
    ~ImGuiWindow();
    void                          BeginFrame();

    //                            tell the window that we are adding a new element of this size
    //                            at the current cursor position.
    //
    //                            this lets the window internally update the cursor and other
    //                            positioning-related housekeeping information for scrollbars,
    //                            sameline, groups, etc.
    void                          ItemSize( v2 size );

    //                            Clipping/Culling functions
    bool                          Overlaps( const ImGuiRect& ) const;
    ImGuiRect                     Clip( const ImGuiRect& ) const;

    void                          UpdateMaxCursorDrawPos( v2 pos );
    ImGuiId                       GetID();
    void                          SetActiveID( ImGuiId );
    ImGuiId                       GetActiveID();
    void*                         GetWindowResource( ImGuiIndex );
    bool                          IsHovered( const ImGuiRect& );
    v2                            GetMousePosViewport();
    void                          Scrollbar();
    void                          PushXOffset();

    float                         GetRemainingWidth() const;
    WindowHandle                  GetWindowHandle() const;
    //const DesktopWindowState*     GetDesktopWindowState() const;


    String                        mName;
    ImGuiWindow*                  mParent { nullptr };

    //                            The most bottom right the cursor has ever been,
    //                            updated during ItemSize()
    v2                            mViewportSpaceMaxiCursor{};
    v2                            mViewportSpaceCurrCursor{};
    v2                            mViewportSpacePrevCursor{};

    //                            Position of this ImGuiWindow relative to its viewport ( native desktop window )
    //                            A value of (0,0) is at the top left ( probably mParent == nullptr )
    v2                            mViewportSpacePos  {};
    v2                            mSize  {};

    //                            The viewport-space region in which visible ui is displayed on the screen
    //                            "Visible" here meaning not offscreen due to scrolling
    ImGuiRect                     mViewportSpaceVisibleRegion{};

    float                         mCurrLineHeight { 0 };
    float                         mPrevLineHeight { 0 };

    //                            The viewport-space pixel offset that the cursor should start at before
    //                            rendering any ui elements.
    float                         mScroll { 0 };
    v2                            mScrollMousePosScreenspaceInitial{};
    bool                          mScrolling { false };

    Vector< GroupData >           mGroupSK;

    //                            The mXOffsets.back() represents the horizontal tabbing distance
    //                            from the window mPos and the stuff that's about to be drawn
    Vector< float >               mXOffsets;

    //                            Shared between sub-windows
    ImGuiIDAllocator*             mIDAllocator { nullptr };
    struct TextInputData*         mTextInputData { nullptr };
    Map< ImGuiId, bool >          mCollapsingHeaderStates;
    bool                          mIsAppendingToMenu { false };
    Vector< ImGuiWindowResource > mResources;
    struct UI2DDrawData*          mDrawData { nullptr };

    ImGuiDesktopWindow*           mDesktopWindow;
    bool                          mWindowHandleOwned { false };

    //                            [ ] Q: What does this parameter do
    bool                          mStretchWindow { false };
    
    //                            [ ] Q: What does this parameter do
    bool                          mMoveResizeWindow { false };
    bool                          mEnableBG { true };

    //                            [ ] Q: What does this parameter do
    Timestamp                     mRequestTime{};
  };

  struct ImGuiRenderBuffers
  {
    Render::BufferHandle mVB;
    Render::BufferHandle mIB;
    int mVBCount { 0 };
    int mIBCount { 0 };
  };

  struct ImGuiSimWindowDraws
  {
    void CopyVertexes( ImGuiRenderBuffers*, Errors& );
    void CopyIndexes( ImGuiRenderBuffers*, Errors& );

    WindowHandle                       mHandle;
    Vector< SmartPtr< UI2DDrawData > > mDrawData;
    int                                mVertexCount{};
    int                                mIndexCount{};
  };

  // generated once per game logic update frame,
  // passed to the imgui platform frame
  struct ImGuiSimFrameDraws
  {
    Vector< ImGuiSimWindowDraws > mWindowDraws;
  };

  struct ImGuiPersistantViewport
  {
    WindowHandle                 mWindowHandle;
    int                          mFrameIndex{};
    Vector< ImGuiRenderBuffers > mRenderBuffers;
  };

  struct ImGuiPersistantPlatformData
  {
    static ImGuiPersistantPlatformData Instance;

    void UpdateAndRender( ImGuiSysDrawParams*, Errors& );

    ImGuiPersistantViewport* GetPersistantWindowData( WindowHandle h)
    {
      for( ImGuiPersistantViewport& viewportData : mViewportDatas )
        if( viewportData.mWindowHandle == h )
          return &viewportData;
      mViewportDatas.push_back( ImGuiPersistantViewport{ .mWindowHandle = h } );
      return &mViewportDatas.back();
    }

    Vector< ImGuiPersistantViewport > mViewportDatas;
  };

  struct ImGuiDesktopWindowImpl : public ImGuiDesktopWindow
  {
    ImGuiDesktopWindowImpl() = default;
    ImGuiSimWindowDraws GetSimWindowDraws();
  };

  struct ImGuiGlobals
  {
    static ImGuiGlobals               Instance;

    ImGuiMouseCursor                  mMouseCursor = ImGuiMouseCursor::kNone;
    ImGuiWindow*                      FindWindow( const StringView& name );
    ImGuiDesktopWindowImpl*           FindDesktopWindow( WindowHandle );

    Timestamp                         mElapsedSeconds;
    Vector< ImGuiWindow* >            mAllWindows;
    Vector< ImGuiWindow* >            mWindowStack;
    Vector< ImGuiDesktopWindowImpl* > mDesktopWindows;
    ImGuiWindow*                      mCurrentWindow { nullptr };
    Vector< float >                   mFontSizeSK; // wtf does sk stand for?
    UIStyle                           mUIStyle;
    WindowHandle                      mMouseHoveredWindow;
    bool                              mScrollBarEnabled { true };
    int                               mMaxGpuFrameCount{};

    // Ok so here's a problem:
    //
    //   Almost all imgui functions operate on the simulation thread and thus should have access to
    //   mSimWindowApi/mSimKeyboardApi.
    //
    //   However ImGuiPlatformRender runs on the system thread and should not have access to these.
    // 
    // Possible solution... split off ImGuiGlobals access from ImGuiPlatformRender render?
    SimWindowApi*                     mSimWindowApi{};
    SimKeyboardApi*                   mSimKeyboardApi{};
  };

  struct ImGuiNextWindow
  {
    v2           mPosition  {};
    v2           mSize  {};
		WindowHandle mWindowHandle;
    bool         mStretch { false };
    bool         mMoveResize { false };
    bool         mEnableBG { true }; // Set false to disable background render
  };

  extern ImGuiNextWindow gNextWindow;

} // namespace Tac
