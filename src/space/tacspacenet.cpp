#include <cstring>
#include "tacspacenet.h"
#include "tacworld.h"

const TacString tac( "tac" );

void TacWriteNetMsgHeader( TacWriter* writer, TacNetMsgType networkMessageType )
{
  writer->Write( tac.data(), ( int )tac.size(), 1 );
  writer->Write( networkMessageType );
}

TacNetMsgType TacReadNetMsgHeader( TacReader* reader, TacErrors& errors )
{
  for( char c:tac )
  {
    char l;
    if( !reader->Read( &l ) )
    {
      errors = "fuck";
      return TacNetMsgType::Count;
    }
    if( l != c )
    {
      errors = "fuck";
      return TacNetMsgType::Count;
    }
  }
  TacNetMsgType result;
  if( !reader->Read( &result ) )
  {
    errors = "fuck";
    return TacNetMsgType::Count;
  }
  return result;
}

uint8_t GetNetworkBitfield(
  void* oldData,
  void* newData,
  const TacVector< TacNetworkBit >& networkBits )
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

void TacLagTest::SaveMessage( const TacVector< char >& data, double elapsedSecs )
{
  TacDelayedNetMsg delayedNetMsg;
  delayedNetMsg.mData = data;
  delayedNetMsg.mDelayedTillSecs = elapsedSecs + mLagSimulationMS * 0.001f;
  mSavedNetworkMessages.push_back( delayedNetMsg );
}
bool TacLagTest::TryPopSavedMessage( TacVector< char >& data, double elapsedSecs )
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


TacSnapshotBuffer::~TacSnapshotBuffer()
{
  for( auto world : mSnapshots )
    delete world;
}
void TacSnapshotBuffer::AddSnapshot( const TacWorld* world )
{
  if( mSnapshots.size() == maxSnapshots )
  {
    delete mSnapshots.front();
    mSnapshots.pop_front();
  }
  auto snapshot = new TacWorld();
  snapshot->DeepCopy( *world );
  mSnapshots.push_back( snapshot );
}
TacWorld* TacSnapshotBuffer::FindSnapshot( double elapsedGameSecs )
{
  for( auto world : mSnapshots )
    if( world->mElapsedSecs == elapsedGameSecs )
      return world;
  return nullptr;
}
