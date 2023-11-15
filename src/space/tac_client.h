
#pragma once

#include "src/space/tac_space_types.h"
#include "src/space/tac_space_net.h"
#include "src/space/tac_world.h"
#include "src/common/math/tac_vector2.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/string/tac_string.h"
#include "src/common/core/tac_error_handling.h"

import std; // #include <list>

namespace Tac
{
  struct SavedInput
  {
    Timestamp mTimestamp;
    v2        mInputDirection;
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
    std::list< SavedInput > mSavedInputs;
    bool                    mIsPredicting = true;
    // </>

    Timestamp               mMostRecentSnapshotTime;
    SnapshotBuffer          mSnapshots;
    void                    ReadSnapshotBody( Reader*, Errors& );
    void                    OnClientDisconnect();
    void                    WriteInputBody( Writer* );
    void                    ExecuteNetMsg( void* bytes, int byteCount, Errors& );
    void                    ApplyPrediction( Timestamp lastTime );
    void                    ReadEntityDifferences( Reader*, Errors& );
    void                    ReadPlayerDifferences( Reader*, Errors& );
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

