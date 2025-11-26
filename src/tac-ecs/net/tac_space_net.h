#pragma once

#include "tac-std-lib/dataprocess/tac_serialization.h" // Endianness
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-std-lib/containers/tac_list.h"

#include "tac-ecs/tac_space.h"

namespace Tac
{
  const Endianness GameEndianness { Endianness::Little };
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
    GameTime      mDelayedTillSecs {};
    Vector< char > mData            {};
  };

  struct LagTest
  {
    void                       SaveMessage( const Vector< char >& data, GameTime elapsedSecs );
    bool                       TryPopSavedMessage( Vector< char >& data, GameTime elapsedSecs );

    int                        mLagSimulationMS      {};
    List< DelayedNetMsg >      mSavedNetworkMessages {};
  };

  struct SnapshotBuffer
  {
    ~SnapshotBuffer();
    void                AddSnapshot( const World* );
    World*              FindSnapshot( GameTime elapsedGameSecs );

    List< World* >      mSnapshots   {};
    const int           maxSnapshots { 32 };
  };

} // namespace Tac 

