#include "tac_level_editor_property_window.h" // self-inc

#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/algorithm/tac_algorithm.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/containers/tac_map.h"
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

#include "space/graphics/model/tac_model.h"
#include "space/ecs/tac_component.h"
#include "space/ecs/tac_entity.h"
#include "space/ecs/tac_component_registry.h"
#include "space/tac_space_types.h"
#include "space/ecs/tac_system.h"
#include "space/world/tac_world.h"

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
    static Map< Entity*, bool > sRequests;

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
        const ShortFixedString str = ShortFixedString::Concat(
          "Add ",
          componentType->mName,
          " component" );

        if( ImGuiButton( str ) )
        {
          entity->AddNewComponent( componentType );
        }
      }
    }
  }

  CreationPropertyWindow* CreationPropertyWindow::Instance = nullptr;

  CreationPropertyWindow::CreationPropertyWindow()
  {
    Instance = this;
  }

  CreationPropertyWindow::~CreationPropertyWindow()
  {
    Instance = nullptr;
    DesktopApp::GetInstance()->DestroyWindow( mDesktopWindowHandle );
  }

  void CreationPropertyWindow::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mDesktopWindowHandle = gCreation.mWindowManager.CreateDesktopWindow( gPropertyWindowName );
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
      TAC_CALL( const AssetPathStringView prefabAssetPath = AssetOpenDialog( errors ) );

      if( prefabAssetPath.size() )
      {
        Camera* cam = world->mEntities.size() ? nullptr : gCreation.mEditorCamera;
        TAC_CALL( PrefabLoadAtPath(
          &gCreation.mEntityUUIDCounter,
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
    {
      EntityImGui( entity );
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
  }
}
