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
#include "space/tacsystem.h"
#include "space/tacspacetypes.h"

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
    gTacImGuiGlobals.mIsWindowDirectlyUnderCursor = mDesktopWindow->mCursorUnobscured;
  }
  else
  {
    gTacImGuiGlobals.mIsWindowDirectlyUnderCursor = false;
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
    static TacString occupation = "Bartender";
    TacImGuiInputText( "Name", entity->mName );
    TacImGuiInputText( "Occupation", occupation );
    TacImGuiText( entity->mName );
    TacImGuiText( "UUID: " + TacToString( ( TacUUID )entity->mEntityUUID ) );
    TacImGuiText( "Position: " +
      TacToString( entity->mPosition.x ) + " " +
      TacToString( entity->mPosition.y ) + " " +
      TacToString( entity->mPosition.z ) );
    TacImGuiDragFloat( "X Position: ", &entity->mPosition.x );
    for( TacComponent* component : entity->mComponents )
    {
      TacComponentType componentType = component->GetComponentType();
      TacString componentName = TacToString( componentType );
      if( TacImGuiCollapsingHeader( componentName ) )
      {
        TacImGuiIndent();
        OnDestruct( TacImGuiUnindent() );
        if( TacImGuiButton( "Remove component" ) )
        {
          entity->RemoveComponent( componentType );
          break;
        }
        component->TacDebugImgui();
      }
    }

    for( int i = 0; i < ( int )TacComponentType::Count; ++i )
    {
      auto componentType = ( TacComponentType )i;
      if( !entity->HasComponent( componentType ) &&
        TacImGuiButton( va( "Add %s component", TacToString( componentType ) ) ) )
        entity->AddNewComponent( componentType );
    }
  }
  TacImGuiButton( "C" );
  TacImGuiCheckbox( "Happy", &areYouHappy );
  TacImGuiEndGroup();
  TacImGuiEnd();

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
