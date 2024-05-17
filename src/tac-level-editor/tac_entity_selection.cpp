#include "tac_entity_selection.h" // self-inc

// TODO: remove dependency on these begin
#include "tac-level-editor/tac_level_editor.h" // gCreation
#include "tac-level-editor/tac_level_editor_game_window.h" // CreationGameWindow
#include "tac-level-editor/tac_level_editor_prefab.h" // PrefabRemoveEntityRecursively
#include "src/common/input/tac_keyboard_input.h"
// TODO: remove dependency on these end

#include "space/ecs/tac_entity.h"
#include "space/world/tac_world.h"
#include "src/common/algorithm/tac_algorithm.h"
#include "tac-std-lib/math/tac_vector4.h"

namespace Tac
{


  //===-------------- SelectedEntities -------------===//

  void                SelectedEntities::AddToSelection( Entity* e ) { mSelectedEntities.push_back( e ); }

  void                SelectedEntities::DeleteEntities()
  {
    Vector< Entity* > topLevelEntitiesToDelete;

    for( Entity* entity : mSelectedEntities )
    {
      bool isTopLevel = true;
      for( Entity* parent = entity->mParent; parent; parent = parent->mParent )
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
      PrefabRemoveEntityRecursively( entity );
      gCreation.mWorld->KillEntity( entity );
    }

    mSelectedEntities.clear();
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

  v3                  SelectedEntities::GetGizmoOrigin() const
  {
    TAC_ASSERT( !empty() );

    // do i really want average? or like center of bounding circle?
    v3 runningPosSum = {};
    int selectionCount = 0;
    for( Entity* entity : mSelectedEntities )
    {
      runningPosSum += ( entity->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
      entity->mRelativeSpace.mPosition;
      selectionCount++;
    }

    const v3 averagePos = runningPosSum / ( float )selectionCount;
    const v3 result = averagePos;
    //if( mSelectedHitOffsetExists )
    //  result += mSelectedHitOffset;
    return result;
  }

  void                SelectedEntities::clear()
  {
    mSelectedEntities.clear();
  }

  void                SelectedEntities::DeleteEntitiesCheck()
  {
    CreationGameWindow* gameWindow = CreationGameWindow::Instance;
    if( !gameWindow || !gameWindow->mDesktopWindowHandle.IsValid() )
      return;

    DesktopWindowState* desktopWindowState = GetDesktopWindowState( gameWindow->mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    if( !IsWindowHovered( gameWindow->mDesktopWindowHandle ) )
      return;

    if( !KeyboardIsKeyJustDown( Key::Delete ) )
      return;

    DeleteEntities();
  }
} // namespace Tac
