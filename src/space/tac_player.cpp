#include "src/space/tac_player.h"
#include "src/space/tac_world.h"
#include "src/space/tac_entity.h"

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
    sNetworkBits = { .mNetworkBits{
      NetworkBit
      {
        .mDebugName= "mEntityUUID",
        .mByteOffset = (int)TAC_OFFSET_OF( Player, mEntityUUID ),
        .mComponentByteCount = sizeof( EntityUUID ),
        .mComponentCount = 1
      },
      NetworkBit
      {
        .mDebugName = "mInputDirection",
        .mByteOffset = (int)TAC_OFFSET_OF( Player, mInputDirection ),
        .mComponentByteCount = sizeof( float ),
        .mComponentCount = 2
      },
    }};
  }

  const NetworkBits& PlayerNetworkBitsGet()
  {
    return sNetworkBits;
  }

}

