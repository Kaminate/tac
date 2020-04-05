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

NetMsgType ReadNetMsgHeader( Reader* reader, Errors& errors )
{
  for( char c:tac )
  {
    char l;
    if( !reader->Read( &l ) )
    {
      errors = "fuck";
      return NetMsgType::Count;
    }
    if( l != c )
    {
      errors = "fuck";
      return NetMsgType::Count;
    }
  }
  NetMsgType result;
  if( !reader->Read( &result ) )
  {
    errors = "fuck";
    return NetMsgType::Count;
  }
  return result;
}

uint8_t GetNetworkBitfield(
  void* oldData,
  void* newData,
  const Vector< NetworkBit >& networkBits )
{
  if( !oldData )
    return 0xff;
  uint8_t bitfield = 0;
  for( int i = 0; i < networkBits.size(); ++i )
  {
    auto bits = networkBits[ i ];
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
  if( mSnapshots.size() == maxSnapshots )
  {
    delete mSnapshots.front();
    mSnapshots.pop_front();
  }
  auto snapshot = new World();
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

