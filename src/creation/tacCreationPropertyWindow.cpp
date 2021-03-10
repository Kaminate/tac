#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacOS.h"
#include "src/common/tacShell.h"
#include "src/common/tacUtility.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationPropertyWindow.h"
#include "src/creation/tacCreationPrefab.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowGraphics.h"
#include "src/space/model/tacModel.h"
#include "src/space/tacComponent.h"
#include "src/space/tacEntity.h"
#include "src/space/tacSpacetypes.h"
#include "src/space/tacSystem.h"
#include "src/space/tacWorld.h"

namespace Tac
{

	CreationPropertyWindow* CreationPropertyWindow::Instance = nullptr;

	CreationPropertyWindow::CreationPropertyWindow()
	{
		Instance = this;
	}

	CreationPropertyWindow::~CreationPropertyWindow()
	{
		Instance = nullptr;
    DesktopAppDestroyWindow( mDesktopWindowHandle );
	}

	void CreationPropertyWindow::Init( Errors& errors )
	{
		TAC_UNUSED_PARAMETER( errors );
		mDesktopWindowHandle = gCreation.CreateWindow( gPropertyWindowName );
	}

	void CreationPropertyWindow::RecursiveEntityHierarchyElement( Entity* entity )
	{
		bool previouslySelected = Contains( gCreation.mSelectedEntities, entity );
		if( ImGuiSelectable( entity->mName, previouslySelected ) )
		{
			gCreation.ClearSelection();
			gCreation.mSelectedEntities = { entity };
		}
		if( entity->mChildren.empty() )
			return;
		TAC_IMGUI_INDENT_BLOCK;
		for( Entity* child : entity->mChildren )
		{
			RecursiveEntityHierarchyElement( child );
		}
	}

	void CreationPropertyWindow::Update( Errors& errors )
	{

		TAC_HANDLE_ERROR( errors );

		DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
		if( !desktopWindowState->mNativeWindowHandle )
			return;


		ImGuiSetNextWindowHandle( mDesktopWindowHandle );
    ImGuiSetNextWindowStretch();
		ImGuiBegin( "Properties" );



		ImGuiBeginGroup();
		ImGuiBeginChild( "Hierarchy", v2( 250, -100 ) );



		World* world = gCreation.mWorld;
		for( Entity* entity : world->mEntities )
		{
			if( !entity->mParent )
				RecursiveEntityHierarchyElement( entity );
		}
		ImGuiEndChild();
		if( ImGuiButton( "Create Entity" ) )
			gCreation.CreateEntity();
		if( ImGuiButton( "Open Prefab" ) )
		{
			String prefabPath;
			OS::OpenDialog( prefabPath, errors );
			TAC_HANDLE_ERROR( errors );
      if( prefabPath.size() )
      {
        Camera* cam = world->mEntities.size() ? nullptr : &gCreation.mEditorCamera;
        PrefabLoadAtPath( world, cam, prefabPath, errors );
        TAC_HANDLE_ERROR( errors );
      }
		}
		ImGuiEndGroup();
		ImGuiSameLine();
		ImGuiBeginGroup();

		for( Entity* entity : gCreation.mSelectedEntities )
		{
			const char* occupation = "Bartender";
			ImGuiInputText( "Name", entity->mName );
			ImGuiText( "UUID: " + ToString( ( UUID )entity->mEntityUUID ) );
			ImGuiDragFloat( "X Position: ", &entity->mRelativeSpace.mPosition.x );
			ImGuiDragFloat( "Y Position: ", &entity->mRelativeSpace.mPosition.y );
			ImGuiDragFloat( "Z Position: ", &entity->mRelativeSpace.mPosition.z );
			ImGuiDragFloat( "X Scale: ", &entity->mRelativeSpace.mScale.x );
			ImGuiDragFloat( "Y Scale: ", &entity->mRelativeSpace.mScale.y );
			ImGuiDragFloat( "Z Scale: ", &entity->mRelativeSpace.mScale.z );
			v3 rotDeg = entity->mRelativeSpace.mEulerRads * ( 180.0f / 3.14f );
			bool changed = false;
			changed |= ImGuiDragFloat( "X Eul Deg: ", &rotDeg.x );
			changed |= ImGuiDragFloat( "Y Eul Deg: ", &rotDeg.y );
			changed |= ImGuiDragFloat( "Z Eul Deg: ", &rotDeg.z );
			if( changed )
				entity->mRelativeSpace.mEulerRads = rotDeg * ( 3.14f / 180.0f );
			Vector< const ComponentRegistryEntry* > addableComponentTypes;

			if( entity->mParent )
			{
				ImGuiText( "Parent: " + entity->mParent->mName );
				ImGuiSameLine();
				if( ImGuiButton( "Unparent" ) )
					entity->Unparent();
			}
			if( entity->mChildren.size() && ImGuiCollapsingHeader( "Children" ) )
			{
				TAC_IMGUI_INDENT_BLOCK;
				Vector< Entity* > childrenCopy = entity->mChildren; // For iterator invalidation
				for( Entity* child : childrenCopy )
				{
					ImGuiText( child->mName );
					ImGuiSameLine();
					if( ImGuiButton( "Remove" ) )
					{
						child->Unparent();
					}
				}
			}
			Vector< Entity* > potentialParents;
			for( Entity* potentialParent : gCreation.mWorld->mEntities )
			{
				if( potentialParent == entity )
					continue;
				if( entity->mParent == potentialParent )
					continue;
				potentialParents.push_back( potentialParent );
			}
			if( !potentialParents.empty() && ImGuiCollapsingHeader( "Set Parent" ) )
			{
				TAC_IMGUI_INDENT_BLOCK;
				for( Entity* potentialParent : potentialParents )
				{
					if( ImGuiButton( "Set Parent: " + potentialParent->mName ) )
					{
						entity->Unparent();
						potentialParent->mChildren.push_back( entity );
						entity->mParent = potentialParent;
					}
				}
			}

			ImGuiIndent();
			// all the children, recursively
			ImGuiUnindent();

			//for( int i = 0; i < ( int )ComponentRegistryEntryIndex::Count; ++i )
			for( const ComponentRegistryEntry& componentRegistryEntry : ComponentRegistryIterator() )
			{
				//ComponentRegistryEntryIndex componentType = ( ComponentRegistryEntryIndex )i;
				if( !entity->HasComponent( &componentRegistryEntry ) )
				{
					addableComponentTypes.push_back( &componentRegistryEntry );
					continue;
				}
				Component* component = entity->GetComponent( &componentRegistryEntry );
				if( ImGuiCollapsingHeader( componentRegistryEntry.mName ) )
				{
					TAC_IMGUI_INDENT_BLOCK;
					if( ImGuiButton( "Remove component" ) )
					{
						entity->RemoveComponent( &componentRegistryEntry );
						break;
					}

					if( componentRegistryEntry.mDebugImguiFn )
					{
						componentRegistryEntry.mDebugImguiFn( component );
					}
				}
			}

			if( !addableComponentTypes.empty() && ImGuiCollapsingHeader( "Add component" ) )
			{
				TAC_IMGUI_INDENT_BLOCK;
				for( const ComponentRegistryEntry* componentType : addableComponentTypes )
				{
					if( ImGuiButton( va( "Add %s component", componentType->mName ) ) )
					{
						entity->AddNewComponent( componentType );
					}
				}
			}
		}
		ImGuiEndGroup();


    mCloseRequested |= ImGuiButton( "Close window" );

		// temp begin
		ImGuiDebugDraw();
		// temp end

		ImGuiEnd();

		const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );
		const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );;
		const float w = ( float )desktopWindowState->mWidth;
		const float h = ( float )desktopWindowState->mHeight;
		Render::SetViewFramebuffer( viewHandle, framebufferHandle );
		Render::SetViewport( viewHandle, Viewport( w, h ) );
		Render::SetViewScissorRect( viewHandle, ScissorRect( w, h ) );
		TAC_HANDLE_ERROR( errors );
	}
}
