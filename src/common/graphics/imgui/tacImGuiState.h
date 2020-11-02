#pragma once

#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector4.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacString.h"
#include "src/common/tacDesktopWindow.h"

#include <map>

namespace Tac
{
  //struct UI2DDrawData;
  //struct TextInputData;

  struct UIStyle
  {
    float windowPadding = 8;
    v2 itemSpacing = { 8, 4 };

    // Should this be a float?
    int fontSize = 16;

    float buttonPadding = 3.0f;
    v4 textColor = { 1, 1, 0, 1 };
  };



  typedef int ImGuiId;
  const int ImGuiIdNull = -1;

  enum class DragMode
  {
    Drag,
    TextInput
  };

  struct DragData
  {
    DragMode mMode = ( DragMode )0;
    float mDragDistPx = 0;

    // This is the value of the variable we are gizmoing for at the start of the mouse drag.
    // That way, we can accumulate all the mouse drag pixels and apply them to ints in
    // addition to floats
    Vector< char > mValueCopy;
  };

  struct ImGuiIDAllocator
  {
    ImGuiId mActiveID = ImGuiIdNull;
    ImGuiId mIDCounter = 0;
  };

  struct GroupData
  {
    // Position of the cursor when starting the group
    v2 mSavedCursorDrawPos;
    float mSavedLineHeight;
  };


  typedef int ImGuiindex;

  ImGuiindex ImGuiRegisterWindowResource( StringView name,
                                               void* initialDataBytes,
                                               int initialDataByteCount );

  struct ImGuiWindowResource
  {
    ImGuiId                       mImGuiId;
    ImGuiindex                    mIndex;
    Vector< char >                mData;
  };

  struct ImGuiWindow
  {
    ImGuiWindow();
    ~ImGuiWindow();
    void                          BeginFrame();
    void                          ItemSize( v2 size );
    void                          ComputeClipInfo( bool* clipped, ImGuiRect* clipRect );
    void                          UpdateMaxCursorDrawPos( v2 pos );
    ImGuiId                       GetID();
    void                          SetActiveID( ImGuiId );
    ImGuiId                       GetActiveID();
    void*                         GetWindowResource( ImGuiindex id );
    bool                          IsHovered( const ImGuiRect& );
    v2                            GetRelativeMousePosition();


    String                        mName;
    ImGuiWindow*                  mParent = nullptr;

    // The most bottom right the cursor has ever been,
    // updated during ItemSize()
    v2                            mMaxiCursorDrawPos;
    v2                            mCurrCursorDrawPos;
    v2                            mPrevCursorDrawPos;

    v2                            mPosViewportSpace = {};
    v2                            mSize = {};
    ImGuiRect                     mContentRect;
    float                         mCurrLineHeight = 0;
    float                         mPrevLineHeight = 0;

    float                         mScroll = 0;
    v2                            mScrollMousePosScreenspaceInitial;
    bool                          mScrolling = false;

    Vector< GroupData >           mGroupSK;

    // The mXOffsets.back() represents the horizontal tabbing distance
    // from the window mPos and the stuff that's about to be drawn
    Vector< float >               mXOffsets;

    // Shared between sub-windows
    ImGuiIDAllocator*             mIDAllocator = nullptr;
    struct TextInputData*         mTextInputData = nullptr;
    std::map< ImGuiId, bool >     mCollapsingHeaderStates;
    std::map< ImGuiId, DragData > mDragDatas;
    bool                          mIsAppendingToMenu = false;
    Vector< ImGuiWindowResource > mResources;
    struct UI2DDrawData*          mDrawData = nullptr;
    DesktopWindowHandle           mDesktopWindowHandle;
    bool                          mDesktopWindowHandleOwned = false;
    v2                            mDesktopWindowOffset = {};
    bool                          mDesktopWindowOffsetExists = false;
  };

  struct ImGuiGlobals
  {
    static ImGuiGlobals           Instance;
    ImGuiWindow*                  FindWindow( StringView name );
    // TODO: different space
    //v2 mMousePositionDesktopWindowspace = {};
    //bool mIsWindowDirectlyUnderCursor = false;
    double                        mElapsedSeconds = 0;
    Vector< ImGuiWindow* >        mAllWindows;
    Vector< ImGuiWindow* >        mWindowStack;
    ImGuiWindow*                  mCurrentWindow = nullptr;
    //UI2DDrawData* mUI2DDrawData = nullptr;
    String                        mDesktopWindowName;
    Vector< int >                 mFontSizeSK;
    UIStyle                       mUIStyle;
    //int                           mDesktopWindowWidth = 0;
    //int                           mDesktopWindowHeight = 0;
    DesktopWindowHandle           mMouseHoveredWindow;
  };

  struct ImGuiNextWindow
  {
    void                          Clear();
    v2                            mSize = {};
  };
  extern ImGuiNextWindow gNextWindow;

}
