
#pragma once
#include "src/common/tacSerialization.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include <list>
#include <cstdint>
namespace Tac
{

  struct World;
  const Endianness GameEndianness = Endianness::Little;
  enum class NetMsgType : char
  {
    Text,
    Snapshot,
    Input,
    Matchmaking,
    Read,

    Count,
  };

  void WriteNetMsgHeader( Writer*, NetMsgType );
  void ReadNetMsgHeader( Reader*, 
                               NetMsgType*,
                               Errors& );

  uint8_t GetNetworkBitfield( const void* oldData,
                              const void* newData,
                              const Vector< NetworkBit >& networkBits );

  struct DelayedNetMsg
  {
    double         mDelayedTillSecs = 0;
    Vector< char > mData;
  };

  struct LagTest
  {
    void                       SaveMessage( const Vector< char >& data, double elapsedSecs );
    bool                       TryPopSavedMessage( Vector< char >& data, double elapsedSecs );
    int                        mLagSimulationMS = 0;
    std::list< DelayedNetMsg > mSavedNetworkMessages;
  };

  struct SnapshotBuffer
  {
    ~SnapshotBuffer();
    void                AddSnapshot( const World* );
    World*              FindSnapshot( double elapsedGameSecs );
    std::list< World* > mSnapshots;
    const int           maxSnapshots = 32;
  };

}

