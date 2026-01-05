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
    v2    mSavedCursorDrawPos           {};
    float mSavedLineHeight              {};
    v2    mSavedViewportSpaceMaxiCursor {};
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
    bool IsHovered( const ImGuiRect&, ImGuiID ) const;
    auto GetMousePos_uiwindowspace() const -> v2; // mouse relative to ImGuiWindow corner
    auto GetMousePos_nwhspace() const -> v2; // mouse relative to win32 window corner
    auto GetWindowPos_desktopspace() const -> v2; // mouse relative to primary monitor
    void Scrollbar();
    void PushXOffset();
    void BeginMoveControls();
    void UpdateMoveControls();
    auto GetID( StringView ) const -> ImGuiID;
    auto GetRemainingWidth() const -> float;
    auto GetRemainingHeight() const -> float;
    auto GetWindowHandle() const -> WindowHandle;
    void ResizeControls();
    void MenuBar();
    void DrawWindowBackground();
    void ToggleFullscreen();


    String                        mName                        {};
    ImGuiWindow*                  mParent                      {};
    ImGuiWindowFlags              mFlags                       {};
    bool*                         mOpen                        {};

    //                            The most bottom right the cursor has ever been,
    //                            updated during ItemSize()
    v2                            mViewportSpaceMaxiCursor     {};
    v2                            mViewportSpaceCurrCursor     {};
    v2                            mViewportSpacePrevCursor     {};

    //                            Position of this ImGuiWindow relative to its viewport ( native desktop window )
    //                            A value of (0,0) is at the top left ( probably mParent == nullptr )
    v2                            mViewportSpacePos            {};
    v2                            mSize                        {};
    v2                            mPreviousViewportToggleSize  {}; // for toggling fullscreen and back
    v2                            mPreviousViewportTogglePos   {}; // for toggling fullscreen and back

    //                            The viewport-space region in which visible ui is displayed on the screen
    //                            "Visible" here meaning not offscreen due to scrolling
    //                            ...
    //                            This should be renamed content rect. Doesn't include title bar
    ImGuiRect                     mViewportSpaceVisibleRegion  {};

    ImGuiRect                     mViewportSpaceMouseGrabRegion {}; // grabbable by the mouse to move the window

    float                         mCurrLineHeight              {};
    float                         mPrevLineHeight              {};

    //                            The viewport-space pixel offset that the cursor should start at before
    //                            rendering any ui elements.
    float                         mScroll                           {};
    v2                            mScrollMousePosScreenspaceInitial {};

    Vector< GroupData >           mGroupStack;

    //                            The mXOffsets.back() represents the horizontal tabbing distance
    //                            from the window mPos and the stuff that's about to be drawn
    Vector< float >               mXOffsets;

    Vector< ImGuiID >             mIDStack;

    ImGuiID                       mMoveID;
    ImGuiID                       mWindowID;

    //                            Shared between sub-windows
    SmartPtr< TextInputData >     mTextInputData               {};
    Map< ImGuiID, bool >          mCollapsingHeaderStates      {};
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
    bool                          mAppendingToMenuBar          {};
    v2                            mMenuBarMini                 {}; // viewport space
    float                         mMenuBarCurrWidth            {}; // viewport space
    float                         mMenuBarPrevWidth            {}; // viewport space
    v2                            mMenuBarCachedCursor         {};

    String                        mPopupMenu                   {};
  };

  struct ImGuiDesktopWindowImpl : public ImGuiDesktopWindow
  {
    Vector< UI2DDrawData* > mImGuiDrawDatas      {};
    bool                    mMonitorDpiRequested { true };
    UI2DRenderData          mRenderBuffers       {};
  };


  struct ReferenceResolution
  {
    int mWidthPx = 1920;
    int mHeightPx = 1080;
    int mDpi = 96;
  };

  struct ImGuiGlobals
  {
    static auto FindWindow( StringView ) -> ImGuiWindow*;
    static auto FindDesktopWindow( WindowHandle ) -> ImGuiDesktopWindowImpl*;
    static void SetActiveID( ImGuiID, ImGuiWindow* );
    static void ClearActiveID();
    static auto GetActiveID() -> ImGuiID;

    static ImGuiMouseCursor                  mMouseCursor;
    static GameTime                          mElapsedSeconds;
    static Vector< ImGuiWindow* >            mAllWindows;
    static Vector< String >                  mPopupStack;
    static Vector< ImGuiWindow* >            mWindowStack;
    static Vector< ImGuiDesktopWindowImpl* > mDesktopWindows;
    static ImGuiWindow*                      mCurrentWindow;
    static Vector< float >                   mFontSizeStack;
    static UIStyle                           mUIStyle;
    static WindowHandle                      mMouseHoveredWindow;
    static bool                              mScrollBarEnabled;
    static int                               mMaxGpuFrameCount;
    static SettingsNode                      mSettingsNode;
    static ImGuiID                           mHoveredID;
    static ImGuiID                           mHoveredIDPrev;
    static GameTime                          mHoverStartTime;
    static ImGuiID                           mActiveID;
    static ImGuiWindow*                      mActiveIDWindow;
    static ImGuiWindow*                      mMovingWindow;
    static v2                                mActiveIDClickPos_UIWindowSpace;
    static v2                                mActiveIDWindowSize;
    static v2                                mActiveIDWindowPos_DesktopSpace;
    static int                               mResizeMask;
    static bool                              mSettingsDirty;
    static ReferenceResolution               mReferenceResolution;
  };



} // namespace Tac
