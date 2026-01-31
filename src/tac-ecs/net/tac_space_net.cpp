#include "tac_space_net.h" // self-inc

#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/error/tac_error_handling.h"

void Tac::WriteNetMsgHeader( WriteStream* writer, NetMsgType networkMessageType )
  {
    writer->Write( ( u8 )'t' );
    writer->Write( ( u8 )'a' );
    writer->Write( ( u8 )'c' );
    writer->Write( ( u8 )networkMessageType );
  }

auto Tac::ReadNetMsgHeader( ReadStream* reader,  Errors& errors ) -> NetMsgType
{
  TAC_CALL_RET( const u8 t{ reader->Read< u8 >( errors ) } );
  TAC_CALL_RET( const u8 a{ reader->Read< u8 >( errors ) } );
  TAC_CALL_RET( const u8 c{ reader->Read< u8 >( errors ) } );
  TAC_RAISE_ERROR_IF_RETURN( t != 't', "net msg header mismatch" );
  TAC_RAISE_ERROR_IF_RETURN( a != 'a', "net msg header mismatch" );
  TAC_RAISE_ERROR_IF_RETURN( c != 'c', "net msg header mismatch" );
  return reader->Read< NetMsgType >( errors );
}

namespace Tac
{

  //NetBitDiff GetNetVarfield( const void* oldData,
  //                           const void* newData,
  //                           const NetVars& vars )
  //{
  //  return vars.Diff( oldData, newData );
  //}

  void LagTest::SaveMessage( const Vector< char >& data, GameTime elapsedSecs )
  {
    const GameTimeDelta lagSimSecs { mLagSimulationMS * 0.001f };
    const GameTime delayedTillSecs { elapsedSecs + lagSimSecs };
    const DelayedNetMsg delayedNetMsg
    {
      .mDelayedTillSecs { delayedTillSecs },
      .mData            { data },
    };
    mSavedNetworkMessages.push_back( delayedNetMsg );
  }

  bool LagTest::TryPopSavedMessage( Vector< char >& data, GameTime elapsedSecs )
  {
    if( mSavedNetworkMessages.empty() )
      return false;

    const DelayedNetMsg& savedNetMsg { mSavedNetworkMessages.front() };
    if( elapsedSecs < savedNetMsg.mDelayedTillSecs )
      return false;

    data = savedNetMsg.mData;
    mSavedNetworkMessages.pop_front();
    return true;
  }

  // -----------------------------------------------------------------------------------------------

  SnapshotBuffer::~SnapshotBuffer()
  {
    for( World* world : mSnapshots )
      TAC_DELETE world;
  }

  void SnapshotBuffer::AddSnapshot( const World* world )
  {
    World* snapshot{};
    if( mSnapshots.size() == maxSnapshots )
    {
      snapshot = mSnapshots.front();
      mSnapshots.pop_front();
    }

    if( !snapshot )
      snapshot = TAC_NEW World;

    snapshot->DeepCopy( *world );
    mSnapshots.push_back( snapshot );
  }

  auto SnapshotBuffer::FindSnapshot( GameTime elapsedGameSecs ) -> World*
  {
    for( World* world : mSnapshots )
      if( world->mElapsedSecs == elapsedGameSecs )
        return world;
    return nullptr;
  }

} // namespace Tac

