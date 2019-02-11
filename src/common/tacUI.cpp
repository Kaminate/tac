#include "common/tacUI.h"
#include "common/imgui.h"
#include "common/tacPreprocessor.h"
#include "common/tackeyboardinput.h"
#include "common/tacShell.h"
#include "common/tacRenderer.h"
#include "common/tacFont.h"
#include "common/math/tacMath.h"
#include "common/tacAlgorithm.h"
#include "common/tacUI2D.h"
#include "common/tacTime.h"
#include "common/tacOS.h"
#include "common/tacDesktopWindow.h"
#include "common/tacColorUtil.h"

static TacUIText* debugOnlyThisText;
static bool IsIgnoring( TacUIText* uiText )
{
  return TacIsDebugMode() && debugOnlyThisText && debugOnlyThisText != uiText;
}

void TacUITextTransition::Update()
{
  if( mTransitionFinished )
    return;
  TacUIRoot* uiRoot = mUIText->mParent->mUIRoot;
  float transitionElapsedSeconds =
    ( float )( uiRoot->GetElapsedSeconds() - mTransitionStartSeconds );
  float transitionElapsedPercent =
    TacSaturate( transitionElapsedSeconds / uiRoot->transitionDurationSeconds );
  if( transitionElapsedPercent >= 1 )
    mTransitionFinished = true;
  mTransitionTweenPercent = TacEaseInOutQuart( transitionElapsedPercent );
}
void TacUITextTransition::DebugImgui()
{
  ImGui::Checkbox( "Data store", &mUseUITextDataStore );
  if( mUseUITextDataStore )
    mUITextDataStore.DebugImgui();
  if( ImGui::Button( "Transition" ) )
  {
    TacUIRoot* uiRoot = mUIText->mParent->mUIRoot;
    mTransitionFinished = false;
    mTransitionStartSeconds = uiRoot->GetElapsedSeconds();
  }
  ImGui::DragDouble( "transition start secs", &mTransitionStartSeconds );
  ImGui::Checkbox( "transition finished", &mTransitionFinished );
  ImGui::DragFloat( "transition tween percent", &mTransitionTweenPercent );
}


void TacUITextData::DebugImgui()
{
  ImGui::Indent();
  ImGui::Text( "TacUITextData" );
  ImGui::ColorEdit4( "color", mColor.data() );
  ImGui::DragInt( "font size", &mFontSize );
  ImGui::InputText( "text", mUtf8 );
  ImGui::Unindent();
}


TacUIText::TacUIText( const TacString& debugName ) :TacUILayoutable( debugName )
{
}
TacUIText::~TacUIText() = default;

void TacUIText::DebugImgui()
{
  ImGui::PushID( this );
  OnDestruct( ImGui::PopID() );

  TacUILayoutable::DebugImgui();

  if( ImGui::Button( "Go Nuts" ) )
    GoNuts();
  if( ImGui::Button( "Debug only this text" ) )
    debugOnlyThisText = this;

  mUITextData.DebugImgui();
  if( ImGui::Button( "Transition out" ) )
    TransitionOut();
  if( ImGui::Button( "Transition in" ) )
    TransitionIn();

  for( TacUITextTransition& uiTextTransition : mTransitions )
  {
    ImGui::PushID( &uiTextTransition );
    OnDestruct( ImGui::PopID() );
    uiTextTransition.DebugImgui();
    if( &uiTextTransition != &mTransitions.back() )
      ImGui::Separator();
  }
}
void TacUIText::Update( TacUILayoutData* uiLayoutData )
{
  if( IsIgnoring( this ) )
    return;
  float posTarget = !mButtonCallbacks.empty() && mIsHovered ? 15.0f : 0;
  float springyness = 15.0f * 4;
  TacSpring( &mExtraXPos, &mExtraXVel, posTarget, springyness, TAC_DELTA_FRAME_SECONDS );

  UpdateTransitions();
  mPositionAnchored.x = uiLayoutData->mUIMin.x + mExtraXPos;
  mPositionAnchored.y = uiLayoutData->mUIMax.y;

  //hack
  bool hovered =
    mUIRoot->mUiCursor.x > uiLayoutData->mUIMin.x && //buttonXMin &&
    mUIRoot->mUiCursor.y > buttonYMin &&
    mUIRoot->mUiCursor.x < uiLayoutData->mUIMax.x && //buttonXMax &&
    mUIRoot->mUiCursor.y < buttonYMax;
  if( hovered != mIsHovered )
  {
    mHoverStartSeconds = mUIRoot->GetElapsedSeconds();
    mIsHovered = hovered;
  }
}
void TacUIText::UpdateTransitions()
{
  int iUiTextInfo = 0;
  auto uiTextInfoCount = ( int )mTransitions.size();
  while( iUiTextInfo < uiTextInfoCount )
  {
    TacUITextTransition& uiTextInfoCur = mTransitions[ iUiTextInfo ];
    uiTextInfoCur.Update();

    int iUiTextInfoNext;
    auto canDelete = [ & ]()
    {
      if( !uiTextInfoCur.mTransitionFinished )
        return false;
      iUiTextInfoNext = iUiTextInfo + 1;
      if( iUiTextInfoNext == uiTextInfoCount )
        return false;
      TacUITextTransition& uiTextInfoNext = mTransitions[ iUiTextInfoNext ];
      if( !uiTextInfoNext.mTransitionFinished )
        return false;
      return true;
    }( );

    if( !canDelete )
    {
      ++iUiTextInfo;
      continue;
    }

    int iUiTextInfoShift = iUiTextInfo;
    for( ;; )
    {
      mTransitions[ iUiTextInfoShift++ ] = mTransitions[ iUiTextInfoNext++ ];
      if( iUiTextInfoNext == uiTextInfoCount )
        break;
    }
    --uiTextInfoCount;
  }
  mTransitions.resize( uiTextInfoCount );
}
void TacUIText::SetText( TacUITextData uiTextData, bool updateStack )
{
  if( updateStack )
  {
    for( TacUITextTransition& uiTextTransition : mTransitions )
    {
      if( uiTextTransition.mUseUITextDataStore )
        continue;
      uiTextTransition.mUseUITextDataStore = true;
      uiTextTransition.mUITextDataStore = mUITextData;
    }
  }
  mUITextData = uiTextData;
  Think();
}
const TacUITextData* TacUIText::GetUITextData()
{
  return &mUITextData;
}
void TacUIText::TransitionOut()
{
  mTransitionOut = true;
  Think();
}
void TacUIText::TransitionIn()
{
  mTransitionOut = false;
  Think();
}
bool TacUIText::IsMostRecentChangeInStack()
{
  for( TacUITextTransition& uiTextTransition : mTransitions )
    if( !uiTextTransition.mUseUITextDataStore )
      return true;
  return false;
}
void TacUIText::Think()
{
  TacUITextTransition uiTextTransition = {};
  uiTextTransition.mTransitionStartSeconds = mParent->mUIRoot->GetElapsedSeconds() + mInitialDelaySecs;
  uiTextTransition.mUIText = this;
  if( mTransitionOut )
  {
    if( !mTransitions.empty() )
    {
      TacUITextTransition& uiTextTransition = mTransitions.back();
      if( uiTextTransition.mUseUITextDataStore && uiTextTransition.mUITextDataStore.mUtf8.empty() )
        return;
    }
    uiTextTransition.mUseUITextDataStore = true;
  }
  else if( IsMostRecentChangeInStack() )
  {
    return;
  }
  mTransitions.push_back( uiTextTransition );
}
void TacUIText::GoNuts()
{
  TacVector< v4 > colors = {
    colorOrange,
    colorGreen,
    colorBlue,
    colorRed,
    colorMagenta,
  };
  int cloneCount = 10;
  int iCloneLast = cloneCount - 1;
  for( int iClone = 0; iClone < cloneCount; ++iClone )
  {
    float clonePercent = ( float )iClone / cloneCount;
    TacUITextTransition transition;
    transition.mUIText = this;
    transition.mTransitionStartSeconds += clonePercent * 0.4f;
    if( iClone != iCloneLast )
    {
      transition.mUseUITextDataStore = true;
      transition.mUITextDataStore = mUITextData;
      transition.mUITextDataStore.mColor = TacRandom( colors );
    }
    mTransitions.push_back( transition );
  }
}
bool TacUIText::IsHovered()
{
  // hack
  return true;
}
void TacUIText::Render( TacErrors& errors )
{
  TacUILayoutable::Render( errors );
  TAC_HANDLE_ERROR( errors );

  TacUIRoot* uiRoot = mParent->mUIRoot;


  if( IsIgnoring( this ) )
    return;
  float extraColorScale = 0.0f;
  if( !mButtonCallbacks.empty() && mIsHovered )
  {
    float hoverElapsedSeconds = float( uiRoot->GetElapsedSeconds() - mHoverStartSeconds );
    float phase = 3.14f / 2.0f;
    float pulsateyness = 6.0f;
    float t = std::sin( pulsateyness  * hoverElapsedSeconds - phase );
    t = ( t + 1 ) / 2; // map -1,1 to 0,1
    extraColorScale = t;
    if( uiRoot->mKeyboardInput->IsKeyJustDown( TacKey::MouseLeft ) )
    {
      for( const TacUIButtonCallback& buttonCallback : mButtonCallbacks )
      {
        buttonCallback.mUserCallback( buttonCallback.mUserData, errors );
        TAC_HANDLE_ERROR( errors );
      }
    }
  }


  TacDrawCall* drawCallPrev = nullptr;
  float heightBetweenBaselines;
  float maxHeightBaselines = 0;


  float uiWidthMax = 0;

  buttonXMin = std::numeric_limits< float >::max();
  buttonXMax = -std::numeric_limits< float >::max();
  buttonYMin = std::numeric_limits< float >::max();
  buttonYMax = -std::numeric_limits< float >::max();

  TacUI2DDrawData* ui2DDrawData = uiRoot->mUI2DDrawData;
  TacUI2DState* state = ui2DDrawData->PushState();
  OnDestruct( ui2DDrawData->PopState() );

  for( int iTransition = 0; iTransition < ( int )mTransitions.size(); ++iTransition )
  {
    const TacUITextTransition& transition = mTransitions[ iTransition ];
    const TacUITextData* uiTextInfo = transition.mUseUITextDataStore ? &transition.mUITextDataStore : GetUITextData();

    v4 uiTextColor = uiTextInfo->mColor;
    uiTextColor.xyz() += v3( 1, 1, 1 ) * extraColorScale;
    state->Translate( mPositionAnchored );
    state->Draw2DText(
      uiRoot->mDefaultLanguage,
      uiTextInfo->mFontSize,
      uiTextInfo->mUtf8,
      &heightBetweenBaselines,
      uiTextColor,
      errors );
    TAC_HANDLE_ERROR( errors );

    //TacDrawCall* drawCall = graphics->GetLastDrawCall();
    //if( drawCallPrev )
    //{
    //  drawCall->mScissorRectMaxUISpace.x = TacMax(
    //    drawCall->mScissorRectMaxUISpace.x,
    //    drawCallPrev->mScissorRectMaxUISpace.x );
    //}
    //if( !transition.mTransitionFinished )
    //{
    //  drawCall->mScissorRectMaxUISpace.x = TacLerp(
    //    drawCall->mScissorRectMinUISpace.x,
    //    drawCall->mScissorRectMaxUISpace.x,
    //    transition.mTransitionTweenPercent );
    //}
    //if( drawCallPrev )
    //{
    //  drawCallPrev->mScissorRectMinUISpace.x = TacMin(
    //    drawCallPrev->mScissorRectMaxUISpace.x,
    //    drawCall->mScissorRectMaxUISpace.x );
    //}
    //drawCallPrev = drawCall;
    //maxHeightBaselines = TacMax( maxHeightBaselines, heightBetweenBaselines );
    //
    //
    //TacAssert( drawCall->mScissorRectMinUISpace.x <= drawCall->mScissorRectMaxUISpace.x );
    //TacAssert( drawCall->mScissorRectMinUISpace.y <= drawCall->mScissorRectMaxUISpace.y );
    //
    //buttonXMin = TacMin( buttonXMin, drawCall->mScissorRectMinUISpace.x );
    //buttonXMax = TacMax( buttonXMax, drawCall->mScissorRectMaxUISpace.x );
    //buttonYMin = TacMin( buttonYMin, drawCall->mScissorRectMinUISpace.y );
    //buttonYMax = TacMax( buttonYMax, drawCall->mScissorRectMaxUISpace.y );
    //
    //
    //
    //
    //uiWidthMax =
    //  drawCall->mScissorRectMaxUISpace.x -
    //  drawCall->mScissorRectMinUISpace.x;
  }

  if( uiRoot->mShouldDebugDrawButtonAreas )
  {
    float buttonW = buttonXMax - buttonXMin;
    float buttonH = buttonYMax - buttonYMin;
    state->mTransform = M3Translate( buttonXMin, buttonYMax );
    state->Draw2DBox( buttonW, buttonH );
  }

  mUiHeight = maxHeightBaselines;
  mUiWidth = uiWidthMax;

}


void TacUIAnchor::DebugImgui()
{
  int currentHItem = ( int )mAnchorHorizontal;
  auto itemHGetter = []( void* data, int idx, const char** outText )
  {
    TacUnusedParameter( data );
    *outText = TacAnchorHorizontalStrings[ idx ].c_str();
    return true;
  };
  if( ImGui::Combo( "Anchor Horizontal", &currentHItem, itemHGetter, nullptr, ( int )TacUIAnchorHorizontal::Count ) )
  {
    mAnchorHorizontal = ( TacUIAnchorHorizontal )currentHItem;
  }

  int currentVItem = ( int )mAnchorVertical;
  auto itemVGetter = []( void* data, int idx, const char** outText )
  {
    TacUnusedParameter( data );
    *outText = TacAnchorVerticalStrings[ idx ].c_str();
    return true;
  };
  if( ImGui::Combo( "Anchor Vertical", &currentVItem, itemVGetter, nullptr, ( int )TacUIAnchorVertical::Count ) )
  {
    mAnchorVertical = ( TacUIAnchorVertical )currentVItem;
  }

}

TacUILayoutable::TacUILayoutable( const TacString& debugName )
{
  mDebugName = debugName;
}
TacUILayoutable::~TacUILayoutable()
{
}
void TacUILayoutable::DebugImgui()
{
  if( !ImGui::CollapsingHeader( "Layout" ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  ImGui::DragFloat( "ui width", &mUiWidth );
  ImGui::DragFloat( "ui height", &mUiHeight );
  ImGui::DragFloat2( "ui position", mPositionAnchored.data() );
  ImGui::Checkbox( "Debug", &mDebug );
}
void TacUILayoutable::Render( TacErrors& errors )
{
  if( mDebug )
  {
    TacUI2DDrawData* ui2DDrawData = mUIRoot->mUI2DDrawData;
    TacUI2DState* state = ui2DDrawData->PushState();
    state->Translate( mPositionAnchored );
    state->Draw2DBox( // Color?
      mUiWidth,
      mUiHeight );
    ui2DDrawData->PopState();
  }
}
bool TacUILayoutable::IsHovered()
{
  bool isHovered =
    mUIRoot->mUiCursor.x > mPositionAnchored.x &&
    mUIRoot->mUiCursor.y < mPositionAnchored.y &&
    mUIRoot->mUiCursor.x < mPositionAnchored.x + mUiWidth &&
    mUIRoot->mUiCursor.y > mPositionAnchored.y - mUiHeight;
  return isHovered;
}
v2 TacUILayoutable::GetWindowspacePosition()
{
  v2 result = mLocalPosition;
  if( mParent )
  {
    v2 parentWindowspacePosition = mParent->GetWindowspacePosition();
    result += parentWindowspacePosition;
  }
  return result;
}

TacUILayout::TacUILayout( const TacString& debugName ) : TacUILayoutable( debugName )
{
  mColor = v4(
    TacRandomFloat0To1(),
    TacRandomFloat0To1(),
    TacRandomFloat0To1(),
    1
  );
}
TacUILayout::~TacUILayout()
{
  for( auto textDrawInfo : mUILayoutables )
    delete textDrawInfo;
}
float TacUILayout::GetInitialDelaySeconds()
{
  auto index = ( int )mUILayoutables.size();
  float result = GetInitialDelaySeconds( index );
  return result;
}
float TacUILayout::GetInitialDelaySeconds( int index )
{
  float result =
    mTransitionPerTextInitialDelaySeconds +
    mTransitionBetweenTextsDurationSeconds * index;
  return result;
}
void TacUILayout::DebugImgui()
{
  TacUILayoutable::DebugImgui();
  auto childrenCount = ( int )mUILayoutables.size();
  if( childrenCount )
  {
    if( ImGui::CollapsingHeader( va( "%i Children", childrenCount ) ) )
    {
      ImGui::Indent();
      OnDestruct( ImGui::Unindent() );
      int iTextDrawInfo = 0;
      for( TacUILayoutable* textDrawInfo : mUILayoutables )
      {
        iTextDrawInfo++;
        if( !ImGui::CollapsingHeader( textDrawInfo->mDebugName.c_str() ) )
          continue;

        ImGui::Indent();
        OnDestruct( ImGui::Unindent() );

        ImGui::PushID( textDrawInfo );
        OnDestruct( ImGui::PopID() );

        textDrawInfo->DebugImgui();
      }
    }
  }
  mAnchor.DebugImgui();
  //ImGui::DragFloat2( "Position", mPosition.data() );
  ImGui::DragFloat( "Width", &mUiWidth );
  ImGui::DragFloat( "Height Cur", &mHeightCur );
  ImGui::DragFloat( "Height Target", &mHeightTarget );
  ImGui::ColorEdit4( "Color", mColor.data() );
  ImGui::DragFloat( "mTransitionPerTextInitialDelaySeconds", &mTransitionPerTextInitialDelaySeconds );
  ImGui::DragFloat( "mTransitionBetweenTextsDurationSeconds", &mTransitionBetweenTextsDurationSeconds );
  if( ImGui::Button( "Delete menu" ) )
    RequestDeletion();
  if( TacTexture* texture = mTexture )
  {
    float w = 0.9f * ImGui::GetContentRegionAvailWidth();
    float h = w / texture->GetAspect();
    ImGui::Image( texture->GetImguiTextureID(), ImVec2( w, h ) );
  }
}
void TacUILayout::Update( TacUILayoutData* uiLayoutData )
{
  if( !mUILayoutables.empty() )
  {
    float runningAutoWidth = 0;
    bool hasRightAlignedChild = false;
    for( TacUILayoutable* uiLayoutable : mUILayoutables )
    {
      uiLayoutable->Update( uiLayoutData );

      // TODO: this line of code assumes the child is LEFT-ALIGNED
      runningAutoWidth += uiLayoutable->mUiWidth;
    }
    if( mAutoWidth )
      mUiWidth = runningAutoWidth;
  }

  if( mExpandWidth )
  {
    mUiWidth = mParent ?
      mParent->mUiWidth :
      mUIRoot->mUI2DDrawData->mRenderView->mFramebuffer->myImage.mWidth;
  }

  //mPositionAnchored = mPosition;
  mHeightCur = mHeightTarget;
  mUiHeight = mHeightCur;

#if 0
  v2 paddingVec = v2( 1, 1 ) * mMenuPadding;
  uiLayoutData->mUIMin += paddingVec;
  uiLayoutData->mUIMax -= paddingVec;
  v2 contentDimensions = {};

  TacUILayoutData childLayout = {};

  for( TacUILayoutable* uiLayoutable : mUILayoutables )
  {
    uiLayoutable->Update( uiLayoutData );

    switch( mUILayoutType )
    {
    case TacUILayoutType::Vertical:
      contentDimensions.x = TacMax( contentDimensions.x, uiLayoutable->mUiWidth );
      contentDimensions.y += uiLayoutable->mUiHeight;
      uiLayoutData->mUIMax.y += uiLayoutable->mUiHeight;
      break;
    case TacUILayoutType::Horizontal:
      contentDimensions.y = TacMax( contentDimensions.y, uiLayoutable->mUiHeight );
      contentDimensions.x += uiLayoutable->mUiWidth;
      uiLayoutData->mUIMax.x += uiLayoutable->mUiWidth;
      break;
      TacInvalidDefaultCase( mUILayoutType );
    }
  }





  v2 anchoredPenPos = mPosition;
  switch( mAnchor.mAnchorHorizontal )
  {
  case TacUIAnchorHorizontal::Left: anchoredPenPos.x += uiLayoutData->mUIMin.x; break;
  case TacUIAnchorHorizontal::Right: anchoredPenPos.x += uiLayoutData->mUIMax.x - mUiWidth; break;
  case TacUIAnchorHorizontal::Center: anchoredPenPos.x +=
    ( uiLayoutData->mUIMin.x + uiLayoutData->mUIMax.x - mUiWidth ) * 0.5f; break;
  }


  switch( mAnchor.mAnchorVertical )
  {
  case TacUIAnchorVertical::Top: anchoredPenPos.y += uiLayoutData->mUIMin.y; break;
  case TacUIAnchorVertical::Bottom: anchoredPenPos.y += uiLayoutData->mUIMax.y - mUiHeight; break;
  case TacUIAnchorVertical::Center: anchoredPenPos.y +=
    ( uiLayoutData->mUIMin.y + uiLayoutData->mUIMax.y - mUiHeight ) * 0.5f; break;
  }

  mPositionAnchored = anchoredPenPos;


  if( !mTransitionedFinished )
  {
    float percent = ( float )( mUIRoot->GetElapsedSeconds() - mTransitionStartSeconds ) / mTransitionDurationSeconds;
    if( percent > 1 )
    {
      percent = 1;
      mTransitionedFinished = true;
      mHeightPrev = mHeightTarget;
    }
    float t = TacEaseInOutQuart( percent );
    mHeightCur = TacLerp( mHeightPrev, mHeightTarget, t );
  }

  if( !mUILayoutables.empty() )
  {
    uiLayoutData->mUIMin = mPositionAnchored + paddingVec;
    if( !mAutoWidth )
      uiLayoutData->mUIMax = mPositionAnchored - paddingVec + v2( mUiWidth, mUiHeight );

    float contentWidth = 0;
    float contentHeight = 0;

    for( TacUILayoutable* uiLayoutable : mUILayoutables )
    {
      uiLayoutable->Update( uiLayoutData );

      switch( mUILayoutType )
      {
      case TacUILayoutType::Vertical:
        contentHeight += uiLayoutable->mUiHeight;
        uiLayoutData->mUIMin.y -= uiLayoutable->mUiHeight;
        break;
      case TacUILayoutType::Horizontal:
        contentWidth += uiLayoutable->mUiWidth;
        uiLayoutData->mUIMax.x += uiLayoutable->mUiWidth;
        break;
        TacInvalidDefaultCase( mUILayoutType );
      }
    }
    if( mAutoWidth )
      mUiWidth = TacMax( mUiWidth, contentWidth );
  }

  mUiHeight = mHeightCur;

  if( mExpandWidth )
    mUiWidth = uiLayoutData->mUIMax.x - uiLayoutData->mUIMin.x;
#endif
}
void TacUILayout::RequestDeletion()
{
  mHeightTarget = 0;
  mTransitionStartSeconds = mUIRoot->GetElapsedSeconds();
  mTransitionedFinished = false;
  mRequestDeletion = true;
  int iDelay = ( int )mUILayoutables.size() - 1;
  for( TacUILayoutable* uiText : mUILayoutables )
    uiText->TransitionOut();
  mTransitionDurationSeconds = 2.3f;
}
void TacUILayout::Render( TacErrors& errors )
{
  TacUILayoutable::Render( errors );
  TAC_HANDLE_ERROR( errors );
  TacUI2DDrawData* ui2DDrawData = mUIRoot->mUI2DDrawData;
  TacUI2DState* state = ui2DDrawData->PushState();

  v4 menuColor = mColor;
  if( IsHovered() )
    menuColor.xyz() *= 1.30f;

  v2 windowSpacePosition = GetWindowspacePosition();
  state->Translate( windowSpacePosition );
  state->Draw2DBox(
    mUiWidth,
    mUiHeight,
    menuColor,
    mTexture );

  for( TacUILayoutable* uiLayoutable : mUILayoutables )
  {
    uiLayoutable->Render( errors );
    TAC_HANDLE_ERROR( errors );
  }
  ui2DDrawData->PopState();
}
v2 TacUILayout::GetWindowspacePosition()
{
  v2 windowSpacePos = mLocalPosition;
  if( mParent )
  {
    v2 parentWindowSpacePos = mParent->GetWindowspacePosition();
    float parentWidth = mParent->mUiWidth;
    switch( mAnchor.mAnchorHorizontal )
    {
    case TacUIAnchorHorizontal::Left:
      windowSpacePos.x += parentWindowSpacePos.x; break;
    case TacUIAnchorHorizontal::Right:
      windowSpacePos.x += parentWindowSpacePos.x + mParent->mUiWidth - mUiWidth; break;
    case TacUIAnchorHorizontal::Center:
      windowSpacePos.x += parentWindowSpacePos.x + ( mParent->mUiWidth + mUiWidth ) / 2; break;
    }
  }
  return windowSpacePos;
}

TacUIRoot::TacUIRoot()
{
  transitionDurationSeconds = 0.3f;
  mHierarchyRoot = new TacUIHierarchyNode();
  mHierarchyRoot->mUIRoot = this;
}
TacUIRoot::~TacUIRoot()
{
  for( auto menu : mUIMenus )
    delete menu;
}
void TacUIRoot::DebugImgui()
{
  if( !ImGui::CollapsingHeader( "UI Root" ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  ImGui::DragFloat( "Transition Duration Sec", &transitionDurationSeconds );
  ImGui::Checkbox( "Debug button areas", &mShouldDebugDrawButtonAreas );
  int iUIMenu = 0;
  bool deleteMenus = ImGui::Button( "Delete all ui" );
  for( TacUILayout* uiMenu : mUIMenus )
  {
    if( deleteMenus )
      uiMenu->RequestDeletion();
    iUIMenu++;
    if( !ImGui::CollapsingHeader( uiMenu->mDebugName.c_str() ) )
      continue;

    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );

    ImGui::PushID( uiMenu );
    OnDestruct( ImGui::PopID() );

    uiMenu->DebugImgui();
  }
}
TacUILayout* TacUIRoot::AddMenu( const TacString& debugName )
{
  auto* uiMenu = new TacUILayout( debugName );
  uiMenu->mUIRoot = this;
  uiMenu->mTransitionStartSeconds = GetElapsedSeconds();
  mUIMenus.push_back( uiMenu );
  return uiMenu;
}
void TacUIRoot::Render( TacErrors& errors )
{
  TacTexture* framebuffer = mUI2DDrawData->mRenderView->mFramebuffer;
  //TacUI2DState* state = mUI2DDrawData->PushState();
  //state->mTransform = M3Scale(
  //  framebuffer->myImage.mWidth / mUIWidth,
  //  framebuffer->myImage.mHeight / mUIHeight,
  //  1 );
  //mUI2DDrawData->PopState();
  mHierarchyRoot->RenderHierarchy( errors );
  TAC_HANDLE_ERROR( errors );

  for( TacUILayout* uiMenu : mUIMenus )
  {
    uiMenu->Render( errors );
    TAC_HANDLE_ERROR( errors );
  }
}
void TacUIRoot::Update()
{
  TacTexture* framebuffer = mUI2DDrawData->mRenderView->mFramebuffer;
  float aspect = framebuffer->GetAspect();
  //float uiWidth = 1024;
  //float uiHeight = 1024;
  //if( aspect > 1 )
  //  uiWidth *= aspect;
  //else
  //  uiHeight /= aspect;
  //mUIWidth = uiWidth;
  //mUIHeight = uiHeight;

  //v2 cursorPos;
  //TacErrors cursorPosErrors;
  //TacOS::Instance->GetScreenspaceCursorPos( cursorPos, cursorPosErrors );
  //if( cursorPosErrors.empty() )
  //  mUiCursor = cursorPos - v2( ( float )mDesktopWindow->mX, ( float )mDesktopWindow->mY );

  TacVector< TacUILayout* > toDelete;
  for( TacUILayout* uiMenu : mUIMenus )
  {
    TacUILayoutData uiLayoutData;
    uiLayoutData.mUIMin = {};
    //uiLayoutData.mUIMax = { uiWidth, uiHeight };
    uiLayoutData.mUIMax = {
      ( float )framebuffer->myImage.mWidth,
      ( float )framebuffer->myImage.mHeight };
    uiMenu->Update( &uiLayoutData );
    if( uiMenu->mRequestDeletion && uiMenu->mTransitionedFinished )
      toDelete.push_back( uiMenu );
  }
  for( TacUILayout* uiMenu : toDelete )
  {
    mUIMenus.erase( std::find( mUIMenus.begin(), mUIMenus.end(), uiMenu ) );
    delete uiMenu;
  }

  TacImage& image = framebuffer->myImage;

  mHierarchyRoot->mSize = {
    ( float )image.mWidth,
    ( float )image.mHeight };
}
TacString TacUIRoot::DebugGenerateGraphVizDotFile()
{
  TacString stringified;

  // https://en.wikipedia.org/wiki/DOT_(graph_description_language)
  // 
  // digraph graphname    a
  // {                    |
  //   a->b->c;           b
  //   b->d;             / \
  // }                  c   d


  TacVector<TacString> lines;
  lines.push_back( "digraph graphname" );
  lines.push_back( "{" );

  std::set< TacUIHierarchyNode* > allnodes;
  std::map< TacUIHierarchyNode*, TacString > nodeGraphNames;
  TacVector< std::pair< TacUIHierarchyNode*, TacUIHierarchyNode* >> nodeConnections;

  if( mHierarchyRoot )
  {
    TacVector< TacUIHierarchyNode* > nodes = { mHierarchyRoot };
    while( !nodes.empty() )
    {
      TacUIHierarchyNode* node = nodes.back();
      nodes.pop_back();
      allnodes.insert( node );

      for( TacUIHierarchyNode* child : node->mChildren )
      {
        nodes.push_back( child );
        nodeConnections.push_back( { node, child } );
      }
    }
  }

  for( TacUIHierarchyNode* node : allnodes )
  {
    TacString nodeGraphName = "node" + TacToString( ( int )nodeGraphNames.size() );
    TacString nodeGraphLabel = TacToString( node );

    if( node->mDebugName.size() )
      nodeGraphLabel = node->mDebugName;
    else if( node == mHierarchyRoot )
      nodeGraphLabel = "hierarchy root";
    else if( node->GetVisual() && node->GetVisual()->GetDebugName().size() )
      nodeGraphLabel = node->GetVisual()->GetDebugName();

    nodeGraphNames[ node ] = nodeGraphName;

    lines.push_back( nodeGraphName + "[ label = \"" + nodeGraphLabel + "\" ];" );
  }

  for( auto connection : nodeConnections )
  {
    lines.push_back(
      nodeGraphNames[ connection.first ] + "->" + nodeGraphNames[ connection.second ] );
  }

  lines.push_back( "}" );
  stringified = TacJoin( "\n", lines );
  return stringified;

}

TacUIHierarchyNode::TacUIHierarchyNode()
{
  static int iColor;
  auto colors = TacMakeArray< int > (
    0x53c8e9, 
    0x3c3c3b,
    0x074b7f,
    0xc82c60,
    0xf9fff9 );
  int color = colors[ iColor++ % colors.size() ];
  mColor.xyz() = TacHexToRGB( color );
  mColor.w = 1;
  mDrawOutline = true;
}
TacUIHierarchyNode* TacUIHierarchyNode::Split(
  TacUISplit uiSplit,
  TacUILayoutType layoutType )
{
  // TODO: I'm sure this whole if else chain can be simplified into something elegant
  TacUIHierarchyNode* parent = nullptr;
  if( mChildren.size() )
  {
    if( layoutType == mLayoutType )
    {
      parent = this;
    }
    else if( !mParent )
    {
      // get my parent who is vertically split
      parent = new TacUIHierarchyNode();
      parent->mChildren = { this };
      parent->mUIRoot = mUIRoot;
      parent->mLayoutType = layoutType;
      mUIRoot->mHierarchyRoot = parent;
      mParent = parent;
    }
  }
  else if( !mParent )
  {
    parent = new TacUIHierarchyNode();
    parent->mChildren = { this };
    parent->mUIRoot = mUIRoot;
    parent->mLayoutType = layoutType;
    mUIRoot->mHierarchyRoot = parent;
    mParent = parent;
  }
  else
  {
    if( mParent )
    {
      if( mParent->mLayoutType == layoutType )
      {
        parent = mParent;
      }
      else
      {
        parent = new TacUIHierarchyNode();
        parent->mChildren = { this };
        parent->mUIRoot = mUIRoot;
        parent->mLayoutType = layoutType;
        parent->mParent = mParent;
        int n = mParent->mChildren.size();
        bool exchanged = false;
        for( int i = 0; i < n; ++i )
        {
          TacUIHierarchyNode* child = mParent->mChildren[ i ];
          if( child != this )
            continue;
          mParent->mChildren[ i ] = parent;
          mParent = parent;
          exchanged = true;
          break;
        }
        TacAssert( exchanged );
      }
    }
  }

  TacAssert( parent );

  auto newChild = new TacUIHierarchyNode();
  newChild->mUIRoot = mUIRoot;
  newChild->mParent = parent;

  switch( uiSplit )
  {
  case TacUISplit::Before: {
    int oldChildCount = parent->mChildren.size();
    parent->mChildren.resize( oldChildCount + 1 );
    for( int i = 0; i < oldChildCount; ++i )
    {
      parent->mChildren[ oldChildCount - i ] =
        parent->mChildren[ oldChildCount - i - 1 ];
    }
    parent->mChildren[ 0 ] = newChild;
    parent->mExpandingChildIndex++;
  } break;
  case TacUISplit::After: {
    parent->mChildren.push_back( newChild );
  } break;
    TacInvalidDefaultCase( uiSplit );
  }

  return newChild;
}
void TacUIHierarchyNode::RenderHierarchy( TacErrors& errors )
{
  // compute children sizes
  if( !mChildren.empty() )
  {
    float expandedChildSize = mSize[ ( int )mLayoutType ];
    TacUIHierarchyNode* expandedChild = mChildren[ mExpandingChildIndex ];
    for( TacUIHierarchyNode* child : mChildren )
    {
      child->mSize[ 1 - ( int )mLayoutType ] = mSize[ 1 - ( int )mLayoutType ];
      if( child == expandedChild )
        continue;
      if( child->mVisual )
      {
        child->mSize[ ( int )mLayoutType ] = child->mVisual->mDims[ ( int )mLayoutType ];
      }
      expandedChildSize -= child->mSize[ ( int )mLayoutType ];
    }
    expandedChild->mSize[ ( int )mLayoutType ] = expandedChildSize;
  }

  TacUI2DState* state = mUIRoot->mUI2DDrawData->PushState();
  float padding = 0;
  state->Translate( mPositionRelativeToParent + v2( 1, 1 ) * padding );
  if( mDrawOutline )
  {
      v4 color = mColor;
    if( mUIRoot->mDesktopWindow->mCursorUnobscured &&
      mUIRoot->mUiCursor.x > mPositionRelativeToRoot.x &&
      mUIRoot->mUiCursor.x < mPositionRelativeToRoot.x + mSize.x &&
      mUIRoot->mUiCursor.y > mPositionRelativeToRoot.y  &&
      mUIRoot->mUiCursor.y < mPositionRelativeToRoot.y + mSize.y )
    {
      // magic
      color.xyz() = ( color.xyz().Length() < 0.5f ? color.xyz() + v3( 1, 1, 1 ) : color.xyz() ) / 2;
    }

    bool debugdrawNodeBackground = true;
    if( debugdrawNodeBackground )
    {
      state->Draw2DBox(
        mSize[ 0 ] - ( padding * 2 ),
        mSize[ 1 ] - ( padding * 2 ),
        color );
    }

    bool debugdrawNodeName = true;
    if( debugdrawNodeName )
    {
      v4 debugTextColor = mColor.xyz().Length() < 0.5f ? v4( 1, 1, 1, 1 ) : v4( 0, 0, 0, 1 );
      debugTextColor = { 0, 0, 0, 1 };
      state->Draw2DText( TacLanguage::English, 16, mDebugName, nullptr, debugTextColor, errors );
      TAC_HANDLE_ERROR( errors );
    }
  }
  if( mVisual )
  {
    mVisual->Render( errors );
    TAC_HANDLE_ERROR( errors );
  }
  float runningPixelX = 0;
  for( TacUIHierarchyNode* child : mChildren )
  {
    v2 childPosition = {};
    childPosition[ ( int )mLayoutType ] += runningPixelX;
    child->mPositionRelativeToRoot = mPositionRelativeToRoot + childPosition;
    child->mPositionRelativeToParent = childPosition;
    child->RenderHierarchy( errors );
    TAC_HANDLE_ERROR( errors );
    runningPixelX += child->mSize[ ( int )mLayoutType ];
  }
  mUIRoot->mUI2DDrawData->PopState();
}

void TacUIHierarchyVisualText::Render( TacErrors& errors )
{
  if( mUITextData.mUtf8.empty() )
    return;
  float why_the_fuck_do_i_care_about_this;
  TacUIRoot* uiRoot = mHierarchyNode->mUIRoot;
  uiRoot->mUI2DDrawData->mStates.back().Draw2DText(
    uiRoot->mDefaultLanguage,
    mUITextData.mFontSize,
    mUITextData.mUtf8,
    &why_the_fuck_do_i_care_about_this,
    mUITextData.mColor,
    errors );
  TAC_HANDLE_ERROR( errors );
}

void TacUIHierarchyVisualImage::Render( TacErrors& errors )
{
  TacUIRoot* uiRoot = mHierarchyNode->mUIRoot;
  uiRoot->mUI2DDrawData->mStates.back().Draw2DBox(
    mDims[ 0 ],
    mDims[ 1 ],
    v4( 1, 1, 1, 1 ),
    mTexture );
}

TacString TacUIHierarchyVisualImage::GetDebugName()
{
  return mTexture->mName;
}
