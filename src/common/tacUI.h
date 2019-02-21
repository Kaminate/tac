// Contains user interface / user experience stuff
// Like menus and text and buttons
//
// Coordinate space
//
//   +--> x
//   |
//   v
//   y
//
//   The top left window corner is ( 0, 0 ),
//   which is the most natural for drawing ui since you're
//   never really subtracting
//


#pragma once

#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/tacEvent.h"
#include "common/tacLocalization.h"
#include "common/containers/tacVector.h"
#include "common/math/tacVector2.h"
#include "common/math/tacVector4.h"
#include <list>
#include <set>

struct TacUIAnchor;
struct TacUIButtonCallback;
struct TacUITextInfo;
struct TacUITextData;
struct TacUITextTransition;
struct TacUIText;
struct TacUIRoot;
struct TacUILayout;
struct TacUILayoutable;
struct TacUILayoutData;
struct TacUI2DDrawData;
struct TacUIHierarchyNode;
struct TacDepthBuffer;
struct TacDesktopWindow;
struct TacKeyboardInput;
struct TacTexture;
struct TacFontStuff;

enum class TacUIAnchorHorizontal
{
  Center,
  Left,
  Right,
  Count
};
const TacString TacAnchorHorizontalStrings[] =
{
  "Center",
  "Left",
  "Right"
};

enum class TacUIAnchorVertical
{
  Center,
  Top,
  Bottom,
  Count
};
const TacString TacAnchorVerticalStrings[] =
{
  "Center",
  "Top",
  "Bottom"
};

struct TacUIAnchor
{
  void DebugImgui();

  // Changed the defaults from
  // Horizontal Center, Vertical Center to
  // Horizontal Left, Vertical Top...
  TacUIAnchorHorizontal mAnchorHorizontal = TacUIAnchorHorizontal::Left;
  TacUIAnchorVertical mAnchorVertical = TacUIAnchorVertical::Top;
};


struct TacUILayoutable // yee haw
{
  TacUILayoutable( const TacString& debugName );
  virtual ~TacUILayoutable();
  virtual void DebugImgui();
  virtual void Update( TacUILayoutData* uiLayoutData ) {};
  virtual void TransitionOut() {};
  virtual void TransitionIn() {};
  virtual void Render( TacErrors& errors );
  virtual bool IsHovered();
  virtual v2 GetWindowspacePosition();

  TacUILayoutable* mParent = nullptr;
  TacUIRoot* mUIRoot = nullptr;

  // Advance width
  // ^ what does this mean?
  // | why is this zero?
  float mUiWidth = 0;

  // Computed, should not be modified directly
  float mUiHeight = 0;

  // Final uispace position of the ui element, after having been anchored and offset.
  // Computed automatically
  v2 mPositionAnchored = {};

  TacString mDebugName;
  bool mDebug = false;

  v2 mLocalPosition = {};
};

// todo: replace with event
struct TacUIButtonCallback
{
  void* mUserData = nullptr;
  void( *mUserCallback )( void*, TacErrors& errors ) = nullptr;
};

struct TacUITextData
{
  void DebugImgui();

  v4 mColor = { 1, 1, 0, 1 };
  TacString mUtf8;
  int mFontSize = 20;

  // Q: why does text need to know its width and height?
  // Q: can this be deleted?
  // Q: should this be set by the user?
  float mUIWidth = 0;
  float mUIHeight = 0;
};

struct TacUITextTransition
{
  void Update();
  void DebugImgui();

  double mTransitionStartSeconds = 0;
  bool mTransitionFinished = false;
  float mTransitionTweenPercent = 0;
  TacUIText* mUIText = nullptr;
  bool mUseUITextDataStore = false;
  TacUITextData mUITextDataStore;
};

struct TacUIText : public TacUILayoutable
{
  TacUIText( const TacString& debugName );
  ~TacUIText();
  void UpdateTransitions();
  void Update( TacUILayoutData* uiLayoutData ) override;
  void DebugImgui()override;
  void TransitionOut()override;
  void TransitionIn()override;
  void Render( TacErrors& errors ) override;
  bool IsHovered() override;
  void GoNuts();
  const TacUITextData* GetUITextData();
  void SetText( TacUITextData uiTextData, bool updateStack = true );
  // mIsRecentChangeInStack or mTransitionOut changed
  void Think();
  bool IsMostRecentChangeInStack();

  bool mIsHovered = false;
  double mHoverStartSeconds = 0;

  // comment?
  float mExtraXPos = 0;
  float mExtraXVel = 0;
  TacVector< TacUIButtonCallback > mButtonCallbacks;

  // this is a queue. new infos are pushed back, old infos are popped front
  TacVector< TacUITextTransition > mTransitions;

  float mInitialDelaySecs = 0;

  float buttonXMin = 0;
  float buttonXMax = 0;
  float buttonYMin = 0;
  float buttonYMax = 0;

private:
  bool mTransitionOut = false; // aka: mMenu->mRequestDeletion
  TacUITextData mUITextData;

  // AKA: is most recent change in the call stack
  bool mIsRecentChangeInStack = false;
};

// used as an index
enum class TacUILayoutType
{
  Horizontal = 0,
  Vertical = 1,
};

struct TacUILayout : public TacUILayoutable
{
  TacUILayout( const TacString& debugName );
  ~TacUILayout();
  void DebugImgui() override;
  void Update( TacUILayoutData* uiLayoutData ) override;
  void Render( TacErrors& errors ) override;
   v2 GetWindowspacePosition() override;

  template< typename T >
  T* Add( const TacString& debugName )
  {
    TacUILayoutable* t = new T( debugName );
    t->mUIRoot = mUIRoot;
    t->mParent = this;
    mUILayoutables.push_back( t );
    return ( T* )t;
  }

  float GetInitialDelaySeconds( int index );
  float GetInitialDelaySeconds();
  void RequestDeletion();

  TacUIAnchor mAnchor;

  TacVector< TacUILayoutable* > mUILayoutables;

  // set to 0 or height target based on transition in or out
  float mHeightPrev = 0;
  float mHeightCur = 0;

  // The ui element grows to reach this height
  float mHeightTarget = 300;
  float mTransitionDurationSeconds = 0.5f;
  float mTransitionPerTextInitialDelaySeconds = 0.2f;
  float mTransitionBetweenTextsDurationSeconds = 0.1f;
  double mTransitionStartSeconds = 0;
  bool mTransitionedFinished = false;
  v4 mColor = { 1, 0, 0, 1 };
  bool mRequestDeletion = false;
  TacUILayoutType mUILayoutType = TacUILayoutType::Vertical;
  float mMenuPadding = 0;
  TacTexture* mTexture = nullptr;

  // If true, expands the width of this element to contain its children
  bool mAutoWidth = true;

  // If true, expands the width of this element to fill its encompassing space
  bool mExpandWidth = false;

  //TacUILayout* mLeftEdge = nullptr;

};

// The layout min and max is relative to the parent's anchored position
struct TacUILayoutData
{
  v2 mUIMin = {};
  v2 mUIMax = {};
};


enum class TacUISplit
{
  Before,
  After,
};

struct TacUIHierarchyVisual
{
  virtual void Render(  TacErrors& errors ) {}
  virtual TacString GetDebugName() = 0;
  TacUIHierarchyNode* mHierarchyNode = nullptr;
  v2 mDims;
};

struct TacUIHierarchyVisualText : public TacUIHierarchyVisual
{
  void Render( TacErrors& errors ) override;
  TacString GetDebugName() override { return mUITextData.mUtf8; }
  TacUITextData mUITextData;
};

struct TacUIHierarchyVisualImage : public TacUIHierarchyVisual
{
  void Render( TacErrors& errors ) override;
  TacString GetDebugName() override;
  TacTexture* mTexture = nullptr;
};

struct TacUIHierarchyNode
{
  TacUIHierarchyNode();
  TacUIHierarchyNode* Split(
    TacUISplit uiSplit = TacUISplit::After,
    TacUILayoutType layoutType = TacUILayoutType::Horizontal );
  void RenderHierarchy( TacErrors& errors );
  void SetVisual( TacUIHierarchyVisual* visual );
  TacUIHierarchyNode* AddChild();
  void Expand();
  TacString DebugGenerateGraphVizDotFile();

  v4 mColor = { 1, 1, 1, 1 };
  TacVector< TacUIHierarchyNode* > mChildren;
  TacUIHierarchyNode* mParent = nullptr;

  // use injection instead?
  TacUIRoot* mUIRoot = nullptr;

  bool mDrawOutline;

  TacUILayoutType mLayoutType = TacUILayoutType::Horizontal;

  v2 mPositionRelativeToParent = {};
  v2 mPositionRelativeToRoot = {};

  TacString mDebugName;

  TacEvent<>::Emitter mOnClickEventEmitter;


  // This variable is used...
  //
  // to hold the dims of the box to show thie space this node takes up,
  // especially in the case of the uiroot's hierarchy root, where
  // the size is equal to the window dims
  //
  // It starts with a non-zero default size so that you can see the
  // layout background in absence of a visual, or before a visual has been set
  v2 mSize = { 50, 50 };

  // I'm thinking about replacing this with grid.width="*" kind of deal
  // so that I can center a window between two *s
  int mExpandingChildIndex = 0;

  TacUIHierarchyVisual* mVisual = nullptr;
};

struct TacImGuiWindow
{
  void Text( const TacString& utf8 );

  void Begin();
  

  v2 mCursorDrawPos = {};
  TacUIRoot* mUIRoot = nullptr;
};

struct TacUIRoot
{
  TacUIRoot();
  ~TacUIRoot();
  TacUILayout* AddMenu( const TacString& debugName );
  void Update();
  void Render( TacErrors& errors );
  void DebugImgui();
  
  // Unified getter because I can't make up my mind how to store this variable ( Shell*? )
  double GetElapsedSeconds() { return *mElapsedSeconds; }


  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacFontStuff* mFontStuff = nullptr;
  TacKeyboardInput* mKeyboardInput = nullptr;


  // | The ui shouldn't know about the window it's rendering to
  // | because it could be rendererd to a texture
  // |
  // | Although the above is true, we still need to know if our window
  // | is the cursor unobscured window
  // v
  TacDesktopWindow* mDesktopWindow = nullptr;
  double* mElapsedSeconds = 0;

  TacLanguage mDefaultLanguage = TacLanguage::English;
  std::list< TacUILayout* > mUIMenus;
  float transitionDurationSeconds;

  // TODO: add notes on coordinates
  //       ...
  //       ...?
  v2 mUiCursor = {};

  bool mShouldDebugDrawButtonAreas = false;


  ////////////////////////
  // BEGIN EXPERIMENTAL //
  ////////////////////////

  TacUIHierarchyNode* mHierarchyRoot = nullptr;
  TacImGuiWindow* mImGuiWindow = nullptr;
  
  //////////////////////
  // END EXPERIMENTAL //
  //////////////////////
};

