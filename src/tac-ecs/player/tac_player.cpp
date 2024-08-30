#include "tac_player.h" // self-inc

#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/entity/tac_entity.h"

namespace Tac
{

  struct PlayerUUIDMeta
  {
  };

  TAC_META_REGISTER_COMPOSITE_BEGIN( Player )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Player, mPlayerUUID )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Player, mEntityUUID )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Player, mInputDirection )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Player, mIsSpaceJustDown )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Player, mCameraPos )
  TAC_META_REGISTER_COMPOSITE_END( Player );

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

  static NetVarReaderWriter sNetVars;

  void               PlayerNetVarsRegister()
  {
    const MetaCompositeType& metaPlayer{ ( MetaCompositeType& )GetMetaType< Player >() };

    sNetVars = {};
    sNetVars.mMetaType = &metaPlayer;
    sNetVars.Add( TAC_MEMBER_NAME( Player, mPlayerUUID ) );
    sNetVars.Add( TAC_MEMBER_NAME( Player, mInputDirection ) );

#if 0
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
#endif
  }

  const NetVarReaderWriter& PlayerNetVarsGet()
  {
    return sNetVars;
  }
}

