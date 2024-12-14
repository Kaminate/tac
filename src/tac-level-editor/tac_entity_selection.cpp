#include "tac_entity_selection.h" // self-inc

// TODO: remove dependency on these begin
#include "tac-level-editor/tac_level_editor.h" // gCreation
#include "tac-level-editor/tac_level_editor_game_window.h" // CreationGameWindow
#include "tac-level-editor/tac_level_editor_prefab.h" // PrefabRemoveEntityRecursively
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
// TODO: remove dependency on these end

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/math/tac_vector4.h"

namespace Tac
{



  //===-------------- SelectedEntities -------------===//

  void SelectedEntities::Init( SettingsNode settingsNode)
  {
    mSettingsNode = settingsNode;
  }

  void                SelectedEntities::AddToSelection( Entity* e ) { mSelectedEntities.push_back( e ); }

  void                SelectedEntities::DeleteEntities()
  {
    SettingsNode settingsNode{ mSettingsNode };
    Vector< Entity* > topLevelEntitiesToDelete;

    for( Entity* entity : mSelectedEntities )
    {
      bool isTopLevel { true };
      for( Entity* parent { entity->mParent }; parent; parent = parent->mParent )
      {
        if( Contains( mSelectedEntities, parent ) )
        {
          isTopLevel = false;
          break;
        }
      }

      if( isTopLevel )
      {
        topLevelEntitiesToDelete.push_back( entity );
      }
    }

    for( Entity* entity : topLevelEntitiesToDelete )
    {
      PrefabRemoveEntityRecursively( settingsNode, entity );
      entity->mWorld->KillEntity( entity );
    }

    mSelectedEntities.clear();
  }

  v3 SelectedEntities::ComputeAveragePosition()
  {
    const int n{ mSelectedEntities.size() };
    TAC_ASSERT( n );
    float scale{ 1.0f / n };
    v3 result{};
    for( int i{}; i < n; ++i )
      result += scale * mSelectedEntities[i]->mWorldPosition;
    return result;
  }

  bool                SelectedEntities::IsSelected( Entity* e )
  {
    for( Entity* s : mSelectedEntities )
      if( s == e )
        return true;
    return false;
  }

  int                 SelectedEntities::size() const { return mSelectedEntities.size(); }

  Entity** SelectedEntities::begin() { return mSelectedEntities.begin(); }

  Entity** SelectedEntities::end() { return mSelectedEntities.end(); }

  void                SelectedEntities::Select( Entity* e ) { mSelectedEntities = { e }; }

  bool                SelectedEntities::empty() const
  {
    return mSelectedEntities.empty();
  }


  void                SelectedEntities::clear()
  {
    mSelectedEntities.clear();
  }

  void                SelectedEntities::DeleteEntitiesCheck()
  {
    SimKeyboardApi:: keyboardApi{};
    if( AppKeyboardApi::JustPressed( Key::Delete ) )
      DeleteEntities();
  }
} // namespace Tac
