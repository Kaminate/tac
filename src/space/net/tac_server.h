#pragma once

#include "space/tac_space_types.h"
#include "space/net/tac_space_net.h"
#include "space/tac_space.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/containers/tac_list.h"
#include "src/common/shell/tac_shell_timer.h"

namespace Tac
{
  struct OtherPlayer
  {
    // A unique ID per players
    // ( both client players and server player ).
    // Generated by the server
    PlayerUUID     mPlayerUUID = NullPlayerUUID;
    // Last acknowledged
    Timestamp      mTimeStamp;
    LagTest        delayedNetMsg;
    ConnectionUUID mConnectionUUID = NullConnectionUUID;
  };

  typedef void( *ServerSendNetworkMessageCallback )( ConnectionUUID,
                                                     const void* bytes,
                                                     int byteCount,
                                                     void* userData );

  struct ServerData
  {
    ServerData();
    ~ServerData();

    Entity*                   SpawnEntity();
    Player*                   SpawnPlayer();

    void                      OnClientJoin( ConnectionUUID );

    void                      DebugImgui();

    OtherPlayer*              FindOtherPlayer( ConnectionUUID );

    void                      ReceiveMessage( ConnectionUUID,
                                              void* bytes,
                                              int byteCount,
                                              Errors& );

    void                      Update( float seconds,
                                      ServerSendNetworkMessageCallback,
                                      void* userData,
                                      Errors& );

    void                      OnLoseClient( ConnectionUUID );

    void                      ReadInput( Reader*,
                                         ConnectionUUID,
                                         Errors& );

    void                      WriteSnapshotBody( OtherPlayer*, Writer* );

    void                      ExecuteNetMsg( ConnectionUUID,
                                             const void* bytes,
                                             int byteCount,
                                             Errors& );
    SnapshotBuffer            mSnapshots;
    float                     mSnapshotUntilNextSecondsCur = 0;
    World*                    mWorld = nullptr;
    World*                    mEmptyWorld = nullptr;
    static const int          sOtherPlayerCountMax = 3;
    List< OtherPlayer* >      mOtherPlayers;
    //PlayerUUID                mPlayerUUIDCounter = NullPlayerUUID;
    //EntityUUID                mEntityUUIDCounter = NullEntityUUID;
    PlayerUUIDCounter         mPlayerUUIDCounter;
    EntityUUIDCounter         mEntityUUIDCounter;
  };

  extern const float sSnapshotUntilNextSecondsMax;
}

