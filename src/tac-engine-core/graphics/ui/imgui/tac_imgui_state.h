// TODO: this file should not be accessible out of imgui files.
// tac_profile_imgui.cpp uses ImGuiWindow::IsHovered(), but should instead define
// a new fn ImGuiIsHovered() in tac_imgui.h that returns true if the last item was hovered.

#pragma once

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
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
  ImGuiRscIdx ImGuiRegisterWindowResource( StringView name,
                                          const void* initialDataBytes,
                                          int initialDataByteCount );

  struct ImGuiWindowResource
  {
    ImGuiID                       mImGuiID {};
    ImGuiRscIdx                   mIndex   { -1 };
    Vector< char >                mData    {};
  };

  struct ImGuiWindow
  {
    ImGuiWindow();
    ~ImGuiWindow();
    void                          BeginFrame();
    void                          EndFrame();

    //                            tell the window that we are adding a new element of this size
    //                            at the current cursor position.
    //
    //                            this lets the window internally update the cursor and other
    //                            positioning-related housekeeping information for scrollbars,
    //                            sameline, groups, etc.
    void                          ItemSize( v2 );

    //                            Clipping/Culling functions
    bool                          Overlaps( const ImGuiRect& ) const;
    ImGuiRect                     Clip( const ImGuiRect& ) const;

    void                          UpdateMaxCursorDrawPos( v2 );
    void*                         GetWindowResource( ImGuiRscIdx, ImGuiID );
    bool                          IsHovered( const ImGuiRect& );
    v2                            GetMousePosViewport();
    v2                            GetWindowPosScreenspace();
    void                          Scrollbar();
    void                          PushXOffset();
    void                          BeginMoveControls();
    void                          UpdateMoveControls();

    ImGuiID                       GetID( StringView );

    float                         GetRemainingWidth() const;
    WindowHandle                  GetWindowHandle() const;
    //const DesktopWindowState*     GetDesktopWindowState() const;

    void                          ResizeControls();
    void                          DrawWindowBackground();


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
    Timestamp                     mRequestTime                 {};
  };

  struct ImGuiRenderBuffer
  {
    Render::BufferHandle mBuffer    {};
    int                  mByteCount {};
  };

  struct ImGuiRenderBuffers
  {
    ImGuiRenderBuffer mVB;
    ImGuiRenderBuffer mIB;
  };

  struct ImGuiSimWindowDraws
  {
    void CopyBuffers( Render::IContext*, ImGuiRenderBuffers*, Errors& );

    WindowHandle                       mHandle;
    Vector< SmartPtr< UI2DDrawData > > mDrawData;
    int                                mVertexCount{};
    int                                mIndexCount{};

    // cant really give it a debug name because this is assocaited wiht hte viewport, not the wnd
    //
    //String                             mDebugName;

  private:
    void CopyIdxBuffer( Render::IContext*, ImGuiRenderBuffers*, Errors& );
    void CopyVtxBuffer( Render::IContext*, ImGuiRenderBuffers*, Errors& );
  };

  // generated once per game logic update frame,
  // passed to the imgui platform frame
  struct ImGuiSimFrame
  {
    struct WindowSizeData
    {
      WindowHandle    mWindowHandle;
      Optional< v2i > mRequestedPosition;
      Optional< v2i > mRequestedSize;
    };

    Vector< ImGuiSimWindowDraws > mWindowDraws;
    Vector< WindowSizeData >      mWindowSizeDatas;
    //Vector< WindowHandle >        mOwnedWindows;
    ImGuiMouseCursor              mCursor{ ImGuiMouseCursor::kNone };
  };

  struct ImGuiPersistantViewport
  {
    WindowHandle                 mWindowHandle  {};
    int                          mFrameIndex    {};
    Vector< ImGuiRenderBuffers > mRenderBuffers {};
  };



  struct ImGuiPersistantPlatformData
  {
    static ImGuiPersistantPlatformData Instance;

    void                       Init( Errors& );
    void                       UpdateAndRender( ImGuiSimFrame*,
                                                SysWindowApi,
                                                Errors& );
    void                       UpdateAndRenderWindow( SysWindowApi,
                                                      ImGuiSimWindowDraws*,
                                                      ImGuiPersistantViewport*,
                                                      Errors& );
    ImGuiPersistantViewport*   GetPersistantWindowData( WindowHandle );
    void                       UpdatePerFrame( Render::IContext*, v2i, Errors& );
    void                       UpdatePerObject( Render::IContext*,
                                               const UI2DDrawCall&,
                                               Errors& );

    // todo: hard code this to always expect rgba16f with d24s8
    struct Element
    {
      Render::PipelineHandle mPipeline;
      Render::TexFmt         mTexFmt;
      Render::IShaderVar*    mShaderImage{};
      Render::IShaderVar*    mShaderSampler{};
      Render::IShaderVar*    mShaderPerObject{};
      Render::IShaderVar*    mShaderPerFrame{};
    };

    Element&                   GetElement( Render::TexFmt, Errors& );
    Render::BlendState         GetBlendState() const;
    Render::DepthState         GetDepthState() const;
    Render::RasterizerState    GetRasterizerState() const;
    void                       Init1x1White( Errors& );
    void                       InitProgram( Errors& );
    void                       InitPerFrame( Errors& );
    void                       InitPerObject( Errors& );
    void                       InitSampler();
    Render::VertexDeclarations GetVertexDeclarations() const;

    Render::SamplerHandle             mSampler;
    Render::TextureHandle             m1x1White;
    Vector< ImGuiPersistantViewport > mViewportDatas;
    Vector< Element >                 mElements;
    Render::ProgramHandle             mProgram;
    Render::BufferHandle              mPerObject;
    Render::BufferHandle              mPerFrame;
  };

  struct ImGuiDesktopWindowImpl : public ImGuiDesktopWindow
  {
    ImGuiDesktopWindowImpl() = default;
    ImGuiSimWindowDraws GetSimWindowDraws();
  };

  void    SetActiveID( ImGuiID, ImGuiWindow* );
  void    SetHoveredID( ImGuiID );
  void    ClearActiveID();
  ImGuiID GetActiveID();

  struct ImGuiGlobals
  {
    static ImGuiGlobals               Instance;

    ImGuiWindow*                      FindWindow( const StringView& );
    ImGuiDesktopWindowImpl*           FindDesktopWindow( WindowHandle );

    ImGuiMouseCursor                  mMouseCursor          { ImGuiMouseCursor::kNone };
    Timestamp                         mElapsedSeconds       {};
    Vector< ImGuiWindow* >            mAllWindows           {};
    Vector< ImGuiWindow* >            mWindowStack          {};
    Vector< ImGuiDesktopWindowImpl* > mDesktopWindows       {};
    ImGuiWindow*                      mCurrentWindow        {};
    Vector< float >                   mFontSizeSK           {}; // wtf does sk stand for?
    UIStyle                           mUIStyle              {};
    WindowHandle                      mMouseHoveredWindow   {};
    bool                              mScrollBarEnabled     { true };
    int                               mMaxGpuFrameCount     {};

    // Ok so here's a problem:
    //
    //   Almost all imgui functions operate on the simulation thread and thus should have access to
    //   mSimWindowApi/mSimKeyboardApi.
    //
    //   However ImGuiPlatformRender runs on the system thread and should not have access to these.
    // 
    // Possible solution... split off ImGuiGlobals access from ImGuiPlatformRender render?
    SimWindowApi                      mSimWindowApi         {};
    SimKeyboardApi                    mSimKeyboardApi       {};
    SettingsNode                      mSettingsNode         {};

    ImGuiID                           mHoveredID            {};
    ImGuiID                           mActiveID             {};
    ImGuiWindow*                      mActiveIDWindow       {};
    ImGuiWindow*                      mMovingWindow         {};
    v2                                mActiveIDClickPos_VS  {};
    v2                                mActiveIDWindowSize   {};
    v2                                mActiveIDWindowPos_SS {};
    int                               mResizeMask           {};
    bool                              mSettingsDirty        {};
  };

  struct ImGuiNextWindow
  {
    v2             mPosition          {};
    bool           mPositionValid     {};
    ImGuiCondition mPositionCondition {};
    v2             mSize              {};
    bool           mSizeValid         {};
    ImGuiCondition mSizeCondition     {};
    WindowHandle   mWindowHandle      {};
    bool           mStretch           {};
    bool           mMoveResize        {};
    bool           mEnableBG          { true }; // Set false to disable background render
  };

  extern ImGuiNextWindow gNextWindow;

} // namespace Tac
