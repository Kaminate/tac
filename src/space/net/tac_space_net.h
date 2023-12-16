#pragma once

#include "src/common/dataprocess/tac_serialization.h" // Endianness
#include "space/tac_space.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/tac_core.h"

import std; // <list>, <cstdint>(uint8_t)
using std::uint8_t;

namespace Tac
{
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
  void ReadNetMsgHeader( Reader*, NetMsgType*, Errors& );

  uint8_t GetNetworkBitfield( const void* oldData,
                              const void* newData,
                              const NetworkBits& networkBits );

  struct DelayedNetMsg
  {
    Timestamp      mDelayedTillSecs;
    Vector< char > mData;
  };

  struct LagTest
  {
    void                       SaveMessage( const Vector< char >& data, Timestamp elapsedSecs );
    bool                       TryPopSavedMessage( Vector< char >& data, Timestamp elapsedSecs );
    int                        mLagSimulationMS = 0;
    std::list< DelayedNetMsg > mSavedNetworkMessages;
  };

  struct SnapshotBuffer
  {
    ~SnapshotBuffer();
    void                AddSnapshot( const World* );
    World*              FindSnapshot( Timestamp elapsedGameSecs );
    std::list< World* > mSnapshots;
    const int           maxSnapshots = 32;
  };

}

