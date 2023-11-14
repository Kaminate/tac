#include "src/level_editor/tac_level_editor_property_window.h" // self-inc

#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/core/tac_algorithm.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_os.h"
#include "src/level_editor/tac_level_editor.h"
#include "src/level_editor/tac_level_editor_prefab.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/space/model/tac_model.h"
#include "src/space/tac_component.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_space_types.h"
#include "src/space/tac_system.h"
#include "src/space/tac_world.h"

import std; // #include <map>

namespace Tac
{
  static void EntityImGuiRotation( Entity* entity )
  {
    v3& eulerRads = entity->mRelativeSpace.mEulerRads;
    v3 eulerDegs = RadiansToDegrees( eulerRads );

    if( !ImGuiDragFloat3( "eul deg", eulerDegs.data() ) )
      return;

    for( float& eulerDeg : eulerDegs )
    {
      eulerDeg = Fmod( eulerDeg, 360.0f );
      eulerDeg -= eulerDeg > 360 ? 360 : 0;
      eulerDeg += eulerDeg < -360 ? 360 : 0;
    }

    eulerRads = DegreesToRadians( eulerDegs );
  }

  static void EntityImGuiScale( Entity* entity )
  {
    // true if requested to be uniform, false if requested to be separate
    static std::map< Entity*, bool > sRequests;

    v3& scale = entity->mRelativeSpace.mScale;

    bool isUniform
      =  sRequests.contains( entity ) 
      ?  sRequests[ entity ]
      : scale.x == scale.y && scale.x == scale.z;

    if( isUniform )
    {
      if( ImGuiDragFloat( "Scale", &scale.x ) )
      {
        scale = v3( scale.x );
      }
    }
    else
    {
      ImGuiDragFloat3( "Scale", scale.data() );
    }

    if( ImGuiCheckbox( "Uniform scale", &isUniform ) )
      sRequests[ entity ] = isUniform;
  }

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
    const bool previouslySelected = gCreation.mSelectedEntities.IsSelected( entity );
    if( ImGuiSelectable( entity->mName, previouslySelected ) )
    {
      gCreation.mSelectedEntities.Select( entity );
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
    TAC_PROFILE_BLOCK;

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
      AssetPathStringView prefabAssetPath = GetAssetOpenDialog(errors );
      TAC_HANDLE_ERROR( errors );

      //String prefabPath;
      //OS::OSOpenDialog( prefabPath, errors );
      //TAC_HANDLE_ERROR( errors );
      if( prefabAssetPath.size() )
      {
        Camera* cam = world->mEntities.size() ? nullptr : gCreation.mEditorCamera;
        PrefabLoadAtPath( &gCreation.mEntityUUIDCounter, world, cam, prefabAssetPath, errors );
        TAC_HANDLE_ERROR( errors );
      }
    }
    ImGuiEndGroup();
    ImGuiSameLine();
    ImGuiBeginGroup();

    for( Entity* entity : gCreation.mSelectedEntities )
    {
      ImGuiInputText( "Name", entity->mName );

      AssetPathStringView prefabPath = PrefabGetOrNull( entity );
      ImGuiText(va( "Prefab path: {},", prefabPath.c_str() ));

      ImGuiText( "UUID: " + ToString( ( UUID )entity->mEntityUUID ) );
      if( ImGuiButton( "Reset Transform" ) )
        entity->mRelativeSpace = {};

      ImGuiDragFloat3( "Position", entity->mRelativeSpace.mPosition.data() );
      EntityImGuiRotation( entity );
      EntityImGuiScale( entity );

      ImGuiCheckbox( "active", &entity->mActive );

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
          if( ImGuiButton( "Select" ) )
          {
            gCreation.mSelectedEntities.Select( child );
          }

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

      for( const ComponentRegistryEntry& componentRegistryEntry : ComponentRegistryIterator() )
      {
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
          if( ImGuiButton( va( "Add {} component", componentType->mName ) ) )
          {
            entity->AddNewComponent( componentType );
          }
        }
      }
    }
    ImGuiEndGroup();


    mCloseRequested |= ImGuiButton( "Close window" );

    ImGuiDebugDraw();

    ImGuiEnd();

    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );
    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );;
    const v2 size = desktopWindowState->GetSizeV2();
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Render::Viewport(size) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect(size) );
    TAC_HANDLE_ERROR( errors );
  }
}
