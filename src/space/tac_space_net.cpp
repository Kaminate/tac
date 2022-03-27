#include "src/space/tacSpacenet.h"
#include "src/space/tacWorld.h"
#include <cstring>
namespace Tac
{

  const String tac( "tac" );

  void WriteNetMsgHeader( Writer* writer, NetMsgType networkMessageType )
  {
    writer->Write( tac.data(), ( int )tac.size(), 1 );
    writer->Write( networkMessageType );
  }

  void ReadNetMsgHeader( Reader* reader, NetMsgType* netMsgType, Errors& errors )
  {
    for( char c : tac )
    {
      char l;
      if( !reader->Read( &l ) )
        TAC_RAISE_ERROR( "failed reading net msg header tac", errors );
      if( l != c )
        TAC_RAISE_ERROR( "mismatchg reading net msg header tac", errors );
    }
    if( !reader->Read(netMsgType) )
      TAC_RAISE_ERROR( "failure reading NetMsgType", errors );
  }

  uint8_t GetNetworkBitfield( const void* oldData,
                              const void* newData,
                              const NetworkBits& networkBits )
  {
    if( !oldData )
      return 0xff;
    uint8_t bitfield = 0;
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

  void LagTest::SaveMessage( const Vector< char >& data, double elapsedSecs )
  {
    DelayedNetMsg delayedNetMsg;
    delayedNetMsg.mData = data;
    delayedNetMsg.mDelayedTillSecs = elapsedSecs + mLagSimulationMS * 0.001f;
    mSavedNetworkMessages.push_back( delayedNetMsg );
  }
  bool LagTest::TryPopSavedMessage( Vector< char >& data, double elapsedSecs )
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
    for( auto world : mSnapshots )
      delete world;
  }
  void SnapshotBuffer::AddSnapshot( const World* world )
  {
    World* snapshot = nullptr;
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
  World* SnapshotBuffer::FindSnapshot( double elapsedGameSecs )
  {
    for( auto world : mSnapshots )
      if( world->mElapsedSecs == elapsedGameSecs )
        return world;
    return nullptr;
  }

}
