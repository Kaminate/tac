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
  mUIRoot->mKeyboardInput = mShell->mKeyboardInput;
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
  //mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  static bool areYouHappy;

  mUIRoot->mImGuiWindow->mSize =
  {
    ( float )mDesktopWindow->mWidth,
    ( float )mDesktopWindow->mHeight
  };
  mUIRoot->mImGuiWindow->BeginFrame();

  {
    mUIRoot->mImGuiWindow->BeginGroup();
    {
      static int entityCount;
      TacImGuiWindow* hierarchy = mUIRoot->mImGuiWindow->BeginChild( "Hierarchy", v2( 250, -100 ) );
      for( int iEntity = 0; iEntity < entityCount; ++iEntity )
      {
        TacString text = "Entity " + TacToString( iEntity + 1 );
        hierarchy->Text( text );
      }
      if( hierarchy->Button( "Add Entity" ) )
      {
        entityCount++;
      }
      if( hierarchy->Button( "Add 30 Entity" ) )
      {
        entityCount += 30;
      }
      hierarchy->EndChild();
    }


    //mUIRoot->mImGuiWindow->Text( "Hello" );
    //mUIRoot->mImGuiWindow->Text( "Obj 2" );
    //mUIRoot->mImGuiWindow->Text( "Obj 3" );
    mUIRoot->mImGuiWindow->Button( "A" );
    //mUIRoot->mImGuiWindow->SameLine();
    //mUIRoot->mImGuiWindow->Button( "B" );
    mUIRoot->mImGuiWindow->EndGroup();
  }

  mUIRoot->mImGuiWindow->SameLine();

  {
    mUIRoot->mImGuiWindow->BeginGroup();
    mUIRoot->mImGuiWindow->Button( "C" );

    //mUIRoot->mImGuiWindow->Text( "This is some useful text" );
    //mUIRoot->mImGuiWindow->Text( "This is some more useful text" );
    mUIRoot->mImGuiWindow->Checkbox( "Happy", &areYouHappy );
    //if( mUIRoot->mImGuiWindow->Button( "If you're happy and you know it" ) )
    //  std::cout << "Clap your hands" << std::endl;
    mUIRoot->mImGuiWindow->EndGroup();
  }


  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
