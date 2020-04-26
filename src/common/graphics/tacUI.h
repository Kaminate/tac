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

#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacEvent.h"
#include "src/common/tacLocalization.h"
#include "src/common/containers/tacVector.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector4.h"
#include "src/common/graphics/tacRenderer.h"
#include <list>

namespace Tac
{


  struct UIAnchor;
  struct UIButtonCallback;
  struct UITextInfo;
  struct UITextData;
  struct UITextTransition;
  struct UIText;
  struct UIRoot;
  struct UILayout;
  struct UILayoutable;
  struct UILayoutData;
  struct UI2DDrawData;
  struct UIHierarchyNode;
  struct DepthBuffer;
  struct DesktopWindow;
  struct KeyboardInput;
  struct Texture;
  struct FontStuff;


  enum class UIAnchorHorizontal
  {
    Center,
    Left,
    Right,
    Count
  };
  const String AnchorHorizontalStrings[] =
  {
    "Center",
    "Left",
    "Right"
  };

  enum class UIAnchorVertical
  {
    Center,
    Top,
    Bottom,
    Count
  };
  const String AnchorVerticalStrings[] =
  {
    "Center",
    "Top",
    "Bottom"
  };

  struct UIAnchor
  {
    void DebugImgui();

    // Changed the defaults from
    // Horizontal Center, Vertical Center to
    // Horizontal Left, Vertical Top...
    UIAnchorHorizontal mAnchorHorizontal = UIAnchorHorizontal::Left;
    UIAnchorVertical mAnchorVertical = UIAnchorVertical::Top;
  };


  struct UILayoutable // yee haw
  {
    UILayoutable( const String& debugName );
    virtual ~UILayoutable() = default;
    virtual void DebugImgui();
    virtual void Update( UILayoutData* ) {};
    virtual void TransitionOut() {};
    virtual void TransitionIn() {};
    virtual void Render( Errors& errors );
    virtual bool IsHovered();
    virtual v2 GetWindowspacePosition();

    UILayoutable* mParent = nullptr;
    UIRoot* mUIRoot = nullptr;

    // Advance width
    // ^ what does this mean?
    // | why is this zero?
    float mUiWidth = 0;

    // Computed, should not be modified directly
    float mUiHeight = 0;

    // Final uispace position of the ui element, after having been anchored and offset.
    // Computed automatically
    v2 mPositionAnchored = {};

    String mDebugName;
    bool mDebug = false;

    v2 mLocalPosition = {};
  };

  // todo: replace with event
  struct UIButtonCallback
  {
    void* mUserData = nullptr;
    void( *mUserCallback )( void*, Errors& errors ) = nullptr;
  };

  struct UITextData
  {
    void DebugImgui();

    v4 mColor = { 1, 1, 0, 1 };
    String mUtf8;
    int mFontSize = 20;

    // Q: why does text need to know its width and height?
    // Q: can this be deleted?
    // Q: should this be set by the user?
    float mUIWidth = 0;
    float mUIHeight = 0;
  };

  struct UITextTransition
  {
    void Update();
    void DebugImgui();

    double mTransitionStartSeconds = 0;
    bool mTransitionFinished = false;
    float mTransitionTweenPercent = 0;
    UIText* mUIText = nullptr;
    bool mUseUITextDataStore = false;
    UITextData mUITextDataStore;
  };

  struct UIText : public UILayoutable
  {
    UIText( const String& debugName );
    ~UIText();
    void UpdateTransitions();
    void Update( UILayoutData* uiLayoutData ) override;
    void DebugImgui()override;
    void TransitionOut()override;
    void TransitionIn()override;
    void Render( Errors& errors ) override;
    bool IsHovered() override;
    void GoNuts();
    const UITextData* GetUITextData();
    void SetText( const UITextData& uiTextData, bool updateSK = true );
    // mIsRecentChangeInSK or mTransitionOut changed
    void Think();
    bool IsMostRecentChangeInSK();

    bool mIsHovered = false;
    double mHoverStartSeconds = 0;

    // comment?
    float mExtraXPos = 0;
    float mExtraXVel = 0;
    Vector< UIButtonCallback > mButtonCallbacks;

    // this is a queue. new infos are pushed back, old infos are popped front
    Vector< UITextTransition > mTransitions;

    float mInitialDelaySecs = 0;

    float buttonXMin = 0;
    float buttonXMax = 0;
    float buttonYMin = 0;
    float buttonYMax = 0;

  private:
    bool mTransitionOut = false; // aka: mMenu->mRequestDeletion
    UITextData mUITextData;
  };

  // used as an index
  enum class UILayoutType
  {
    Horizontal = 0,
    Vertical = 1,
  };

  struct UILayout : public UILayoutable
  {
    UILayout( const String& debugName );
    ~UILayout();
    void DebugImgui() override;
    void Update( UILayoutData* uiLayoutData ) override;
    void Render( Errors& errors ) override;
    v2 GetWindowspacePosition() override;

    template< typename T >
    T* Add( const String& debugName )
    {
      UILayoutable* t = new T( debugName );
      t->mUIRoot = mUIRoot;
      t->mParent = this;
      mUILayoutables.push_back( t );
      return ( T* )t;
    }

    float GetInitialDelaySeconds( int index );
    float GetInitialDelaySeconds();
    void RequestDeletion();

    UIAnchor mAnchor;

    Vector< UILayoutable* > mUILayoutables;

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
    UILayoutType mUILayoutType = UILayoutType::Vertical;
    float mMenuPadding = 0;
    Render::TextureHandle mTexture;

    // If true, expands the width of this element to contain its children
    bool mAutoWidth = true;

    // If true, expands the width of this element to fill its encompassing space
    bool mExpandWidth = false;

    //UILayout* mLeftEdge = nullptr;

  };

  // The layout min and max is relative to the parent's anchored position
  struct UILayoutData
  {
    v2 mUIMin = {};
    v2 mUIMax = {};
  };


  enum class UISplit
  {
    Before,
    After,
  };

  struct UIHierarchyVisual
  {
    virtual ~UIHierarchyVisual() = default;
    virtual void Render( Errors& ) {}
    virtual String GetDebugName() = 0;
    UIHierarchyNode* mHierarchyNode = nullptr;
    v2 mDims;
  };

  struct UIHierarchyVisualText : public UIHierarchyVisual
  {
    virtual ~UIHierarchyVisualText() = default;
    void Render( Errors& errors ) override;
    String GetDebugName() override { return mUITextData.mUtf8; }
    UITextData mUITextData;
  };

  struct UIHierarchyVisualImage : public UIHierarchyVisual
  {
    virtual ~UIHierarchyVisualImage() = default;
    void Render( Errors& errors ) override;
    String GetDebugName() override;
    Render::TextureHandle mTexture;
  };

  struct UIHierarchyNode
  {
    UIHierarchyNode();
    UIHierarchyNode* Split(
      UISplit uiSplit = UISplit::After,
      UILayoutType layoutType = UILayoutType::Horizontal );
    void RenderHierarchy( Errors& errors );
    void SetVisual( UIHierarchyVisual* visual );
    UIHierarchyNode* AddChild();
    void Expand();
    String DebugGenerateGraphVizDotFile();

    v4 mColor = { 1, 1, 1, 1 };
    Vector< UIHierarchyNode* > mChildren;
    UIHierarchyNode* mParent = nullptr;

    // use injection instead?
    UIRoot* mUIRoot = nullptr;

    bool mDrawOutline;

    UILayoutType mLayoutType = UILayoutType::Horizontal;

    v2 mPositionRelativeToParent = {};
    v2 mPositionRelativeToRoot = {};

    String mDebugName;

    Event<>::Emitter mOnClickEventEmitter;


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

    UIHierarchyVisual* mVisual = nullptr;
  };



  struct UIRoot
  {
    UIRoot();
    ~UIRoot();
    UILayout* AddMenu( const String& debugName );
    void Update();
    void Render( Errors& errors );
    void DebugImgui();

    // Unified getter because I can't make up my mind how to store this variable ( Shell*? )
    double GetElapsedSeconds() { return *mElapsedSeconds; }


    UI2DDrawData* mUI2DDrawData = nullptr;
    KeyboardInput* mKeyboardInput = nullptr;


    // | The ui shouldn't know about the window it's rendering to
    // | because it could be rendererd to a texture
    // |
    // | Although the above is true, we still need to know if our window
    // | is the cursor unobscured window
    // v
    DesktopWindow* mDesktopWindow = nullptr;
    double* mElapsedSeconds = 0;

    Language mDefaultLanguage = Language::English;
    std::list< UILayout* > mUIMenus;
    float transitionDurationSeconds;

    // TODO: add notes on coordinates
    //       ...
    //       ...?
    v2 mUiCursor = {};

    bool mShouldDebugDrawButtonAreas = false;


    ////////////////////////
    // BEGIN EXPERIMENTAL //
    ////////////////////////

    UIHierarchyNode* mHierarchyRoot = nullptr;

    //////////////////////
    // END EXPERIMENTAL //
    //////////////////////
  };

}
