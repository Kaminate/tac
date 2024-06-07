#include "tac_level_editor_property_window.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/tac_level_editor_prefab.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"

namespace Tac
{
  static void EntityImGuiRotation( Entity* entity )
  {
    v3& eulerRads { entity->mRelativeSpace.mEulerRads };
    v3 eulerDegs { RadiansToDegrees( eulerRads ) };

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
    static Map< Entity*, bool > sRequests;

    v3& scale { entity->mRelativeSpace.mScale };

    bool isUniform{
      sRequests.contains( entity )
    ? sRequests[ entity ]
    : scale.x == scale.y && scale.x == scale.z };

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

  static void EntityImGui(Entity* entity )
  {
    ImGuiInputText( "Name", entity->mName );

    const AssetPathStringView prefabPath = PrefabGetOrNull( entity );
    if( prefabPath.empty() )
    {
      ImGuiText( "<not a prefab>" );
    }
    else
    {
      ImGuiText(ShortFixedString::Concat( "Prefab path: ", prefabPath ));
    }

    ImGuiText( ShortFixedString::Concat( "UUID: ", ToString( ( UUID )entity->mEntityUUID ) ) );
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

      Component* component { entity->GetComponent( &componentRegistryEntry ) };
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
        const ShortFixedString str{ ShortFixedString::Concat(
          "Add ",
          componentType->mName,
          " component" ) };

        if( ImGuiButton( str ) )
        {
          entity->AddNewComponent( componentType );
        }
      }
    }
  }

  static void RecursiveEntityHierarchyElement( Entity* entity )
  {
    const bool previouslySelected { gCreation.mSelectedEntities.IsSelected( entity ) };
    if( ImGuiSelectable( entity->mName, previouslySelected ) )
      gCreation.mSelectedEntities.Select( entity );

    if( entity->mChildren.empty() )
      return;

    TAC_IMGUI_INDENT_BLOCK;
    for( Entity* child : entity->mChildren )
      RecursiveEntityHierarchyElement( child );
  }

  // -----------------------------------------------------------------------------------------------

  bool CreationPropertyWindow::sShowWindow{};


  void CreationPropertyWindow::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;


    ImGuiSetNextWindowStretch();
    if( !ImGuiBegin( "Properties" ) )
      return;

    ImGuiBeginGroup();
    ImGuiBeginChild( "Hierarchy", v2( 250, -100 ) );

    World* world = gCreation.mWorld;
    for( Entity* entity : world->mEntities )
      if( !entity->mParent )
        RecursiveEntityHierarchyElement( entity );

    ImGuiEndChild();

    if( ImGuiButton( "Create Entity" ) )
      gCreation.CreateEntity();

    if( ImGuiButton( "Open Prefab" ) )
    {
      TAC_CALL( const AssetPathStringView prefabAssetPath = AssetOpenDialog( errors ) );

      if( prefabAssetPath.size() )
      {
        Camera* cam { world->mEntities.size() ? nullptr : gCreation.mEditorCamera };
        TAC_CALL( PrefabLoadAtPath( &gCreation.mEntityUUIDCounter,
                                    world,
                                    cam,
                                    prefabAssetPath,
                                    errors ) );
      }
    }
    ImGuiEndGroup();
    ImGuiSameLine();
    ImGuiBeginGroup();

    for( Entity* entity : gCreation.mSelectedEntities )
      EntityImGui( entity );

    ImGuiEndGroup();

    if( ImGuiButton( "Close window" ) )
      sShowWindow = false;

    ImGuiDebugDraw();
    ImGuiEnd();
  }
}
