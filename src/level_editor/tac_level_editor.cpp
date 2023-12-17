#include "src/level_editor/tac_level_editor.h" // self-inc

// common
#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/algorithm/tac_algorithm.h"
#include "src/common/preprocess/tac_preprocessor.h"
#include "src/common/dataprocess/tac_json.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/graphics/tac_color_util.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/math/tac_math.h"
#include "src/common/meta/tac_meta_composite.h"
#include "src/common/meta/tac_meta_fn.h"
#include "src/common/meta/tac_meta_fn_sig.h"
#include "src/common/meta/tac_meta_var.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/system/tac_os.h"

// level_editor
#include "src/level_editor/tac_level_editor_asset_view.h"
#include "src/level_editor/tac_level_editor_game_window.h"
#include "src/level_editor/tac_level_editor_main_window.h"
#include "src/level_editor/tac_level_editor_prefab.h"
#include "src/level_editor/tac_level_editor_profile_window.h"
#include "src/level_editor/tac_level_editor_property_window.h"
#include "src/level_editor/tac_level_editor_system_window.h"

// shell
#include "src/shell/tac_desktop_app.h"

// space
#include "space/graphics/model/tac_model.h"
#include "space/presentation/tac_game_presentation.h"
#include "space/presentation/tac_shadow_presentation.h"
#include "space/presentation/tac_skybox_presentation.h"
#include "space/presentation/tac_voxel_gi_presentation.h"
#include "space/ecs/tac_entity.h"
#include "space/ghost/tac_ghost.h"
#include "space/tac_space.h"
#include "space/world/tac_world.h"
#include "space/terrain/tac_terrain.h"


import std; // iostream, functional, algorithm

namespace Tac
{
  Creation gCreation;

  struct CreatedWindowData
  {
    String              mName;
    int                 mX;
    int                 mY;
    int                 mW;
    int                 mH;
    const void*         mNativeWindowHandle;
  };
  
  static CreatedWindowData sCreatedWindowData[ kDesktopWindowCapacity ]{};

  static void   CreationInitCallback( Errors& errors )   { gCreation.Init( errors ); }
  static void   CreationUninitCallback( Errors& errors ) { gCreation.Uninit( errors ); }
  static void   CreationUpdateCallback( Errors& errors ) { gCreation.Update( errors ); }

  static String CreationGetNewEntityName()
  {
    World* world = gCreation.mWorld;
    String desiredEntityName = "Entity";
    int parenNumber = 1;
    for( ;; )
    {
      if( !world->FindEntity( desiredEntityName ) )
        break;

      desiredEntityName = "Entity (" + ToString( parenNumber ) + ")";
      parenNumber++;
    }

    return desiredEntityName;
  }

  static void   CheckSavePrefab()
  {
    World* world = gCreation.mWorld;

    const bool triggered =
      Keyboard::KeyboardIsKeyJustDown( Keyboard::Key::S ) &&
      Keyboard::KeyboardIsKeyDown( Keyboard::Key::Modifier );

    if( !triggered )
      return;

    Errors saveErrors;
    PrefabSave( world, saveErrors );

    CreationGameWindow* window = CreationGameWindow::Instance;
    if( window )
    {
      if( saveErrors )
      {
        const TimestampDifference errorDurationSecs = 60.0f;
        window->SetStatusMessage( saveErrors.ToString(), errorDurationSecs );
      }
      else
      {
        const TimestampDifference successDurationSecs = 5.0f;
        window->SetStatusMessage( "Saved prefabs!", successDurationSecs );
      }
    }
  }

  //===-------------- App -------------===//

  struct LevelEditorApp : public App
  {
    LevelEditorApp( const Config& cfg ) : App( cfg ) {}
    void Init( Errors& errors ) override { CreationInitCallback( errors ); }
    void Update( Errors& errors ) override { CreationUpdateCallback( errors ); }
    void Uninit( Errors& errors ) override { CreationUninitCallback( errors ); }
  };

  App* App::Create() { return TAC_NEW LevelEditorApp( { .mName = "Level Editor" } ); }

  //===-------------- Creation -------------===//

  void                Creation::Init( Errors& errors )
  {
    mWorld = TAC_NEW World;
    mEditorCamera = TAC_NEW Camera
    {
      .mPos = { 0, 1, 5 },
      .mForwards = { 0, 0, -1 },
      .mRight = { 1, 0, 0 },
      .mUp = { 0, 1, 0 }
    };

    TAC_CALL( SkyboxPresentationInit( errors ) );

    TAC_CALL( GamePresentationInit( errors ) );

    TAC_CALL( ShadowPresentationInit( errors ) );

    TAC_CALL( VoxelGIPresentationInit( errors ) );

    TAC_CALL( mWindowManager.CreateInitialWindows( errors ) );

    TAC_CALL( PrefabLoad( &mEntityUUIDCounter, mWorld, mEditorCamera, errors ) );
  }

  void                Creation::Uninit( Errors& errors )
  {
    SkyboxPresentationUninit();
    GamePresentationUninit();
    VoxelGIPresentationUninit();
    ShadowPresentationUninit();

    mWindowManager.Uninit( errors );
  }

  void                Creation::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    CheckSavePrefab();

    TAC_CALL( mWindowManager.Update( errors ) );

    if( mWindowManager.AllWindowsClosed() )
      OS::OSAppStopRunning();

    if( mUpdateAssetView )
      CreationUpdateAssetView();

    mWorld->Step( TAC_DELTA_FRAME_SECONDS );


    mSelectedEntities.DeleteEntitiesCheck();
  }


  RelativeSpace       Creation::GetEditorCameraVisibleRelativeSpace()
  {
    const v3 pos = mEditorCamera->mPos + mEditorCamera->mForwards * 5.0f;
    RelativeSpace relativeSpace;
    relativeSpace.mPosition = pos;
    return relativeSpace;
  }

  Entity*             Creation::InstantiateAsCopy( Entity* prefabEntity, const RelativeSpace& relativeSpace )
  {
    Entity* copyEntity = CreateEntity();
    copyEntity->mRelativeSpace = relativeSpace;
    copyEntity->mInheritParentScale = prefabEntity->mInheritParentScale;
    copyEntity->mName = prefabEntity->mName;

    for( Component* prefabComponent : prefabEntity->mComponents )
    {
      const ComponentRegistryEntry* entry = prefabComponent->GetEntry();
      Component* copyComponent = copyEntity->AddNewComponent( prefabComponent->GetEntry() );
      Json dOnT_mInD_iF_i_dO;
      entry->mSaveFn( dOnT_mInD_iF_i_dO, prefabComponent );
      entry->mLoadFn( dOnT_mInD_iF_i_dO, copyComponent );
    }

    for( Entity* prefabChildEntity : prefabEntity->mChildren )
    {
      Entity* copyChildEntity = InstantiateAsCopy( prefabChildEntity, prefabChildEntity->mRelativeSpace );
      //Entity* copyChildEntity = CreateEntity();
      copyEntity->AddChild( copyChildEntity );
    }
    return copyEntity;
  }

  Entity*             Creation::CreateEntity()
  {
    World* world = mWorld;
    Entity* entity = world->SpawnEntity( mEntityUUIDCounter.AllocateNewUUID() );
    entity->mName = CreationGetNewEntityName();
    entity->mRelativeSpace = GetEditorCameraVisibleRelativeSpace();
    mSelectedEntities.Select( entity );
    return entity;
  }



} // namespace Tac

