#pragma once
#include "common/tacSerialization.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"

#include <list>
#include <cstdint>

struct TacWorld;
const TacEndianness TacGameEndianness = TacEndianness::Little;
enum class TacNetMsgType : char
{
  Text,
  Snapshot,
  Input,
  Matchmaking,
  Read,

  Count,
};

void TacWriteNetMsgHeader( TacWriter* writer, TacNetMsgType networkMessageType );
TacNetMsgType TacReadNetMsgHeader( TacReader* reader, TacErrors& errors );

uint8_t GetNetworkBitfield(
  void* oldData,
  void* newData,
  const TacVector< TacNetworkBit >& networkBits );

struct TacDelayedNetMsg
{
  double mDelayedTillSecs = 0;
  TacVector< char > mData;
};

struct TacLagTest
{
  void SaveMessage( const TacVector< char >& data, double elapsedSecs );
  bool TryPopSavedMessage( TacVector< char >& data, double elapsedSecs );
  int mLagSimulationMS = 0;
  std::list< TacDelayedNetMsg > mSavedNetworkMessages;
};
struct TacSnapshotBuffer
{
  ~TacSnapshotBuffer();
  void AddSnapshot( const TacWorld* world );
  TacWorld* FindSnapshot( double elapsedGameSecs );
  std::list< TacWorld* > mSnapshots;
  const int maxSnapshots = 32;
};
