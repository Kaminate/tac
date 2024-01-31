#include "space/net/tac_space_net.h"
#include "space/world/tac_world.h"
#include "src/common/error/tac_error_handling.h"

namespace Tac
{

  const String tac( "tac" );

  void WriteNetMsgHeader( Writer* writer, NetMsgType networkMessageType )
  {
    writer->Write( tac.data(), tac.size(), 1 );
    writer->Write( networkMessageType );
  }

  void ReadNetMsgHeader( Reader* reader, NetMsgType* netMsgType, Errors& errors )
  {
    for( char cExpected : tac )
    {
      char cActual;
      TAC_RAISE_ERROR_IF( !reader->Read( &cActual ), "failed reading net msg header tac" );

      TAC_RAISE_ERROR_IF( cExpected != cActual, "mismatchg reading net msg header tac" );
    }

    TAC_RAISE_ERROR_IF( !reader->Read(netMsgType),  "failure reading NetMsgType" );
  }

  u8 GetNetworkBitfield( const void* oldData,
                              const void* newData,
                              const NetworkBits& networkBits )
  {
    if( !oldData )
      return 0xff;

    u8 bitfield = 0;
    for( int i = 0; i < networkBits.size(); ++i )
    {
      const NetworkBit& bits = networkBits[ i ];
      auto oldBits = ( char* )oldData + bits.mByteOffset;
      auto newBits = ( char* )newData + bits.mByteOffset;
      auto componentTotalSize = bits.mComponentCount * bits.mComponentByteCount;
      if( std::memcmp( oldBits, newBits, componentTotalSize ) )
        bitfield |= 1 << i;
    }
    return bitfield;
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

