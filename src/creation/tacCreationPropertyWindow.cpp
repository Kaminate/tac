#include "creation/tacCreationPropertyWindow.h"
#include "creation/tacCreation.h"
#include "common/tacErrorHandling.h"
#include "common/tacUI.h"
#include "common/tacImGui.h"
#include "common/tacUI2D.h"
#include "common/tacDesktopWindow.h"
#include "common/tacOS.h"
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

void TacCreationPropertyWindow::SetImGuiGlobals()
{
  TacErrors screenspaceCursorPosErrors;
  v2 screenspaceCursorPos;
  TacOS::Instance->GetScreenspaceCursorPos( screenspaceCursorPos, screenspaceCursorPosErrors );
  if( screenspaceCursorPosErrors.empty() )
  {
    gTacImGuiGlobals.mMousePositionDesktopWindowspace = {
      screenspaceCursorPos.x - mDesktopWindow->mX,
      screenspaceCursorPos.y - mDesktopWindow->mY };
    gTacImGuiGlobals.mIsWindowDirectlyCursor = mDesktopWindow->mCursorUnobscured;
  }
  else
  {
    gTacImGuiGlobals.mIsWindowDirectlyCursor = false;
  }
  gTacImGuiGlobals.mUI2DDrawData = mUI2DDrawData;
  gTacImGuiGlobals.mKeyboardInput = mShell->mKeyboardInput;
  gTacImGuiGlobals.mElapsedSeconds = mShell->mElapsedSeconds;
}

void TacCreationPropertyWindow::Update( TacErrors& errors )
{
  static bool areYouHappy;

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

  SetImGuiGlobals();


  TacImGuiBegin( "Properties", {} );
  TacImGuiBeginGroup();
  TacImGuiBeginChild( "Hierarchy", v2( 250, -100 ) );
  TacWorld* world = mCreation->mWorld;
  for( TacEntity* entity : world->mEntities )
  {
    bool isSelected = mCreation->mSelectedEntity == entity;
    if( TacImGuiSelectable( entity->mName, isSelected ) )
    {
      mCreation->mSelectedEntity = entity;
    }
  }
  TacImGuiEndChild();
  if( TacImGuiButton( "Create Entity" ) )
    mCreation->CreateEntity();
  TacImGuiEndGroup();
  TacImGuiSameLine();
  TacImGuiBeginGroup();

  if( TacEntity* entity = mCreation->mSelectedEntity )
  {
    TacImGuiInputText( "Name", entity->mName );
    TacImGuiText( entity->mName );
    TacImGuiText( "UUID: " + TacToString( ( TacUUID )entity->mEntityUUID ) );
    TacImGuiText( "Position: " +
      TacToString( entity->mPosition.x ) + " " +
      TacToString( entity->mPosition.y ) + " " +
      TacToString( entity->mPosition.z ) );
    for( TacComponent* component : entity->mComponents )
    {
      TacComponentType componentType = component->GetComponentType();
      TacString componentName = TacToString( componentType );
      TacImGuiText( componentName + " component" );
    }
    TacImGuiButton( "Add component" );
  }
  TacImGuiButton( "C" );
  TacImGuiCheckbox( "Happy", &areYouHappy );
  TacImGuiEndGroup();
  TacImGuiEnd();

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
