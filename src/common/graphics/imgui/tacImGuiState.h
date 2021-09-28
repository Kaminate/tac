#pragma once

#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector4.h"
#include "src/common/containers/tacVector.h"
#include "src/common/string/tacString.h"
#include "src/common/tacDesktopWindow.h"

#include <map>

namespace Tac
{
  struct UIStyle
  {
    float windowPadding = 8;
    v2    itemSpacing = { 8, 4 };

    float fontSize = 16;

    float buttonPadding = 3.0f;
    v4    textColor = { 1, 1, 0, 1 };
  };



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
    float mSavedLineHeight;
  };


  typedef int ImGuiIndex;

  ImGuiIndex ImGuiRegisterWindowResource( StringView name,
                                          const void* initialDataBytes,
                                          int initialDataByteCount );

  struct ImGuiWindowResource
  {
    ImGuiId                       mImGuiId;
    ImGuiIndex                    mIndex;
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

    void                          ComputeClipInfo( bool* clipped, ImGuiRect* clipRect );
    void                          UpdateMaxCursorDrawPos( v2 pos );
    ImGuiId                       GetID();
    void                          SetActiveID( ImGuiId );
    ImGuiId                       GetActiveID();
    void*                         GetWindowResource( ImGuiIndex );
    bool                          IsHovered( const ImGuiRect& );
    v2                            GetMousePosViewport();
    void                          Scrollbar();
    void                          PushXOffset();


    String                        mName;
    ImGuiWindow*                  mParent = nullptr;

    //                            The most bottom right the cursor has ever been,
    //                            updated during ItemSize()
    v2                            mViewportSpaceMaxiCursor;
    v2                            mViewportSpaceCurrCursor;
    v2                            mViewportSpacePrevCursor;

    //                            Position of this ImGuiWindow relative to the native desktop window
    //                            A value of (0,0) is at the top left ( probably mParent == nullptr )
    v2                            mViewportSpacePos = {};
    v2                            mSize = {};

    //                            The viewport-space region in which visible ui is displayed on the screen
    //                            "Visible" here meaning not offscreen due to scrolling
    ImGuiRect                     mViewportSpaceVisibleRegion;

    float                         mCurrLineHeight = 0;
    float                         mPrevLineHeight = 0;

    //                            The viewport-space pixel offset that the cursor should start at before
    //                            rendering any ui elements.
    float                         mScroll = 0;
    v2                            mScrollMousePosScreenspaceInitial;
    bool                          mScrolling = false;

    Vector< GroupData >           mGroupSK;

    //                            The mXOffsets.back() represents the horizontal tabbing distance
    //                            from the window mPos and the stuff that's about to be drawn
    Vector< float >               mXOffsets;

    //                            Shared between sub-windows
    ImGuiIDAllocator*             mIDAllocator = nullptr;
    struct TextInputData*         mTextInputData = nullptr;
    std::map< ImGuiId, bool >     mCollapsingHeaderStates;
    bool                          mIsAppendingToMenu = false;
    Vector< ImGuiWindowResource > mResources;
    struct UI2DDrawData*          mDrawData = nullptr;
    DesktopWindowHandle           mDesktopWindowHandle;
    bool                          mDesktopWindowHandleOwned = false;
    //v2                            mDesktopWindowOffset = {};
    //bool                          mDesktopWindowOffsetExists = false;
    bool                          mStretchWindow = false;
    bool                          mMoveResizeWindow = false;
    double                        mRequestTime;
  };

  struct ImGuiGlobals
  {
    ImGuiWindow*                  FindWindow( StringView name );
    static ImGuiGlobals           Instance;
    double                        mElapsedSeconds = 0;
    Vector< ImGuiWindow* >        mAllWindows;
    Vector< ImGuiWindow* >        mWindowStack;
    ImGuiWindow*                  mCurrentWindow = nullptr;
    String                        mDesktopWindowName;
    Vector< float >               mFontSizeSK;
    UIStyle                       mUIStyle;
    DesktopWindowHandle           mMouseHoveredWindow;
  };

  struct ImGuiNextWindow
  {
    v2                            mPosition = {};
    v2                            mSize = {};
		DesktopWindowHandle           mDesktopWindowHandle;
    bool                          mStretch = false;
    bool                          mMoveResize = false;
  };
  extern ImGuiNextWindow gNextWindow;

}
