
#include "src/space/tacPlayer.h"
#include "src/space/tacWorld.h"
#include "src/space/tacEntity.h"
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
    sNetworkBits.Add( { "mEntityUUID",     TAC_OFFSET_OF( Player, mEntityUUID ),     sizeof( EntityUUID ), 1 } );
    sNetworkBits.Add( { "mInputDirection", TAC_OFFSET_OF( Player, mInputDirection ), sizeof( float ),      2 } );
  }

  const NetworkBits& PlayerNetworkBitsGet()
  {
    return sNetworkBits;
  }

}

