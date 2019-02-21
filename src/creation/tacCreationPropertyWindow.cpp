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

  //TacUIHierarchyVisualText* text;
  //TacUIHierarchyNode* node;

  //node = mUIRoot->mHierarchyRoot->AddChild();
  //node->mLayoutType = TacUILayoutType::Vertical;
  //node->mDebugName = "Hierarchy Pane";
  //mHierarchyPane = node;

  //text = new TacUIHierarchyVisualText;
  //text->mUITextData.mUtf8 = "Hierarchy";
  //text->mUITextData.mColor = textColor;
  //text->mDims = { 100, 50 };
  //node = mHierarchyPane->AddChild();
  //node->mDebugName = "Hierarchy Label";
  //node->SetVisual( text );

  //node = mHierarchyPane->AddChild();
  //node->mDebugName = "Hierarchy List";
  //node->Expand();
  //mHierarchyList = node;




  //text = new TacUIHierarchyVisualText;
  //text->mUITextData.mUtf8 = "Inspector";
  //text->mUITextData.mColor = textColor;
  //text->mDims = { 100, 50 };
  //mInspector = mUIRoot->mHierarchyRoot->AddChild();
  //mInspector->mLayoutType = TacUILayoutType::Vertical;
  //mInspector->mDebugName = "Inspector";
  //mInspector->SetVisual( text );
}

void TacCreationPropertyWindow::Update( TacErrors& errors )
{
  mUIRoot->mImGuiWindow->Begin();
  mUIRoot->mImGuiWindow->Text( "This is some useful text" );

  //static bool added;
  //if( mCreation->mSelectedEntity )
  //{
  //  if( !added )
  //  {
  //    TacUIHierarchyVisualText* text;
  //    TacUIHierarchyNode* name;

  //    text = new TacUIHierarchyVisualText;
  //    text->mUITextData.mUtf8 = mCreation->mSelectedEntity->mName;
  //    text->mUITextData.mColor = textColor;
  //    text->mDims = { 100, 50 };
  //    name = mHierarchyList->AddChild();
  //    name->SetVisual( text );
  //    name->mDebugName = "Entity Name";

  //    added = true;
  //  }
  //}

  mDesktopWindow->SetRenderViewDefaults();
  mUIRoot->Update();
  mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
