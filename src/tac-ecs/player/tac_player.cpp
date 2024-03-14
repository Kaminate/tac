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

  static NetworkBits sNetworkBits;

  void               PlayerNetworkBitsRegister()
  {
    sNetworkBits.Clear();
    sNetworkBits.Add(
      NetworkBit
      {
        .mDebugName = "mEntityUUID",
        .mByteOffset = ( int )TAC_OFFSET_OF( Player, mEntityUUID ),
        .mComponentByteCount = sizeof( EntityUUID ),
        .mComponentCount = 1
      } );

    sNetworkBits.Add(
      NetworkBit
      {
        .mDebugName = "mInputDirection",
        .mByteOffset = (int)TAC_OFFSET_OF( Player, mInputDirection ),
        .mComponentByteCount = sizeof( float ),
        .mComponentCount = 2
      } );
  }

  const NetworkBits& PlayerNetworkBitsGet()
  {
    return sNetworkBits;
  }

}

