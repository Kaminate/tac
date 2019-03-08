#include "common/assetmanagers/tacTextureAssetManager.h"
#include "common/tacMemory.h"
#include "common/tacRenderer.h"
#include "common/tacJobQueue.h"
#include "common/thirdparty/stb_image.h"

struct TacAsyncTextureData : public TacJob
{
  void Execute() override;
  TacImage mImage;
  TacVector< char > mImageData;
  TacString mFilepath;
};

void TacAsyncTextureData::Execute()
{
  TacErrors& errors = mErrors;


  auto memory = TacTemporaryMemory( mFilepath, errors );
  TAC_HANDLE_ERROR( errors );

  int x;
  int y;
  int previousChannelCount;
  int desiredChannelCount = 4;

  // rgba
  stbi_uc* loaded = stbi_load_from_memory(
    ( const stbi_uc* )memory.data(),
    ( int )memory.size(),
    &x,
    &y,
    &previousChannelCount,
    desiredChannelCount );
  OnDestruct( stbi_image_free( loaded ) );

  bool shouldConvertToPremultipliedAlpha = true;
  if( shouldConvertToPremultipliedAlpha )
  {
    stbi_uc* l = loaded;
    for( int i = 0; i < y; ++i )
    {
      for( int j = 0; j < x; ++j )
      {
        uint8_t* r = l++;
        uint8_t* g = l++;
        uint8_t* b = l++;
        uint8_t* a = l++;
        float percent = *a / 255.0f;
        *r = ( uint8_t )( *r * percent );
        *g = ( uint8_t )( *g * percent );
        *b = ( uint8_t )( *b * percent );
      }
    }
  }

  TacFormat format;
  format.mElementCount = desiredChannelCount;
  format.mPerElementByteCount = 1;
  format.mPerElementDataType = TacGraphicsType::unorm;

  int pitch = x * format.mElementCount * format.mPerElementByteCount;
  int imageDataByteCount = y * pitch;
  mImageData.resize( imageDataByteCount );
  TacMemCpy( mImageData.data(), loaded, imageDataByteCount );

  TacImage& image = mImage;
  image.mData = mImageData.data();
  image.mFormat = format;
  image.mWidth = x;
  image.mHeight = y;
  image.mPitch = pitch;
}

void TacTextureAssetManager::GetTexture( TacTexture** ppTexture, const TacString& textureFilepath, TacErrors& errors )
{
  if( ( *ppTexture = mLoadedTextureMap[ textureFilepath ] ) )
    return;

  TacAsyncTextureData* asyncTextureData = nullptr;
  for( TacAsyncTextureData* asyncLoadData : mLoadingTextures )
  {
    if( asyncLoadData->mFilepath != textureFilepath )
      continue;
    asyncTextureData = ( TacAsyncTextureData* )asyncLoadData;
    break;
  }
  if( !asyncTextureData )
  {
    asyncTextureData = new TacAsyncTextureData;
    asyncTextureData->mFilepath = textureFilepath;
    mLoadingTextures.insert( asyncTextureData );
    mJobQueue->Push( asyncTextureData );
    return;
  }
  TacAsyncLoadStatus status = asyncTextureData->GetStatus();
  switch( status )
  {
  case TacAsyncLoadStatus::ThreadQueued: // fallthrough
  case TacAsyncLoadStatus::ThreadRunning: break;
  case TacAsyncLoadStatus::ThreadFailed: {
    errors = asyncTextureData->mErrors;
    TAC_HANDLE_ERROR( errors );
  } break;
  case TacAsyncLoadStatus::ThreadCompleted: {
    TacTextureData textureData;
    textureData.mName = textureFilepath;
    textureData.mStackFrame = TAC_STACK_FRAME;
    textureData.myImage = asyncTextureData->mImage;
    textureData.binding = { TacBinding::ShaderResource };
    mRenderer->AddTextureResource( ppTexture, textureData, errors );
    TAC_HANDLE_ERROR( errors );
    mLoadingTextures.erase( asyncTextureData );
    delete asyncTextureData;
    mLoadedTextureMap[ textureFilepath ] = *ppTexture;
    break;
  }
                                            TacInvalidDefaultCase( status );
  }

}
