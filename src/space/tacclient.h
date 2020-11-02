
#pragma once

#include "src/space/tacSpacetypes.h"
#include "src/space/tacSpacenet.h"
#include "src/space/tacWorld.h"
#include "src/common/math/tacVector2.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"

#include <list>

namespace Tac
{
  struct SavedInput
  {
    double mTimestamp;
    v2     mInputDirection;
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

    double                  mMostRecentSnapshotTime = 0;
    SnapshotBuffer          mSnapshots;
    void                    ReadSnapshotBody( Reader*, Errors& );
    void                    OnClientDisconnect();
    void                    WriteInputBody( Writer* );
    void                    ExecuteNetMsg( void* bytes, int byteCount, Errors& );
    void                    ApplyPrediction( double lastTime );
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

