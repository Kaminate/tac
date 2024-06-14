#include "tac-ecs/player/tac_player.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/entity/tac_entity.h"

namespace Tac
{
  void Player::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( va( "Player %i", mPlayerUUID ) ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //ImGui::DragInt( "entity uuid", ( int* )&mEntityUUID );
    //if( mEntityUUID != NullEntityUUID )
    //{
    //  auto entity = mWorld->FindEntity( mEntityUUID );
    //  entity->DebugImgui();
    //}
    //ImGui::DragFloat2( "input dir", mInputDirection.data(), 0.1f );
    //ImGui::DragFloat3( "camera pos", mCameraPos.data(), 0.1f );
  }

  static NetVars sNetVars;

  void               PlayerNetVarsRegister()
  {
    const NetVar netEntityUUID
    {
      .mDebugName        { "mEntityUUID" },
      .mByteOffset       { ( int )TAC_OFFSET_OF( Player, mEntityUUID ) },
      .mElementByteCount { sizeof( EntityUUID ) },
      .mElementCount     { 1 },
    };

    const NetVar netInputDir
    {
      .mDebugName        { "mInputDirection" },
      .mByteOffset       { ( int )TAC_OFFSET_OF( Player, mInputDirection ) },
      .mElementByteCount { sizeof( float ) },
      .mElementCount     { 2 }
    };

    sNetVars.Clear();
    sNetVars.Add( netEntityUUID );
    sNetVars.Add( netInputDir );
  }

  const NetVars& PlayerNetVarsGet()
  {
    return sNetVars;
  }
}

