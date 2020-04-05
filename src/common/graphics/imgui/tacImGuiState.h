#pragma once

#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector4.h"

namespace Tac
{


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


struct ImGuiWindowResource
{
  virtual ~ImGuiWindowResource() = default;
};

struct ImGuiWindow
{
  ImGuiWindow();
  ~ImGuiWindow();
  void BeginFrame();
  void ItemSize( v2 size );
  void ComputeClipInfo( bool* clipped, ImGuiRect* clipRect );
  void UpdateMaxCursorDrawPos( v2 pos );
  ImGuiId GetID();
  void SeTiveID( ImGuiId );
  ImGuiId GeTiveID();

  String mName;
  ImGuiWindow* mParent = nullptr;

  // The most bottom right the cursor has ever been,
  // updated during ItemSize()
  v2 mMaxiCursorDrawPos;
  v2 mCurrCursorDrawPos;
  v2 mPrevCursorDrawPos;

  // ( rename to mPosDesktopWindowspace )
  // Position of this ImGuiWindow relative to the desktop window (?)
  v2 mPos = {};
  v2 mSize = {};
  ImGuiRect mContentRect;
  float mCurrLineHeight = 0;
  float mPrevLineHeight = 0;

  float mScroll = 0;
  v2 mScrollMousePosScreenspaceInitial;
  bool mScrolling = false;

  Vector< GroupData > mGroupSK;

  // The mXOffsets.back() represents the horizontal tabbing distance
  // from the window mPos and the stuff that's about to be drawn
  Vector< float > mXOffsets;

  // Shared between sub-windows
  ImGuiIDAllocator* mIDAllocator = nullptr;

  TextInputData* inputData;
  std::map< ImGuiId, bool > mCollapsingHeaderStates;

  std::map< ImGuiId, DragData > mDragDatas;

  bool mIsAppendingToMenu = false;


  template< typename T > T* GetOrCreateResource()
  {
    ImGuiId id = GetID();
    ImGuiWindowResource* resource = mResources[id];
    T* t = dynamic_cast< T* >( resource );
    if( !t )
    {
      delete resource;
      t = new T;
      mResources[id] = t;
    }
    return t;
  }
  std::map< ImGuiId, ImGuiWindowResource* > mResources;
};

struct ImGuiGlobals
{
  static ImGuiGlobals Instance;
  ImGuiWindow* FindWindow( const String& name );
  bool IsHovered( const ImGuiRect& rect );

  v2 mNextWindowPos = {};
  // TODO: different space
  v2 mMousePositionDesktopWindowspace = {};
  bool mIsWindowDirectlyUnderCursor = false;
  double mElapsedSeconds = 0;
  Vector< ImGuiWindow* > mAllWindows;
  Vector< ImGuiWindow* > mWindowSK;
  ImGuiWindow* mCurrentWindow = nullptr;
  UI2DDrawData* mUI2DDrawData = nullptr;
  KeyboardInput* mKeyboardInput = nullptr;
  Vector< int > mFontSizeSK;
  UIStyle mUIStyle;
};
}
