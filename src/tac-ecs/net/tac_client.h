#pragma once

#include "tac-std-lib/math/tac_vector2.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/tac_std_lib.h" // ReadStream, WriteStream

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/net/tac_space_net.h"
#include "tac-ecs/world/tac_world.h"

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

  //struct Reader;
  //struct Writer;


  struct ClientData
  {
    void                    ReadSnapshotBody( ReadStream*, Errors& );
    void                    OnClientDisconnect();
    void                    WriteInputBody( WriteStream* );
    void                    ExecuteNetMsg( const void*, int, Errors& );
    void                    ApplyPrediction( Timestamp lastTime );
    void                    Update( float seconds,
                                    v2 inputDir,
                                    ClientSendNetworkMessageCallback sendNetworkMessageCallback,
                                    void* userData,
                                    Errors& );
    void                    ReceiveMessage( void* bytes,
                                            int byteCount,
                                            Errors& );

    LagTest                 mSavedNetworkMessages   {};
    World*                  mWorld                  {};
    World*                  mEmptyWorld             {};
    PlayerUUID              mPlayerUUID             { NullPlayerUUID };

    // < Prediction >
    static const int        sMaxSavedInputCount     { 60 };
    List< SavedInput >      mSavedInputs            {};
    bool                    mIsPredicting           { true };
    // </>

    Timestamp               mMostRecentSnapshotTime {};
    SnapshotBuffer          mSnapshots              {};
  };

}

