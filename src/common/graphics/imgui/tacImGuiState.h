#pragma once

#include "common/graphics/imgui/tacImGui.h"
#include "common/math/tacVector2.h"
#include "common/math/tacVector4.h"

struct TacUIStyle
{
  float windowPadding = 8;
  v2 itemSpacing = { 8, 4 };

  // Should this be a float?
  int fontSize = 16;

  float buttonPadding = 3.0f;
  v4 textColor = { 1, 1, 0, 1 };
};



typedef int TacImGuiId;
const int TacImGuiIdNull = -1;

enum class TacDragMode
{
  Drag,
  TextInput
};

struct TacDragData
{
  TacDragMode mMode = ( TacDragMode )0;
  float mDragDistPx = 0;

  // This is the value of the variable we are gizmoing for at the start of the mouse drag.
  // That way, we can accumulate all the mouse drag pixels and apply them to ints in
  // addition to floats
  TacVector< char > mValueCopy;
};

struct TacImGuiIDAllocator
{
  TacImGuiId mActiveID = TacImGuiIdNull;
  TacImGuiId mIDCounter = 0;
};

struct TacGroupData
{
  // Position of the cursor when starting the group
  v2 mSavedCursorDrawPos;
  float mSavedLineHeight;
};


struct TacImGuiWindowResource
{
  virtual ~TacImGuiWindowResource() = default;
};

struct TacImGuiWindow
{
  TacImGuiWindow();
  ~TacImGuiWindow();
  void BeginFrame();
  void ItemSize( v2 size );
  void ComputeClipInfo( bool* clipped, TacImGuiRect* clipRect );
  void UpdateMaxCursorDrawPos( v2 pos );
  TacImGuiId GetID();
  void SetActiveID( TacImGuiId );
  TacImGuiId GetActiveID();

  TacString mName;
  TacImGuiWindow* mParent = nullptr;

  // The most bottom right the cursor has ever been,
  // updated during ItemSize()
  v2 mMaxiCursorDrawPos;
  v2 mCurrCursorDrawPos;
  v2 mPrevCursorDrawPos;

  // ( rename to mPosDesktopWindowspace )
  // Position of this TacImGuiWindow relative to the desktop window (?)
  v2 mPos = {};
  v2 mSize = {};
  TacImGuiRect mContentRect;
  float mCurrLineHeight = 0;
  float mPrevLineHeight = 0;

  float mScroll = 0;
  v2 mScrollMousePosScreenspaceInitial;
  bool mScrolling = false;

  TacVector< TacGroupData > mGroupStack;

  // The mXOffsets.back() represents the horizontal tabbing distance
  // from the window mPos and the stuff that's about to be drawn
  TacVector< float > mXOffsets;

  // Shared between sub-windows
  TacImGuiIDAllocator* mIDAllocator = nullptr;

  TacTextInputData* inputData;
  std::map< TacImGuiId, bool > mCollapsingHeaderStates;

  std::map< TacImGuiId, TacDragData > mDragDatas;

  bool mIsAppendingToMenu = false;


  template< typename T > T* GetOrCreateResource()
  {
    TacImGuiId id = GetID();
    TacImGuiWindowResource* resource = mResources[id];
    T* t = dynamic_cast< T* >( resource );
    if( !t )
    {
      delete resource;
      t = new T;
      mResources[id] = t;
    }
    return t;
  }
  std::map< TacImGuiId, TacImGuiWindowResource* > mResources;
};

struct TacImGuiGlobals
{
  static TacImGuiGlobals Instance;
  TacImGuiWindow* FindWindow( const TacString& name );
  bool IsHovered( const TacImGuiRect& rect );

  v2 mNextWindowPos = {};
  // TODO: different space
  v2 mMousePositionDesktopWindowspace = {};
  bool mIsWindowDirectlyUnderCursor = false;
  double mElapsedSeconds = 0;
  TacVector< TacImGuiWindow* > mAllWindows;
  TacVector< TacImGuiWindow* > mWindowStack;
  TacImGuiWindow* mCurrentWindow = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacKeyboardInput* mKeyboardInput = nullptr;
  TacVector< int > mFontSizeStack;
  TacUIStyle mUIStyle;
};
