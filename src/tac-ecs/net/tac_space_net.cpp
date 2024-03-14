#include "tac-ecs/net/tac_space_net.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  const String tac( "tac" );

  void WriteNetMsgHeader( Writer* writer, NetMsgType networkMessageType )
  {
    writer->Write( tac.data(), tac.size(), 1 );
    writer->Write( networkMessageType );
  }

  NetMsgType ReadNetMsgHeader( Reader* reader,  Errors& errors )
  {
    for( const char cExpected : tac )
    {
      const char cActual = TAC_CALL_RET( {}, reader->Read< char >( errors ) );
      TAC_RAISE_ERROR_IF_RETURN( cExpected != cActual, "net msg header mismatch", {} );
    }

    return reader->Read<NetMsgType>( errors );
  }

  NetBitDiff GetNetworkBitfield( const void* oldData,
                                 const void* newData,
                                 const NetworkBits& networkBits )
  {
    if( !oldData )
      return NetBitDiff{ 0xff };

    u8 bitfield = 0;
    for( int i = 0; i < networkBits.size(); ++i )
    {
      const NetworkBit& bits = networkBits[ i ];
      auto oldBits = ( char* )oldData + bits.mByteOffset;
      auto newBits = ( char* )newData + bits.mByteOffset;
      auto componentTotalSize = bits.mComponentCount * bits.mComponentByteCount;
      if( MemCmp( oldBits, newBits, componentTotalSize ) )
        bitfield |= 1 << i;
    }
    return NetBitDiff{ bitfield };
  }

  void LagTest::SaveMessage( const Vector< char >& data, Timestamp elapsedSecs )
  {
    const TimestampDifference lagSimSecs = mLagSimulationMS * 0.001f;
    const Timestamp delayedTillSecs = elapsedSecs + lagSimSecs;
    const DelayedNetMsg delayedNetMsg
    {
      .mDelayedTillSecs = delayedTillSecs,
      .mData = data,
    };
    mSavedNetworkMessages.push_back( delayedNetMsg );
  }

  bool LagTest::TryPopSavedMessage( Vector< char >& data, Timestamp elapsedSecs )
  {
    if( mSavedNetworkMessages.empty() )
      return false;

    auto savedNetMsg = mSavedNetworkMessages.front();
    if( elapsedSecs < savedNetMsg.mDelayedTillSecs )
      return false;

    data = savedNetMsg.mData;
    mSavedNetworkMessages.pop_front();
    return true;
  }


  SnapshotBuffer::~SnapshotBuffer()
  {
    for( World* world : mSnapshots )
      delete world;
  }

  void SnapshotBuffer::AddSnapshot( const World* world )
  {
    World* snapshot = nullptr;
    if( (int)mSnapshots.size() == maxSnapshots )
    {
      snapshot = mSnapshots.front();
      mSnapshots.pop_front();
    }

    if( !snapshot )
      snapshot = TAC_NEW World;

    snapshot->DeepCopy( *world );
    mSnapshots.push_back( snapshot );
  }

  World* SnapshotBuffer::FindSnapshot( Timestamp elapsedGameSecs )
  {
    for( World* world : mSnapshots )
      if( world->mElapsedSecs == elapsedGameSecs )
        return world;
    return nullptr;
  }

} // namespace Tac

