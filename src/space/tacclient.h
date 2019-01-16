#pragma once

#include "tacspacetypes.h"
#include "tacspacenet.h"
#include "tacworld.h"

#include "common/math/tacVector2.h"
#include "common/containers/tacVector.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"

#include <list>

struct TacSavedInput
{
  double mTimestamp;
  v2 mInputDirection;
};

typedef void( *ClientSendNetworkMessageCallback )(
  void* bytes,
  int byteCount,
  void* userData );

struct TacReader;
struct TacWriter;


struct TacClientData
{
  TacLagTest mSavedNetworkMessages;

  //TacChat mChat;
  TacWorld* mWorld = nullptr;
  TacWorld* mEmptyWorld = nullptr;
  TacPlayerUUID mPlayerUUID = TacNullPlayerUUID;

  // < Prediction >
  static const int sMaxSavedInputCount = 60;
  std::list< TacSavedInput > mSavedInputs;
  bool mIsPredicting = true;
  // </>

  // --------------------------------------------

  double mMostRecentSnapshotTime = 0;
  TacSnapshotBuffer mSnapshots;

  void TacReadSnapshotBody(
    TacReader* reader,
    TacErrors& errors );
  void TacOnClientDisconnect();
  void WriteInputBody( TacWriter* writer );
  void ExecuteNetMsg( void* bytes, int byteCount, TacErrors& errors );
  void ApplyPrediction( double lastTime );
  void ReadEntityDifferences( TacReader* reader, TacErrors& errors );
  void ReadPlayerDifferences( TacReader* reader, TacErrors& errors );
  void Update(
    float seconds,
    v2 inputDir,
    ClientSendNetworkMessageCallback sendNetworkMessageCallback,
    void* userData,
    TacErrors& errors );
  void ReceiveMessage(
    void* bytes,
    int byteCount,
    TacErrors& errors );
};
