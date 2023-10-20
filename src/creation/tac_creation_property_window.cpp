#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/core/tac_algorithm.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/system/tac_os.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/string/tac_string_util.h"
#include "src/creation/tac_creation.h"
#include "src/creation/tac_creation_property_window.h"
#include "src/creation/tac_creation_prefab.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/space/model/tac_model.h"
#include "src/space/tac_component.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_space_types.h"
#include "src/space/tac_system.h"
#include "src/space/tac_world.h"

#include <map>

namespace Tac
{
  static void EntityImGuiRotation( Entity* entity )
  {
    v3 rotDeg = entity->mRelativeSpace.mEulerRads * ( 180.0f / 3.14f );
    const v3 rotDegOld = rotDeg;
    for( int i = 0; i < 3; ++i )
    {
      float& rF = rotDeg[ i ];
      float* pF = &rotDeg[ i ];
      const char* axisNames[] = { "X","Y","Z" };
      const char* axisName = axisNames[ i ];
      //rF += ImGuiButton( "-90" ) ? -90 : 0;
      //ImGuiSameLine();
      //rF += ImGuiButton( "+90" ) ? 90 : 0;
      //ImGuiSameLine();
      ImGuiDragFloat( FrameMemoryPrintf( "%s eul deg", axisName ), pF );
      rF += rF > 360 ? -360 : 0;
      rF += rF < -360 ? 360 : 0;
    }
    if( rotDeg != rotDegOld )
      entity->mRelativeSpace.mEulerRads = rotDeg * ( 3.14f / 180.0f );
  }

  static void EntityImGuiScale( Entity* entity )
  {
    static std::map< Entity*, bool > sRequestSeparateScaleMap;
    const bool uniformScale = ( entity->mRelativeSpace.mScale.x == entity->mRelativeSpace.mScale.y &&
                                entity->mRelativeSpace.mScale.x == entity->mRelativeSpace.mScale.z );

    const auto it = sRequestSeparateScaleMap.find( entity );
    const bool requestedSeparateScale = it != sRequestSeparateScaleMap.end() && ( *it ).second;
    const bool showSeparateScale = requestedSeparateScale || !uniformScale;

    if( showSeparateScale )
    {
      ImGuiDragFloat( "X Scale: ", &entity->mRelativeSpace.mScale.x );
      ImGuiDragFloat( "Y Scale: ", &entity->mRelativeSpace.mScale.y );
      ImGuiDragFloat( "Z Scale: ", &entity->mRelativeSpace.mScale.z );
      if( ImGuiButton( "Use uniform scale controls" ) )
      {
        entity->mRelativeSpace.mScale = v3( 1, 1, 1 ) * ( entity->mRelativeSpace.mScale.x +
                                                          entity->mRelativeSpace.mScale.y +
                                                          entity->mRelativeSpace.mScale.z ) / 3.0f;
        sRequestSeparateScaleMap[ entity ] = false;
      }
    }
    else
    {
      if( ImGuiButton( "show separate scale" ) )
        sRequestSeparateScaleMap.insert( { entity, true } );
      ImGuiSameLine();
      if( ImGuiDragFloat( "scale: ", &entity->mRelativeSpace.mScale.x ) )
        entity->mRelativeSpace.mScale = v3( 1, 1, 1 ) * entity->mRelativeSpace.mScale.x;
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
      String prefabPath;
      OS::OSOpenDialog( prefabPath, errors );
      TAC_HANDLE_ERROR( errors );
      if( prefabPath.size() )
      {
        Camera* cam = world->mEntities.size() ? nullptr : gCreation.mEditorCamera;
        PrefabLoadAtPath( &gCreation.mEntityUUIDCounter, world, cam, prefabPath, errors );
        TAC_HANDLE_ERROR( errors );
      }
    }
    ImGuiEndGroup();
    ImGuiSameLine();
    ImGuiBeginGroup();

    for( Entity* entity : gCreation.mSelectedEntities )
    {
      ImGuiInputText( "Name", entity->mName );

      const char* prefabPath = PrefabGetOrNull( entity );
      ImGuiText( "Prefab path: " + String( prefabPath ? prefabPath : "null" ) );

      ImGuiText( "UUID: " + ToString( ( UUID )entity->mEntityUUID ) );
      if( ImGuiButton( "Reset Transform" ) )
      {
        entity->mRelativeSpace.mPosition = {};
        entity->mRelativeSpace.mEulerRads = {};
        entity->mRelativeSpace.mScale = { 1, 1, 1 };
      }
      ImGuiDragFloat( "X Position: ", &entity->mRelativeSpace.mPosition.x );
      ImGuiDragFloat( "Y Position: ", &entity->mRelativeSpace.mPosition.y );
      ImGuiDragFloat( "Z Position: ", &entity->mRelativeSpace.mPosition.z );
      EntityImGuiScale( entity );

      ImGuiCheckbox( "active", &entity->mActive );
      EntityImGuiRotation( entity );

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
          if( ImGuiButton( va( "Add %s component", componentType->mName ) ) )
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
    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Render::Viewport( w, h ) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect( w, h ) );
    TAC_HANDLE_ERROR( errors );
  }
}
