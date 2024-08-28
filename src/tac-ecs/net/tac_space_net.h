#pragma once

#include "tac-std-lib/dataprocess/tac_serialization.h" // Endianness
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/containers/tac_list.h"

#include "tac-ecs/tac_space.h"

namespace Tac
{
  const Endianness GameEndianness = Endianness::Little;
  enum class NetMsgType : u8
  {
    Text,
    Snapshot,
    Input,
    Matchmaking,
    Read,

    Count,
  };

  void       WriteNetMsgHeader( WriteStream*, NetMsgType );
  NetMsgType ReadNetMsgHeader( ReadStream*, Errors& );


  //NetBitDiff GetNetVarfield( const void* oldData,
  //                               const void* newData,
  //                               const NetVars& networkBits );

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
    List< DelayedNetMsg >      mSavedNetworkMessages;
  };

  struct SnapshotBuffer
  {
    ~SnapshotBuffer();
    void                AddSnapshot( const World* );
    World*              FindSnapshot( Timestamp elapsedGameSecs );
    List< World* >      mSnapshots;
    const int           maxSnapshots { 32 };
  };

}

