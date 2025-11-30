// TODO: this file should not be accessible out of imgui files.
// tac_profile_imgui.cpp uses ImGuiWindow::IsHovered(), but should instead define
// a new fn ImGuiIsHovered() in tac_imgui.h that returns true if the last item was hovered.

#pragma once

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/memory/tac_smart_ptr.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-engine-core/graphics/ui/tac_text_edit.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"

#undef FindWindow

namespace Tac
{

  struct GroupData
  {
    //    Position of the cursor when starting the group
    v2    mSavedCursorDrawPos {};
    float mSavedLineHeight    {};
  };


  using ImGuiRscIdx = int;

  struct ImGuiWindowResource
  {
    struct Params
    {
       StringView  mName                 {};
       const void* mInitialDataBytes     {};
       int         mInitialDataByteCount {};
    };
    static auto Register( Params ) -> ImGuiRscIdx;

    ImGuiID                       mImGuiID {};
    ImGuiRscIdx                   mIndex   { -1 };
    Vector< char >                mData    {};
  };

  struct ImGuiWindow
  {
    void BeginFrame();
    void EndFrame();

    //  tell the window that we are adding a new element of this size
    //  at the current cursor position.
    //  this lets the window internally update the cursor and other
    //  positioning-related housekeeping information for scrollbars,
    //  sameline, groups, etc.
    void ItemSize( v2 );

    bool Overlaps( const ImGuiRect& ) const;
    auto Clip( const ImGuiRect& ) const -> ImGuiRect;
    void UpdateMaxCursorDrawPos( v2 );
    auto GetWindowResource( ImGuiRscIdx, ImGuiID ) -> void*;
    bool IsHovered( const ImGuiRect&, ImGuiID );
    auto GetMousePosViewport() -> v2;
    auto GetWindowPosScreenspace() const -> v2;
    void Scrollbar();
    void PushXOffset();
    void BeginMoveControls();
    void UpdateMoveControls();
    auto GetID( StringView ) -> ImGuiID;
    auto GetRemainingWidth() const -> float;
    auto GetRemainingHeight() const -> float;
    auto GetWindowHandle() const -> WindowHandle;
    void ResizeControls();
    void DrawWindowBackground();


    String                        mName                        {};
    ImGuiWindow*                  mParent                      {};

    //                            The most bottom right the cursor has ever been,
    //                            updated during ItemSize()
    v2                            mViewportSpaceMaxiCursor     {};
    v2                            mViewportSpaceCurrCursor     {};
    v2                            mViewportSpacePrevCursor     {};

    //                            Position of this ImGuiWindow relative to its viewport ( native desktop window )
    //                            A value of (0,0) is at the top left ( probably mParent == nullptr )
    v2                            mViewportSpacePos            {};
    v2                            mSize                        {};

    //                            The viewport-space region in which visible ui is displayed on the screen
    //                            "Visible" here meaning not offscreen due to scrolling
    ImGuiRect                     mViewportSpaceVisibleRegion  {};

    float                         mCurrLineHeight              {};
    float                         mPrevLineHeight              {};

    //                            The viewport-space pixel offset that the cursor should start at before
    //                            rendering any ui elements.
    float                         mScroll                           {};
    v2                            mScrollMousePosScreenspaceInitial {};

    Vector< GroupData >           mGroupSK;

    //                            The mXOffsets.back() represents the horizontal tabbing distance
    //                            from the window mPos and the stuff that's about to be drawn
    Vector< float >               mXOffsets;

    Vector< ImGuiID >             mIDStack;

    ImGuiID                       mMoveID;
    ImGuiID                       mWindowID;

    //                            Shared between sub-windows
    SmartPtr< TextInputData >     mTextInputData               {};
    Map< ImGuiID, bool >          mCollapsingHeaderStates      {};
    bool                          mIsAppendingToMenu           {};
    Vector< ImGuiWindowResource > mResources                   {};
    SmartPtr< UI2DDrawData >      mDrawData                    {};

    ImGuiDesktopWindow*           mDesktopWindow               {};
    bool                          mWindowHandleOwned           {};

    //                            [ ] Q: What does this parameter do
    bool                          mStretchWindow               {};
    
    //                            [ ] Q: What does this parameter do
    bool                          mMoveResizeWindow            {};
    bool                          mEnableBG                    { true };

    //                            [ ] Q: What does this parameter do
    GameTime                      mRequestTime                 {};

    v2                            mBorderData[4]               {}; // Used for window resize grips
  };

  struct ImGuiDesktopWindowImpl : public ImGuiDesktopWindow
  {
    Vector< UI2DDrawData* > mImGuiDrawDatas      {};
    bool                    mMonitorDpiRequested { true };
    UI2DRenderData          mRenderBuffers       {};
  };

  void SetActiveID( ImGuiID, ImGuiWindow* );
  void ClearActiveID();
  auto GetActiveID() -> ImGuiID;

  struct ReferenceResolution
  {
    int mWidthPx = 1920;
    int mHeightPx = 1080;
    int mDpi = 96;
  };

  struct ImGuiGlobals
  {
    auto FindWindow( const StringView& ) -> ImGuiWindow*;
    auto FindDesktopWindow( WindowHandle ) -> ImGuiDesktopWindowImpl*;

    static ImGuiGlobals               Instance;
    ImGuiMouseCursor                  mMouseCursor          { ImGuiMouseCursor::kNone };
    GameTime                          mElapsedSeconds       {};
    Vector< ImGuiWindow* >            mAllWindows           {};
    Vector< ImGuiWindow* >            mWindowStack          {};
    Vector< ImGuiDesktopWindowImpl* > mDesktopWindows       {};
    ImGuiWindow*                      mCurrentWindow        {};
    Vector< float >                   mFontSizeSK           {}; // wtf does sk stand for?
    UIStyle                           mUIStyle              {};
    WindowHandle                      mMouseHoveredWindow   {};
    bool                              mScrollBarEnabled     { true };
    int                               mMaxGpuFrameCount     {};
    SettingsNode                      mSettingsNode         {};
    ImGuiID                           mHoveredID            {};
    ImGuiID                           mHoveredIDPrev        {};
    GameTime                          mHoverStartTime       {};
    ImGuiID                           mActiveID             {};
    ImGuiWindow*                      mActiveIDWindow       {};
    ImGuiWindow*                      mMovingWindow         {};
    v2                                mActiveIDClickPos_VS  {};
    v2                                mActiveIDWindowSize   {};
    v2                                mActiveIDWindowPos_SS {};
    int                               mResizeMask           {};
    bool                              mSettingsDirty        {};
    ReferenceResolution               mReferenceResolution  {};
  };



} // namespace Tac
