#include "tac_entity_selection.h" // self-inc

// TODO: remove dependency on these begin
#include "tac-level-editor/tac_level_editor.h" // gCreation
#include "tac-level-editor/tac_level_editor_game_window.h" // CreationGameWindow
#include "tac-level-editor/tac_level_editor_prefab.h" // PrefabRemoveEntityRecursively
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
// TODO: remove dependency on these end

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/math/tac_vector4.h"

namespace Tac
{
  static Vector< Entity* >   sSelectedEntities;
  static SettingsNode        sSettingsNode;

  auto SelectedEntities::begin() -> Iterator
  {
    return Iterator
    {
      .mBegin = sSelectedEntities.data(),
      .mEnd = sSelectedEntities.data() + sSelectedEntities.size(),
      .mCurr = sSelectedEntities.data(),
    };
  }

  auto SelectedEntities::end() -> Iterator
  {
    return Iterator
    {
      .mBegin = sSelectedEntities.data(),
      .mEnd = sSelectedEntities.data() + sSelectedEntities.size(),
      .mCurr = sSelectedEntities.data() + sSelectedEntities.size(),
    };
  }

  void SelectedEntities::Init( SettingsNode settingsNode)
  {
    sSettingsNode = settingsNode;
  }

  void SelectedEntities::AddToSelection( Entity* e ) { sSelectedEntities.push_back( e ); }

  void SelectedEntities::DeleteEntitiesCheck()
  {
    if( AppKeyboardApi::JustPressed( Key::Delete ) )
      DeleteEntities();
  }

  void SelectedEntities::DeleteEntities()
  {
    SettingsNode settingsNode{ sSettingsNode };
    Vector< Entity* > topLevelEntitiesToDelete;

    for( Entity* entity : sSelectedEntities )
    {
      bool isTopLevel { true };
      for( Entity* parent { entity->mParent }; parent; parent = parent->mParent )
      {
        if( Contains( sSelectedEntities, parent ) )
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

    sSelectedEntities.clear();
  }

  auto SelectedEntities::ComputeAveragePosition() -> v3
  {
    const int n{ sSelectedEntities.size() };
    TAC_ASSERT( n );
    float scale{ 1.0f / n };
    v3 result{};
    for( int i{}; i < n; ++i )
      result += scale * sSelectedEntities[i]->mWorldPosition;
    return result;
  }

  bool SelectedEntities::IsSelected( Entity* e )
  {
    for( Entity* s : sSelectedEntities )
      if( s == e )
        return true;
    return false;
  }

  void SelectedEntities::Select( Entity* e ) { sSelectedEntities = { e }; }

  bool SelectedEntities::empty() { return sSelectedEntities.empty(); }

  void SelectedEntities::clear()
  {
    sSelectedEntities.clear();
  }

  void SelectedEntities::Iterator::operator ++() { mCurr++; }
  bool SelectedEntities::Iterator::operator != ( Iterator& i ) const { return mCurr != i.mCurr; }
  auto SelectedEntities::Iterator::operator* () const -> Entity*{ return *mCurr; }

} // namespace Tac
