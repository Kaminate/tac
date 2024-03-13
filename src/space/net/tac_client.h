#pragma once

#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/shell/tac_shell_timestep.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"

#include "space/tac_space_types.h"
#include "space/net/tac_space_net.h"
#include "space/world/tac_world.h"

namespace Tac
{
  struct SavedInput
  {
    Timestamp mTimestamp;
    v2        mInputDirection{};
  };

  typedef void( *ClientSendNetworkMessageCallback )( void* bytes,
                                                     int byteCount,
                                                     void* userData );

  struct Reader;
  struct Writer;


  struct ClientData
  {
    LagTest                 mSavedNetworkMessages;
    World*                  mWorld = nullptr;
    World*                  mEmptyWorld = nullptr;
    PlayerUUID              mPlayerUUID = NullPlayerUUID;

    // < Prediction >
    static const int        sMaxSavedInputCount = 60;
    List< SavedInput >      mSavedInputs;
    bool                    mIsPredicting = true;
    // </>

    Timestamp               mMostRecentSnapshotTime;
    SnapshotBuffer          mSnapshots;
    void                    ReadSnapshotBody( Reader*, Errors& );
    void                    OnClientDisconnect();
    void                    WriteInputBody( Writer* );
    void                    ExecuteNetMsg( void* bytes, int byteCount, Errors& );
    void                    ApplyPrediction( Timestamp lastTime );
    void                    Update( float seconds,
                                    v2 inputDir,
                                    ClientSendNetworkMessageCallback sendNetworkMessageCallback,
                                    void* userData,
                                    Errors& );
    void                    ReceiveMessage( void* bytes,
                                            int byteCount,
                                            Errors& );
  };

}

