#include "creation/tacCreationPropertyWindow.h"
#include "creation/tacCreation.h"
#include "common/tacErrorHandling.h"
#include "common/tacUI.h"
#include "common/tacImGui.h"
#include "common/tacUI2D.h"
#include "common/tacDesktopWindow.h"
#include "common/tacShell.h"
#include "space/tacentity.h"
#include "space/tacworld.h"
#include "space/taccomponent.h"

TacCreationPropertyWindow::~TacCreationPropertyWindow()
{
}

void TacCreationPropertyWindow::Init( TacErrors& errors )
{
  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData->mUI2DCommonData = mShell->mUI2DCommonData;
  mUIRoot = new TacUIRoot;
  mUIRoot->mElapsedSeconds = &mShell->mElapsedSeconds;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mKeyboardInput = mShell->mKeyboardInput;
  mUIRoot->mDesktopWindow = mDesktopWindow;
  mUIRoot->mHierarchyRoot->mLayoutType = TacUILayoutType::Horizontal;

}

void TacCreationPropertyWindow::Update( TacErrors& errors )
{

  mDesktopWindow->SetRenderViewDefaults();
  mUIRoot->Update();
  //mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  static bool once;
  if( !once )
  {
    once = true;
    mCreation->CreateEntity();
  }

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
      TacImGuiWindow* hierarchy = mUIRoot->mImGuiWindow->BeginChild( "Hierarchy", v2( 250, -100 ) );

      TacWorld* world = mCreation->mWorld;
      for( TacEntity* entity : world->mEntities )
      {
        bool isSelected = mCreation->mSelectedEntity == entity;
        if( hierarchy->Selectable( entity->mName, isSelected ) )
        {
          mCreation->mSelectedEntity = entity;
        }
      }
      hierarchy->EndChild();
    }
    if( mUIRoot->mImGuiWindow->Button( "Create Entity" ) )
      mCreation->CreateEntity();
    mUIRoot->mImGuiWindow->EndGroup();
  }

  mUIRoot->mImGuiWindow->SameLine();

  {
    mUIRoot->mImGuiWindow->BeginGroup();

    if( TacEntity* entity = mCreation->mSelectedEntity )
    {
      mUIRoot->mImGuiWindow->InputText( "Name", entity->mName );
      mUIRoot->mImGuiWindow->Text( entity->mName );
      mUIRoot->mImGuiWindow->Text( "UUID: " + TacToString( ( TacUUID )entity->mEntityUUID ) );
      mUIRoot->mImGuiWindow->Text( "Position: " +
        TacToString( entity->mPosition.x ) + " " +
        TacToString( entity->mPosition.y ) + " " +
        TacToString( entity->mPosition.z ) );
      for( TacComponent* component : entity->mComponents )
      {
        TacComponentType componentType = component->GetComponentType();
        TacString componentName = TacToString( componentType );
        mUIRoot->mImGuiWindow->Text( componentName + " component" );
      }
      mUIRoot->mImGuiWindow->Button( "Add component" );
    }
    mUIRoot->mImGuiWindow->Button( "C" );
    mUIRoot->mImGuiWindow->Checkbox( "Happy", &areYouHappy );
    mUIRoot->mImGuiWindow->EndGroup();
  }


  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
