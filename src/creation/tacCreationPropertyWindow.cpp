#include "creation/tacCreationPropertyWindow.h"
#include "creation/tacCreation.h"
#include "common/tacErrorHandling.h"
#include "common/tacUI.h"
#include "common/tacUI2D.h"
#include "common/tacDesktopWindow.h"
#include "common/tacShell.h"
#include "space/tacentity.h"

TacCreationPropertyWindow::~TacCreationPropertyWindow()
{
}

void TacCreationPropertyWindow::Init( TacErrors& errors )
{
  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData->mUI2DCommonData = mShell->mUI2DCommonData;
  mUIRoot = new TacUIRoot;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mDesktopWindow = mDesktopWindow;
  mUIRoot->mHierarchyRoot->mLayoutType = TacUILayoutType::Horizontal;

  TacUIHierarchyVisualText* text;


  mHierarchy = mUIRoot->mHierarchyRoot->AddChild();
  mHierarchy->mLayoutType = TacUILayoutType::Vertical;
  mHierarchy->mDebugName = "Hierarchy";

  text = new TacUIHierarchyVisualText;
  text->mUITextData.mUtf8 = "Hierarchy";
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  TacUIHierarchyNode* node = mHierarchy->AddChild();
  node->SetVisual( text );

  text = new TacUIHierarchyVisualText;
  text->mUITextData.mUtf8 = "Inspector";
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  mInspector = mUIRoot->mHierarchyRoot->AddChild();
  mInspector->mLayoutType = TacUILayoutType::Vertical;
  mInspector->mDebugName = "Inspector";
  mInspector->SetVisual( text );
}

void TacCreationPropertyWindow::Update( TacErrors& errors )
{
  static bool added;
  if( mCreation->mSelectedEntity )
  {
    if( !added )
    {
      TacUIHierarchyVisualText* text;
      TacUIHierarchyNode* name;

      text = new TacUIHierarchyVisualText;
      text->mUITextData.mUtf8 = mCreation->mSelectedEntity->mName;
      text->mUITextData.mColor = textColor;
      text->mDims = { 100, 50 };
      name = mHierarchy->AddChild();
      name->SetVisual( text );
      name->mDebugName = "Entity Name";

      added = true;
    }
  }

  mDesktopWindow->SetRenderViewDefaults();
  mUIRoot->Update();
  mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
