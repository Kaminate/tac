#include "src/common/tacPreprocessor.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacShell.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacTime.h"
#include "src/common/tacUtility.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/tacColorUtil.h"

#include <cmath> // sin

namespace Tac
{


  static UIText* debugOnlyThisText;
  static bool IsIgnoring( UIText* uiText )
  {
    return IsDebugMode() && debugOnlyThisText && debugOnlyThisText != uiText;
  }

  void UITextTransition::Update()
  {
    if( mTransitionFinished )
      return;
    UIRoot* uiRoot = mUIText->mParent->mUIRoot;
    float transitionElapsedSeconds =
      ( float )( uiRoot->GetElapsedSeconds() - mTransitionStartSeconds );
    float transitionElapsedPercent =
      Saturate( transitionElapsedSeconds / uiRoot->transitionDurationSeconds );
    if( transitionElapsedPercent >= 1 )
      mTransitionFinished = true;
    mTransitionTweenPercent = EaseInOutQuart( transitionElapsedPercent );
  }
  void UITextTransition::DebugImgui()
  {
    //ImGui::Checkbox( "Data store", &mUseUITextDataStore );
    //if( mUseUITextDataStore )
    //  mUITextDataStore.DebugImgui();
    //if( ImGui::Button( "Transition" ) )
    //{
    //  UIRoot* uiRoot = mUIText->mParent->mUIRoot;
    //  mTransitionFinished = false;
    //  mTransitionStartSeconds = uiRoot->GetElapsedSeconds();
    //}
    //ImGui::DragDouble( "transition start secs", &mTransitionStartSeconds );
    //ImGui::Checkbox( "transition finished", &mTransitionFinished );
    //ImGui::DragFloat( "transition tween percent", &mTransitionTweenPercent );
  }


  void UITextData::DebugImgui()
  {
    //ImGui::Indent();
    //ImGui::Text( "UITextData" );
    //ImGui::ColorEdit4( "color", mColor.data() );
    //ImGui::DragInt( "font size", &mFontSize );
    //ImGui::InputText( "text", mUtf8 );
    //ImGui::Unindent();
  }


  UIText::UIText( StringView debugName ) : UILayoutable( debugName )
  {
  }
  UIText::~UIText() = default;

  void UIText::DebugImgui()
  {
    //ImGui::PushID( this );
    //OnDestruct( ImGui::PopID() );

    //UILayoutable::DebugImgui();

    //if( ImGui::Button( "Go Nuts" ) )
    //  GoNuts();
    //if( ImGui::Button( "Debug only this text" ) )
    //  debugOnlyThisText = this;

    //mUITextData.DebugImgui();
    //if( ImGui::Button( "Transition out" ) )
    //  TransitionOut();
    //if( ImGui::Button( "Transition in" ) )
    //  TransitionIn();

    //for( UITextTransition& uiTextTransition : mTransitions )
    //{
    //  ImGui::PushID( &uiTextTransition );
    //  OnDestruct( ImGui::PopID() );
    //  uiTextTransition.DebugImgui();
    //  if( &uiTextTransition != &mTransitions.back() )
    //    ImGui::Separator();
    //}
  }
  void UIText::Update( UILayoutData* uiLayoutData )
  {
    if( IsIgnoring( this ) )
      return;
    float posTarget = !mButtonCallbacks.empty() && mIsHovered ? 15.0f : 0;
    float springyness = 15.0f * 4;
    Spring( &mExtraXPos, &mExtraXVel, posTarget, springyness, TAC_DELTA_FRAME_SECONDS );

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
  void UIText::UpdateTransitions()
  {
    int iUiTextInfo = 0;
    auto uiTextInfoCount = ( int )mTransitions.size();
    while( iUiTextInfo < uiTextInfoCount )
    {
      UITextTransition& uiTextInfoCur = mTransitions[ iUiTextInfo ];
      uiTextInfoCur.Update();

      int iUiTextInfoNext;
      auto canDelete = [ & ]()
      {
        if( !uiTextInfoCur.mTransitionFinished )
          return false;
        iUiTextInfoNext = iUiTextInfo + 1;
        if( iUiTextInfoNext == uiTextInfoCount )
          return false;
        UITextTransition& uiTextInfoNext = mTransitions[ iUiTextInfoNext ];
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
  void UIText::SetText( const UITextData& uiTextData, bool updateSK )
  {
    if( updateSK )
    {
      for( UITextTransition& uiTextTransition : mTransitions )
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
  const UITextData* UIText::GetUITextData()
  {
    return &mUITextData;
  }
  void UIText::TransitionOut()
  {
    mTransitionOut = true;
    Think();
  }
  void UIText::TransitionIn()
  {
    mTransitionOut = false;
    Think();
  }
  bool UIText::IsMostRecentChangeInSK()
  {
    for( UITextTransition& uiTextTransition : mTransitions )
      if( !uiTextTransition.mUseUITextDataStore )
        return true;
    return false;
  }
  void UIText::Think()
  {
    UITextTransition uiTextTransition = {};
    uiTextTransition.mTransitionStartSeconds = mParent->mUIRoot->GetElapsedSeconds() + mInitialDelaySecs;
    uiTextTransition.mUIText = this;
    if( mTransitionOut )
    {
      if( !mTransitions.empty() )
      {
        UITextTransition& backuiTextTransition = mTransitions.back();
        if( backuiTextTransition.mUseUITextDataStore && backuiTextTransition.mUITextDataStore.mUtf8.empty() )
          return;
      }
      uiTextTransition.mUseUITextDataStore = true;
    }
    else if( IsMostRecentChangeInSK() )
    {
      return;
    }
    mTransitions.push_back( uiTextTransition );
  }
  void UIText::GoNuts()
  {
    Vector< v4 > colors = {
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
      UITextTransition transition;
      transition.mUIText = this;
      transition.mTransitionStartSeconds += clonePercent * 0.4f;
      if( iClone != iCloneLast )
      {
        transition.mUseUITextDataStore = true;
        transition.mUITextDataStore = mUITextData;
        transition.mUITextDataStore.mColor = Random( colors );
      }
      mTransitions.push_back( transition );
    }
  }
  bool UIText::IsHovered()
  {
    // hack
    return true;
  }
  void UIText::Render( Errors& errors )
  {
    UILayoutable::Render( errors );
    TAC_HANDLE_ERROR( errors );

    UIRoot* uiRoot = mParent->mUIRoot;


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
      if( KeyboardInput::Instance->IsKeyJustDown( Key::MouseLeft ) )
      {
        for( const UIButtonCallback& buttonCallback : mButtonCallbacks )
        {
          buttonCallback.mUserCallback( buttonCallback.mUserData, errors );
          TAC_HANDLE_ERROR( errors );
        }
      }
    }


    float heightBetweenBaselines;
    float maxHeightBaselines = 0;


    float uiWidthMax = 0;

    buttonXMin = std::numeric_limits< float >::max();
    buttonXMax = -std::numeric_limits< float >::max();
    buttonYMin = std::numeric_limits< float >::max();
    buttonYMax = -std::numeric_limits< float >::max();

    UI2DDrawData* ui2DDrawData = uiRoot->mUI2DDrawData;
    UI2DState* state = ui2DDrawData->PushState();
    TAC_ON_DESTRUCT( ui2DDrawData->PopState() );

    for( const UITextTransition& transition : mTransitions )
    {
      const UITextData* uiTextInfo = transition.mUseUITextDataStore ? &transition.mUITextDataStore : GetUITextData();

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

      //DrawCall* drawCall = graphics->GetLastDrawCall();
      //if( drawCallPrev )
      //{
      //  drawCall->mScissorRectMaxUISpace.x = Max(
      //    drawCall->mScissorRectMaxUISpace.x,
      //    drawCallPrev->mScissorRectMaxUISpace.x );
      //}
      //if( !transition.mTransitionFinished )
      //{
      //  drawCall->mScissorRectMaxUISpace.x = Lerp(
      //    drawCall->mScissorRectMinUISpace.x,
      //    drawCall->mScissorRectMaxUISpace.x,
      //    transition.mTransitionTweenPercent );
      //}
      //if( drawCallPrev )
      //{
      //  drawCallPrev->mScissorRectMinUISpace.x = Min(
      //    drawCallPrev->mScissorRectMaxUISpace.x,
      //    drawCall->mScissorRectMaxUISpace.x );
      //}
      //drawCallPrev = drawCall;
      //maxHeightBaselines = Max( maxHeightBaselines, heightBetweenBaselines );
      //
      //
      //Assert( drawCall->mScissorRectMinUISpace.x <= drawCall->mScissorRectMaxUISpace.x );
      //Assert( drawCall->mScissorRectMinUISpace.y <= drawCall->mScissorRectMaxUISpace.y );
      //
      //buttonXMin = Min( buttonXMin, drawCall->mScissorRectMinUISpace.x );
      //buttonXMax = Max( buttonXMax, drawCall->mScissorRectMaxUISpace.x );
      //buttonYMin = Min( buttonYMin, drawCall->mScissorRectMinUISpace.y );
      //buttonYMax = Max( buttonYMax, drawCall->mScissorRectMaxUISpace.y );
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


  void UIAnchor::DebugImgui()
  {
    //int currentHItem = ( int )mAnchorHorizontal;
    //auto itemHGetter = []( void* data, int idx, const char** outText )
    //{
    //  UnusedParameter( data );
    //  *outText = AnchorHorizontalStrings[ idx ].c_str();
    //  return true;
    //};
    //if( ImGui::Combo( "Anchor Horizontal", &currentHItem, itemHGetter, nullptr, ( int )UIAnchorHorizontal::Count ) )
    //{
    //  mAnchorHorizontal = ( UIAnchorHorizontal )currentHItem;
    //}

    //int currentVItem = ( int )mAnchorVertical;
    //auto itemVGetter = []( void* data, int idx, const char** outText )
    //{
    //  UnusedParameter( data );
    //  *outText = AnchorVerticalStrings[ idx ].c_str();
    //  return true;
    //};
    //if( ImGui::Combo( "Anchor Vertical", &currentVItem, itemVGetter, nullptr, ( int )UIAnchorVertical::Count ) )
    //{
    //  mAnchorVertical = ( UIAnchorVertical )currentVItem;
    //}

  }

  UILayoutable::UILayoutable( StringView debugName )
  {
    mDebugName = debugName;
  }
  void UILayoutable::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( "Layout" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //ImGui::DragFloat( "ui width", &mUiWidth );
    //ImGui::DragFloat( "ui height", &mUiHeight );
    //ImGui::DragFloat2( "ui position", mPositionAnchored.data() );
    //ImGui::Checkbox( "Debug", &mDebug );
  }
  void UILayoutable::Render( Errors& errors )
  {
    if( mDebug )
    {
      UI2DDrawData* ui2DDrawData = mUIRoot->mUI2DDrawData;
      UI2DState* state = ui2DDrawData->PushState();
      state->Translate( mPositionAnchored );
      state->Draw2DBox( // Color?
                        mUiWidth,
                        mUiHeight );
      ui2DDrawData->PopState();
    }
  }
  bool UILayoutable::IsHovered()
  {
    bool isHovered =
      mUIRoot->mUiCursor.x > mPositionAnchored.x &&
      mUIRoot->mUiCursor.y < mPositionAnchored.y &&
      mUIRoot->mUiCursor.x < mPositionAnchored.x + mUiWidth &&
      mUIRoot->mUiCursor.y > mPositionAnchored.y - mUiHeight;
    return isHovered;
  }
  v2 UILayoutable::GetWindowspacePosition()
  {
    v2 result = mLocalPosition;
    if( mParent )
    {
      v2 parentWindowspacePosition = mParent->GetWindowspacePosition();
      result += parentWindowspacePosition;
    }
    return result;
  }

  UILayout::UILayout( StringView debugName ) : UILayoutable( debugName )
  {
    mColor = v4(
      RandomFloat0To1(),
      RandomFloat0To1(),
      RandomFloat0To1(),
      1
    );
  }
  UILayout::~UILayout()
  {
    for( auto textDrawInfo : mUILayoutables )
      delete textDrawInfo;
  }
  float UILayout::GetInitialDelaySeconds()
  {
    auto index = ( int )mUILayoutables.size();
    float result = GetInitialDelaySeconds( index );
    return result;
  }
  float UILayout::GetInitialDelaySeconds( int index )
  {
    float result =
      mTransitionPerTextInitialDelaySeconds +
      mTransitionBetweenTextsDurationSeconds * index;
    return result;
  }
  void UILayout::DebugImgui()
  {
    //UILayoutable::DebugImgui();
    //auto childrenCount = ( int )mUILayoutables.size();
    //if( childrenCount )
    //{
    //  if( ImGui::CollapsingHeader( va( "%i Children", childrenCount ) ) )
    //  {
    //    ImGui::Indent();
    //    OnDestruct( ImGui::Unindent() );
    //    int iTextDrawInfo = 0;
    //    for( UILayoutable* textDrawInfo : mUILayoutables )
    //    {
    //      iTextDrawInfo++;
    //      if( !ImGui::CollapsingHeader( textDrawInfo->mDebugName.c_str() ) )
    //        continue;

    //      ImGui::Indent();
    //      OnDestruct( ImGui::Unindent() );

    //      ImGui::PushID( textDrawInfo );
    //      OnDestruct( ImGui::PopID() );

    //      textDrawInfo->DebugImgui();
    //    }
    //  }
    //}
    //mAnchor.DebugImgui();
    ////ImGui::DragFloat2( "Position", mPosition.data() );
    //ImGui::DragFloat( "Width", &mUiWidth );
    //ImGui::DragFloat( "Height Cur", &mHeightCur );
    //ImGui::DragFloat( "Height Target", &mHeightTarget );
    //ImGui::ColorEdit4( "Color", mColor.data() );
    //ImGui::DragFloat( "mTransitionPerTextInitialDelaySeconds", &mTransitionPerTextInitialDelaySeconds );
    //ImGui::DragFloat( "mTransitionBetweenTextsDurationSeconds", &mTransitionBetweenTextsDurationSeconds );
    //if( ImGui::Button( "Delete menu" ) )
    //  RequestDeletion();
    //if( Texture* texture = mTexture )
    //{
    //  float w = 0.9f * ImGui::GetContentRegionAvailWidth();
    //  float h = w / texture->GetAspect();
    //  ImGui::Image( texture->GetImguiTextureID(), ImVec2( w, h ) );
    //}
  }
  void UILayout::Update( UILayoutData* uiLayoutData )
  {
    if( !mUILayoutables.empty() )
    {
      float runningAutoWidth = 0;
      for( UILayoutable* uiLayoutable : mUILayoutables )
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
      //mUiWidth = mParent ?
      //  mParent->mUiWidth :
      //  mUIRoot->mUI2DDrawData->mRenderView->mFramebuffer->myImage.mWidth;
      TAC_INVALID_CODE_PATH; // uhh
    }

    //mPositionAnchored = mPosition;
    mHeightCur = mHeightTarget;
    mUiHeight = mHeightCur;

#if 0
    v2 paddingVec = v2( 1, 1 ) * mMenuPadding;
    uiLayoutData->mUIMin += paddingVec;
    uiLayoutData->mUIMax -= paddingVec;
    v2 contentDimensions = {};

    UILayoutData childLayout = {};

    for( UILayoutable* uiLayoutable : mUILayoutables )
    {
      uiLayoutable->Update( uiLayoutData );

      switch( mUILayoutType )
      {
        case UILayoutType::Vertical:
          contentDimensions.x = Max( contentDimensions.x, uiLayoutable->mUiWidth );
          contentDimensions.y += uiLayoutable->mUiHeight;
          uiLayoutData->mUIMax.y += uiLayoutable->mUiHeight;
          break;
        case UILayoutType::Horizontal:
          contentDimensions.y = Max( contentDimensions.y, uiLayoutable->mUiHeight );
          contentDimensions.x += uiLayoutable->mUiWidth;
          uiLayoutData->mUIMax.x += uiLayoutable->mUiWidth;
          break;
          TAC_INVALID_DEFAULT_CASE( mUILayoutType );
      }
    }





    v2 anchoredPenPos = mPosition;
    switch( mAnchor.mAnchorHorizontal )
    {
      case UIAnchorHorizontal::Left: anchoredPenPos.x += uiLayoutData->mUIMin.x; break;
      case UIAnchorHorizontal::Right: anchoredPenPos.x += uiLayoutData->mUIMax.x - mUiWidth; break;
      case UIAnchorHorizontal::Center: anchoredPenPos.x +=
        ( uiLayoutData->mUIMin.x + uiLayoutData->mUIMax.x - mUiWidth ) * 0.5f; break;
    }


    switch( mAnchor.mAnchorVertical )
    {
      case UIAnchorVertical::Top: anchoredPenPos.y += uiLayoutData->mUIMin.y; break;
      case UIAnchorVertical::Bottom: anchoredPenPos.y += uiLayoutData->mUIMax.y - mUiHeight; break;
      case UIAnchorVertical::Center: anchoredPenPos.y +=
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
      float t = EaseInOutQuart( percent );
      mHeightCur = Lerp( mHeightPrev, mHeightTarget, t );
    }

    if( !mUILayoutables.empty() )
    {
      uiLayoutData->mUIMin = mPositionAnchored + paddingVec;
      if( !mAutoWidth )
        uiLayoutData->mUIMax = mPositionAnchored - paddingVec + v2( mUiWidth, mUiHeight );

      float contentWidth = 0;
      float contentHeight = 0;

      for( UILayoutable* uiLayoutable : mUILayoutables )
      {
        uiLayoutable->Update( uiLayoutData );

        switch( mUILayoutType )
        {
          case UILayoutType::Vertical:
            contentHeight += uiLayoutable->mUiHeight;
            uiLayoutData->mUIMin.y -= uiLayoutable->mUiHeight;
            break;
          case UILayoutType::Horizontal:
            contentWidth += uiLayoutable->mUiWidth;
            uiLayoutData->mUIMax.x += uiLayoutable->mUiWidth;
            break;
            TAC_INVALID_DEFAULT_CASE( mUILayoutType );
        }
      }
      if( mAutoWidth )
        mUiWidth = Max( mUiWidth, contentWidth );
    }

    mUiHeight = mHeightCur;

    if( mExpandWidth )
      mUiWidth = uiLayoutData->mUIMax.x - uiLayoutData->mUIMin.x;
#endif
  }
  void UILayout::RequestDeletion()
  {
    mHeightTarget = 0;
    mTransitionStartSeconds = mUIRoot->GetElapsedSeconds();
    mTransitionedFinished = false;
    mRequestDeletion = true;
    for( UILayoutable* uiText : mUILayoutables )
      uiText->TransitionOut();
    mTransitionDurationSeconds = 2.3f;
  }
  void UILayout::Render( Errors& errors )
  {
    UILayoutable::Render( errors );
    TAC_HANDLE_ERROR( errors );
    UI2DDrawData* ui2DDrawData = mUIRoot->mUI2DDrawData;
    UI2DState* state = ui2DDrawData->PushState();

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

    for( UILayoutable* uiLayoutable : mUILayoutables )
    {
      uiLayoutable->Render( errors );
      TAC_HANDLE_ERROR( errors );
    }
    ui2DDrawData->PopState();
  }
  v2 UILayout::GetWindowspacePosition()
  {
    v2 windowSpacePos = mLocalPosition;
    if( mParent )
    {
      v2 parentWindowSpacePos = mParent->GetWindowspacePosition();
      switch( mAnchor.mAnchorHorizontal )
      {
        case UIAnchorHorizontal::Left:
          windowSpacePos.x += parentWindowSpacePos.x; break;
        case UIAnchorHorizontal::Right:
          windowSpacePos.x += parentWindowSpacePos.x + mParent->mUiWidth - mUiWidth; break;
        case UIAnchorHorizontal::Center:
          windowSpacePos.x += parentWindowSpacePos.x + ( mParent->mUiWidth + mUiWidth ) / 2; break;
          TAC_INVALID_DEFAULT_CASE( mAnchor.mAnchorHorizontal );
      }
    }
    return windowSpacePos;
  }

  UIRoot::UIRoot()
  {
    transitionDurationSeconds = 0.3f;

    mHierarchyRoot = TAC_NEW UIHierarchyNode;
    mHierarchyRoot->mUIRoot = this;

  }
  UIRoot::~UIRoot()
  {
    for( auto menu : mUIMenus )
      delete menu;
    delete mHierarchyRoot;
  }
  void UIRoot::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( "UI Root" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //ImGui::DragFloat( "Transition Duration Sec", &transitionDurationSeconds );
    //ImGui::Checkbox( "Debug button areas", &mShouldDebugDrawButtonAreas );
    //int iUIMenu = 0;
    //bool deleteMenus = ImGui::Button( "Delete all ui" );
    //for( UILayout* uiMenu : mUIMenus )
    //{
    //  if( deleteMenus )
    //    uiMenu->RequestDeletion();
    //  iUIMenu++;
    //  if( !ImGui::CollapsingHeader( uiMenu->mDebugName.c_str() ) )
    //    continue;

    //  ImGui::Indent();
    //  OnDestruct( ImGui::Unindent() );

    //  ImGui::PushID( uiMenu );
    //  OnDestruct( ImGui::PopID() );

    //  uiMenu->DebugImgui();
    //}
  }
  UILayout* UIRoot::AddMenu( StringView debugName )
  {
    auto* uiMenu = TAC_NEW UILayout( debugName );
    uiMenu->mUIRoot = this;
    uiMenu->mTransitionStartSeconds = GetElapsedSeconds();
    mUIMenus.push_back( uiMenu );
    return uiMenu;
  }
  void UIRoot::Render( Errors& errors )
  {
    //UI2DState* state = mUI2DDrawData->PushState();
    //state->mTransform = M3Scale(
    //  framebuffer->myImage.mWidth / mUIWidth,
    //  framebuffer->myImage.mHeight / mUIHeight,
    //  1 );
    //mUI2DDrawData->PopState();
    mHierarchyRoot->RenderHierarchy( errors );
    TAC_HANDLE_ERROR( errors );

    for( UILayout* uiMenu : mUIMenus )
    {
      uiMenu->Render( errors );
      TAC_HANDLE_ERROR( errors );
    }
  }
  void UIRoot::Update()
  {
    //Texture* framebuffer = mUI2DDrawData->mRenderView->mFramebuffer;
    //float uiWidth = 1024;
    //float uiHeight = 1024;
    //if( aspect > 1 )
    //  uiWidth *= aspect;
    //else
    //  uiHeight /= aspect;
    //mUIWidth = uiWidth;
    //mUIHeight = uiHeight;

    //v2 cursorPos;
    //Errors cursorPosErrors;
    //OS::GetScreenspaceCursorPos( cursorPos, cursorPosErrors );
    //if( cursorPosErrors.empty() )
    //  mUiCursor = cursorPos - v2( ( float )mDesktopWindow->mX, ( float )mDesktopWindow->mY );

    //Vector< UILayout* > toDelete;
    //for( UILayout* uiMenu : mUIMenus )
    //{
    //  UILayoutData uiLayoutData;
    //  uiLayoutData.mUIMin = {};
    //  //uiLayoutData.mUIMax = { uiWidth, uiHeight };
    //  uiLayoutData.mUIMax = {
    //    ( float )framebuffer->myImage.mWidth,
    //    ( float )framebuffer->myImage.mHeight };
    //  uiMenu->Update( &uiLayoutData );
    //  if( uiMenu->mRequestDeletion && uiMenu->mTransitionedFinished )
    //    toDelete.push_back( uiMenu );
    //}
    //for( UILayout* uiMenu : toDelete )
    //{
    //  mUIMenus.erase( std::find( mUIMenus.begin(), mUIMenus.end(), uiMenu ) );
    //  delete uiMenu;
    //}

    //Image& image = framebuffer->myImage;

    //mHierarchyRoot->mSize = {
    //  ( float )image.mWidth,
    //  ( float )image.mHeight };
  }
  String UIHierarchyNode::DebugGenerateGraphVizDotFile()
  {

    // https://en.wikipedia.org/wiki/DOT_(graph_description_language)
    // 
    // digraph graphname    a
    // {                    |
    //   a->b->c;           b
    //   b->d;             / \
    // }                  c   d


    Vector<String> lines;
    lines.push_back( "digraph graphname" );
    lines.push_back( "{" );

    std::set< UIHierarchyNode* > allnodes;
    std::map< UIHierarchyNode*, String > nodeGraphNames;
    Vector< std::pair< UIHierarchyNode*, UIHierarchyNode* >> nodeConnections;

    Vector< UIHierarchyNode* > nodes = { this };
    while( !nodes.empty() )
    {
      UIHierarchyNode* node = nodes.back();
      nodes.pop_back();
      allnodes.insert( node );

      for( UIHierarchyNode* child : node->mChildren )
      {
        nodes.push_back( child );
        nodeConnections.push_back( { node, child } );
      }
    }

    for( UIHierarchyNode* node : allnodes )
    {
      String nodeGraphName = "node" + ToString( ( int )nodeGraphNames.size() );
      String nodeGraphLabel = ToString( node );

      if( !node->mDebugName.empty() )
        nodeGraphLabel = node->mDebugName;
      else if( node == this )
        nodeGraphLabel = "hierarchy root";
      else if( node->mVisual && !node->mVisual->GetDebugName().empty() )
        nodeGraphLabel = node->mVisual->GetDebugName();

      nodeGraphNames[ node ] = nodeGraphName;

      lines.push_back( nodeGraphName + "[ label = \"" + nodeGraphLabel + "\" ];" );
    }

    for( auto connection : nodeConnections )
    {
      lines.push_back(
        nodeGraphNames[ connection.first ] + "->" + nodeGraphNames[ connection.second ] );
    }

    lines.push_back( "}" );

    String stringified = Join( lines, "\n" );
    return stringified;

  }

  UIHierarchyNode::UIHierarchyNode()
  {
    static int iColor;
    auto colors = MakeArray< int >(
      0x53c8e9,
      0x3c3c3b,
      0x074b7f,
      0xc82c60,
      0xf9fff9 );
    int color = colors[ iColor++ % colors.size() ];
    mColor.xyz() = HexToRGB( color );
    mColor.w = 1;
    mDrawOutline = true;
  }
  UIHierarchyNode* UIHierarchyNode::Split(
    UISplit uiSplit,
    UILayoutType layoutType )
  {
    // TODO: I'm sure this whole if else chain can be simplified into something elegant
    UIHierarchyNode* parent = nullptr;
    if( !mChildren.empty() )
    {
      if( layoutType == mLayoutType )
      {
        parent = this;
      }
      else if( !mParent )
      {
        // get my parent who is vertically split
        parent = TAC_NEW UIHierarchyNode;
        parent->mChildren = { this };
        parent->mUIRoot = mUIRoot;
        parent->mLayoutType = layoutType;
        mUIRoot->mHierarchyRoot = parent;
        mParent = parent;
      }
    }
    else if( !mParent )
    {
      parent = TAC_NEW UIHierarchyNode;
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
          parent = TAC_NEW UIHierarchyNode;
          parent->mChildren = { this };
          parent->mUIRoot = mUIRoot;
          parent->mLayoutType = layoutType;
          parent->mParent = mParent;
          int n = mParent->mChildren.size();
          bool exchanged = false;
          for( int i = 0; i < n; ++i )
          {
            UIHierarchyNode* child = mParent->mChildren[ i ];
            if( child != this )
              continue;
            mParent->mChildren[ i ] = parent;
            mParent = parent;
            exchanged = true;
            break;
          }
          TAC_ASSERT( exchanged );
        }
      }
    }

    TAC_ASSERT( parent );

    auto newChild = TAC_NEW UIHierarchyNode;
    newChild->mUIRoot = mUIRoot;
    newChild->mParent = parent;

    switch( uiSplit )
    {
      case UISplit::Before:
      {
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
      case UISplit::After:
      {
        parent->mChildren.push_back( newChild );
      } break;
      TAC_INVALID_DEFAULT_CASE( uiSplit );
    }

    return newChild;
  }

  UIHierarchyNode* UIHierarchyNode::AddChild()
  {
    auto newChild = TAC_NEW UIHierarchyNode;
    newChild->mUIRoot = mUIRoot;
    newChild->mParent = this;
    mChildren.push_back( newChild );
    return newChild;
  }

  void UIHierarchyNode::Expand()
  {
    for( int iChild = 0; iChild < mParent->mChildren.size(); ++iChild )
      if( this == mParent->mChildren[ iChild ] )
        mParent->mExpandingChildIndex = iChild;
  }

  void UIHierarchyNode::RenderHierarchy( Errors& errors )
  {
    // compute children sizes
    if( !mChildren.empty() )
    {
      float expandedChildSize = mSize[ ( int )mLayoutType ];
      UIHierarchyNode* expandedChild = mChildren[ mExpandingChildIndex ];
      for( UIHierarchyNode* child : mChildren )
      {
        child->mSize[ 1 - ( int )mLayoutType ] = mSize[ 1 - ( int )mLayoutType ];
        if( child == expandedChild )
          continue;
        if( child->mVisual )
          child->mSize[ ( int )mLayoutType ] = child->mVisual->mDims[ ( int )mLayoutType ];
        expandedChildSize -= child->mSize[ ( int )mLayoutType ];
      }
      expandedChild->mSize[ ( int )mLayoutType ] = expandedChildSize;
    }

    UI2DState* state = mUIRoot->mUI2DDrawData->PushState();
    float padding = 0;
    state->Translate( mPositionRelativeToParent + v2( 1, 1 ) * padding );
    if( mDrawOutline )
    {
      v4 color = mColor;
      if( mUIRoot->mDesktopWindow->mCursorUnobscured &&
          mOnClickEventEmitter.size() &&
          mUIRoot->mUiCursor.x > mPositionRelativeToRoot.x &&
          mUIRoot->mUiCursor.x < mPositionRelativeToRoot.x + mSize.x &&
          mUIRoot->mUiCursor.y > mPositionRelativeToRoot.y  &&
          mUIRoot->mUiCursor.y < mPositionRelativeToRoot.y + mSize.y )
      {
        // magic
        color.xyz() = ( color.xyz().Length() < 0.5f ? color.xyz() + v3( 1, 1, 1 ) : color.xyz() ) / 2;
        if( KeyboardInput::Instance->IsKeyJustDown( Key::MouseLeft ) )
          mOnClickEventEmitter.EmitEvent();
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
      if( !mVisual && debugdrawNodeName )
      {
        v4 debugTextColor = mColor.xyz().Length() < 0.5f ? v4( 1, 1, 1, 1 ) : v4( 0, 0, 0, 1 );
        debugTextColor = { 0, 0, 0, 1 };
        state->Draw2DText( Language::English, 16, mDebugName, nullptr, debugTextColor, errors );
        TAC_HANDLE_ERROR( errors );
      }
    }
    if( mVisual )
    {
      mVisual->Render( errors );
      TAC_HANDLE_ERROR( errors );
    }
    float runningPixelX = 0;
    for( UIHierarchyNode* child : mChildren )
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

  void UIHierarchyNode::SetVisual( UIHierarchyVisual* visual )
  {
    mVisual = visual;
    if( visual )
      visual->mHierarchyNode = this;
  }


  void UIHierarchyVisualText::Render( Errors& errors )
  {
    if( mUITextData.mUtf8.empty() )
      return;
    float why_the_fuck_do_i_care_about_this;
    UIRoot* uiRoot = mHierarchyNode->mUIRoot;
    uiRoot->mUI2DDrawData->mStates.back().Draw2DText(
      uiRoot->mDefaultLanguage,
      mUITextData.mFontSize,
      mUITextData.mUtf8,
      &why_the_fuck_do_i_care_about_this,
      mUITextData.mColor,
      errors );
    TAC_HANDLE_ERROR( errors );
  }

  void UIHierarchyVisualImage::Render( Errors& errors )
  {
    UIRoot* uiRoot = mHierarchyNode->mUIRoot;
    uiRoot->mUI2DDrawData->mStates.back().Draw2DBox(
      mDims[ 0 ],
      mDims[ 1 ],
      v4( 1, 1, 1, 1 ),
      mTexture );
  }

  String UIHierarchyVisualImage::GetDebugName()
  {
    return "UIHierarchyVisualImage";
    //return mTexture->mName;
  }

}
