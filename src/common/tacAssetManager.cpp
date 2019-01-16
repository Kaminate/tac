#include "tacAssetManager.h"

//#include "tacAssetManager.h"
//#include "core/taccommon.h"
//#include "tacstb.h"
//#include "collada_parser.h"
//
//#if TAC_USING_ASSIMP
//#include "tacAssimp.h"
//#endif

//void TacGenericAsyncCallback( void* data )
//{
//  auto asyncData = ( TacAsyncLoadData* )data;
//  asyncData->SetStatus( TacAsyncLoadStatus::ThreadRunning );
//  asyncData->DoWork( asyncData->mErrors );
//  asyncData->SetStatus( asyncData->mErrors.empty() ? TacAsyncLoadStatus::ThreadCompleted : TacAsyncLoadStatus::ThreadFailed );
//}

static float mMinimumLoadSeconds = 0;

//TacAsyncLoadData::TacAsyncLoadData()
//{
//}


//TacAssetManager::~TacAssetManager()
//{
//}
//
//void TacAssetManager::Add( TacAsyncLoadData* asyncLoadData )
//{
//  mAsyncLoadDatas.insert( asyncLoadData );
//}
//void TacAssetManager::Remove( TacAsyncLoadData* asyncLoadData)
//{
//  mAsyncLoadDatas.erase( asyncLoadData );
//}
