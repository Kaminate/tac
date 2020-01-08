#include "creation/tacCreationPropertyWindow.h"
#include "creation/tacCreation.h"
#include "common/tacErrorHandling.h"
#include "common/tacAlgorithm.h"
#include "common/graphics/tacUI.h"
#include "common/graphics/tacImGui.h"
#include "common/graphics/tacUI2D.h"
#include "common/tacDesktopWindow.h"
#include "common/tacOS.h"
#include "common/tacShell.h"
#include "common/tacUtility.h"
#include "space/tacentity.h"
#include "space/tacworld.h"
#include "space/taccomponent.h"
#include "space/tacsystem.h"
#include "space/tacspacetypes.h"
#include "space/model/tacmodel.h"

TacCreationPropertyWindow::~TacCreationPropertyWindow()
{
  delete mUI2DDrawData;
}

void TacCreationPropertyWindow::Init( TacErrors& errors )
{
  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUIRoot = new TacUIRoot;
  mUIRoot->mElapsedSeconds = &mShell->mElapsedSeconds;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mDesktopWindow = mDesktopWindow;
  mUIRoot->mHierarchyRoot->mLayoutType = TacUILayoutType::Horizontal;
}

void TacCreationPropertyWindow::RecursiveEntityHierarchyElement( TacEntity* entity )
{
  bool previouslySelected = TacContains( mCreation->mSelectedEntities, entity );
  if( TacImGuiSelectable( entity->mName, previouslySelected ) )
  {
    mCreation->ClearSelection();
    mCreation->mSelectedEntities = { entity };
  }
  if( entity->mChildren.empty() )
    return;
  TAC_IMGUI_INDENT_BLOCK;
  for( TacEntity* child : entity->mChildren )
  {
    RecursiveEntityHierarchyElement( child );
  }
}
void TacCreationPropertyWindow::Update( TacErrors& errors )
{
  mDesktopWindow->SetRenderViewDefaults();
  mUIRoot->Update();
  //mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  //SetImGuiGlobals();
  TacImGuiSetGlobals( mShell, mDesktopWindow, mUI2DDrawData );


  TacImGuiBegin( "Properties", {} );



  TacImGuiBeginGroup();
  TacImGuiBeginChild( "Hierarchy", v2( 250, -100 ) );
  TacWorld* world = mCreation->mWorld;
  for( TacEntity* entity : world->mEntities )
  {
    if( !entity->mParent )
      RecursiveEntityHierarchyElement( entity );
  }
  TacImGuiEndChild();
  if( TacImGuiButton( "Create Entity" ) )
    mCreation->CreateEntity();
  if( TacImGuiButton( "Open Prefab" ) )
  {
    TacString prefabPath;
    TacOS::Instance->OpenDialog( prefabPath, errors );
    TAC_HANDLE_ERROR( errors );

    mCreation->LoadPrefabAtPath( prefabPath, errors );
    TAC_HANDLE_ERROR( errors );
  }
  TacImGuiEndGroup();
  TacImGuiSameLine();
  TacImGuiBeginGroup();

  for( TacEntity* entity : mCreation->mSelectedEntities )
  {
    static TacString occupation = "Bartender";
    TacImGuiInputText( "Name", entity->mName );
    TacImGuiText( "UUID: " + TacToString( ( TacUUID )entity->mEntityUUID ) );
    TacImGuiDragFloat( "X Position: ", &entity->mRelativeSpace.mPosition.x );
    TacImGuiDragFloat( "Y Position: ", &entity->mRelativeSpace.mPosition.y );
    TacImGuiDragFloat( "Z Position: ", &entity->mRelativeSpace.mPosition.z );
    TacImGuiDragFloat( "X Scale: ", &entity->mRelativeSpace.mScale.x );
    TacImGuiDragFloat( "Y Scale: ", &entity->mRelativeSpace.mScale.y );
    TacImGuiDragFloat( "Z Scale: ", &entity->mRelativeSpace.mScale.z );
    v3 rotDeg = entity->mRelativeSpace.mEulerRads * ( 180.0f / 3.14f );
    bool changed = false;
    changed |= TacImGuiDragFloat( "X Eul Deg: ", &rotDeg.x );
    changed |= TacImGuiDragFloat( "Y Eul Deg: ", &rotDeg.y );
    changed |= TacImGuiDragFloat( "Z Eul Deg: ", &rotDeg.z );
    if( changed )
      entity->mRelativeSpace.mEulerRads = rotDeg * ( 3.14f / 180.0f );
    TacVector< TacComponentRegistryEntry* > addableComponentTypes;

    if( entity->mParent )
    {
      TacImGuiText( "Parent: " + entity->mParent->mName );
      TacImGuiSameLine();
      if( TacImGuiButton( "Unparent" ) )
        entity->Unparent();
    }
    if( entity->mChildren.size() && TacImGuiCollapsingHeader( "Children" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      TacVector< TacEntity* > childrenCopy = entity->mChildren; // For iterator invalidation
      for( TacEntity* child : childrenCopy )
      {
        TacImGuiText( child->mName );
        TacImGuiSameLine();
        if( TacImGuiButton( "Remove" ) )
        {
          child->Unparent();
        }
      }
    }
    TacVector< TacEntity* > potentialParents;
    for( TacEntity* potentialParent : mCreation->mWorld->mEntities )
    {
      if( potentialParent == entity )
        continue;
      if( entity->mParent == potentialParent )
        continue;
      potentialParents.push_back( potentialParent );
    }
    if( !potentialParents.empty() && TacImGuiCollapsingHeader( "Set Parent" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( TacEntity* potentialParent : potentialParents )
      {
        if( TacImGuiButton( "Set Parent: " + potentialParent->mName ) )
        {
          entity->Unparent();
          potentialParent->mChildren.push_back( entity );
          entity->mParent = potentialParent;
        }
      }
    }

    TacImGuiIndent();
    // all the children, recursively
    TacImGuiUnindent();

    //for( int i = 0; i < ( int )TacComponentRegistryEntryIndex::Count; ++i )
    for( TacComponentRegistryEntry* componentRegistryEntry : TacComponentRegistry::Instance()->mEntries )
    {
      //TacComponentRegistryEntryIndex componentType = ( TacComponentRegistryEntryIndex )i;
      if( !entity->HasComponent( componentRegistryEntry ) )
      {
        addableComponentTypes.push_back( componentRegistryEntry );
        continue;
      }
      TacComponent* component = entity->GetComponent( componentRegistryEntry );
      if( TacImGuiCollapsingHeader( componentRegistryEntry->mName ) )
      {
        TAC_IMGUI_INDENT_BLOCK;
        if( TacImGuiButton( "Remove component" ) )
        {
          entity->RemoveComponent( componentRegistryEntry );
          break;
        }

        if( componentRegistryEntry->mDebugImguiFn )
        {
          componentRegistryEntry->mDebugImguiFn( component );
        }
      }
    }

    if( !addableComponentTypes.empty() && TacImGuiCollapsingHeader( "Add component" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( TacComponentRegistryEntry*  componentType : addableComponentTypes )
      {
        if( TacImGuiButton( va( "Add %s component", componentType->mName.c_str() ) ) )
          entity->AddNewComponent( componentType );
      }
    }
  }
  TacImGuiEndGroup();


  if( TacImGuiButton( "Close window" ) )
    mDesktopWindow->mRequestDeletion = true;

  // temp begin
  TacImGuiDebugDraw();
  // temp end

  TacImGuiEnd();

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
