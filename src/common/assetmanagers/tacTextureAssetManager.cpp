#include "common/assetmanagers/tacTextureAssetManager.h"
#include "common/tacMemory.h"
#include "common/tacAlgorithm.h"
#include "common/tacOS.h"
#include "common/tacUtility.h"
#include "common/graphics/tacRenderer.h"
#include "common/tacJobQueue.h"
#include "common/thirdparty/stb_image.h"

struct TacAsyncTextureData
{
  virtual ~TacAsyncTextureData() = default;
  virtual void CreateTexture( TacRenderer* renderer, TacTexture** texture, TacErrors& errors ) = 0;
};

struct TacAsyncTextureSingleData : TacAsyncTextureData
{
  void CreateTexture( TacRenderer* renderer, TacTexture** texture, TacErrors& errors ) override;
  TacImage mImage;
  TacVector< char > mImageData;
  TacString mFilepath;
};
void TacAsyncTextureSingleData::CreateTexture(
  TacRenderer* renderer,
  TacTexture** texture,
  TacErrors& errors )
{
  TacTextureData textureData;
  textureData.mName = mFilepath;
  textureData.mStackFrame = TAC_STACK_FRAME;
  textureData.myImage = mImage;
  textureData.binding = { TacBinding::ShaderResource };
  TacRenderer::Instance->AddTextureResource( texture, textureData, errors );
  TAC_HANDLE_ERROR( errors );
}

struct TacAsyncTextureCubeData : TacAsyncTextureData
{
  void CreateTexture( TacRenderer* renderer, TacTexture** texture, TacErrors& errors ) override;
  TacImage mImage;
  TacVector< char > mImageData[ 6 ];
  TacString mDir;
};
void TacAsyncTextureCubeData::CreateTexture(
  TacRenderer* renderer,
  TacTexture** texture,
  TacErrors& errors )
{
  void* cubedatas[ 6 ];
  for( int i = 0; i < 6; ++i )
    cubedatas[ i ] = mImageData[ i ].data();

  TacTextureData textureData;
  textureData.mName = mDir;
  textureData.mStackFrame = TAC_STACK_FRAME;
  textureData.myImage = mImage;
  textureData.binding = { TacBinding::ShaderResource };
  TacRenderer::Instance->AddTextureResourceCube( texture, textureData, cubedatas, errors );
  TAC_HANDLE_ERROR( errors );
}

struct TacAsyncTexture
{
  TacJob* mJob = nullptr;
  TacAsyncTextureData* mData = nullptr;
};

struct TacAsyncTextureSingleJob : TacJob
{
  void Execute() override;
  TacAsyncTextureSingleData* mData = nullptr;
};
void TacAsyncTextureSingleJob::Execute()
{
  TacErrors& errors = mErrors;

  auto memory = TacTemporaryMemory( mData->mFilepath, errors );
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
  mData->mImageData.resize( imageDataByteCount );
  TacMemCpy( mData->mImageData.data(), loaded, imageDataByteCount );

  TacImage& image = mData->mImage;
  image.mData = mData->mImageData.data();
  image.mFormat = format;
  image.mWidth = x;
  image.mHeight = y;
  image.mPitch = pitch;
}

struct TacAsyncTextureCubeJob : TacJob
{
  void Execute() override;
  TacAsyncTextureCubeData* mData = nullptr;
};
void TacAsyncTextureCubeJob::Execute()
{
  TacErrors& errors = mErrors;

  TacVector< TacString > files;
  TacOS::Instance->GetDirFilesRecursive( files, mData->mDir, errors );
  TAC_HANDLE_ERROR( errors );

  if( files.size() != 6 )
  {
    errors = "found " + TacToString( files.size() ) + " textures in " + mData->mDir;
    TAC_HANDLE_ERROR( errors );
  }

  auto TrySortPart = [ & ]( const TacString& face, int desiredIndex )
  {
    for( int i = 0; i < 6; ++i )
    {
      TacString filepath = files[ i ];
      if( TacToLower( filepath ).find( TacToLower( face ) ) == TacString::npos )
        continue;
      TacSwap( files[ i ], files[ desiredIndex ] );
      break;
    }
  };

  // sort (x, -x, y, -y, z, -z)?
  TrySortPart( "Right", 0 );
  TrySortPart( "Left", 1 );
  TrySortPart( "Top", 2 );
  TrySortPart( "Bottom", 3 );
  TrySortPart( "Front", 4 );
  TrySortPart( "Back", 5 );


  TacFormat format;
  format.mElementCount = 4;
  format.mPerElementByteCount = 1;
  format.mPerElementDataType = TacGraphicsType::unorm;


  int prevWidth = 0;
  int prevHeight = 0;
  for( int iFile = 0; iFile < 6; ++iFile )
  {
    const TacString& filepath = files[ iFile ];
    auto memory = TacTemporaryMemory( filepath, errors );
    TAC_HANDLE_ERROR( mErrors );

    int x;
    int y;
    int previousChannelCount;
    // rgba
    stbi_uc* loaded = stbi_load_from_memory(
      ( const stbi_uc* )memory.data(),
      ( int )memory.size(),
      &x,
      &y,
      &previousChannelCount,
      format.mElementCount );
    OnDestruct
    (
      stbi_image_free( loaded );
    prevWidth = x;
    prevHeight = y;
    );

    if( iFile && !( x == prevWidth && y == prevHeight ) )
    {
      const TacString& filepathPrev = files[ iFile - 1 ];
      errors = filepath + " has dimensions " +
        TacToString( x ) + "x" + TacToString( y ) +
        " which is different from " + filepathPrev + " dimensions " +
        TacToString( prevWidth ) + "x" + TacToString( prevHeight );
      TAC_HANDLE_ERROR( errors );
    }

    int pitch = x * format.mElementCount * format.mPerElementByteCount;
    int imageDataByteCount = y * pitch;
    TacVector< char >& imageData = mData->mImageData[ iFile ];
    imageData.resize( imageDataByteCount );
    TacMemCpy( imageData.data(), loaded, imageDataByteCount );
  }

  TacImage& image = mData->mImage;
  image.mFormat = format;
  image.mWidth = prevWidth;
  image.mHeight = prevHeight;
  image.mPitch = prevWidth * format.mElementCount * format.mPerElementByteCount;
}

TacTextureAssetManager* TacTextureAssetManager::Instance = nullptr;
TacTextureAssetManager::TacTextureAssetManager()
{
  Instance = this;
}
TacTextureAssetManager::~TacTextureAssetManager()
{
  for( auto pair : mLoadedTextures )
  {
    TacTexture* texture = pair.second;
    TacRenderer::Instance->RemoveRendererResource( texture );
  }
}
TacTexture* TacTextureAssetManager::FindLoadedTexture( const TacString& key )
{
  auto it = mLoadedTextures.find( key );
  if( it == mLoadedTextures.end() )
    return nullptr;
  return ( *it ).second;
}
TacAsyncTexture* TacTextureAssetManager::FindLoadingTexture( const TacString& key )
{
  auto it = mLoadingTextures.find( key );
  if( it == mLoadingTextures.end() )
    return nullptr;
  return ( *it ).second;
}
void TacTextureAssetManager::GetTextureCube(
  TacTexture** ppTexture,
  const TacString& textureDir,
  TacErrors& errors )
{
  TacTexture* texture = FindLoadedTexture( textureDir );
  if( texture )
  {
    *ppTexture = texture;
    return;
  }

  TacAsyncTexture* asyncTexture = FindLoadingTexture( textureDir );
  if( !asyncTexture )
  {
    auto data = new TacAsyncTextureCubeData;
    data->mDir = textureDir;

    auto job = new TacAsyncTextureCubeJob;
    job->mData = data;

    asyncTexture = new TacAsyncTexture;
    asyncTexture->mJob = job;
    asyncTexture->mData = data;
    mLoadingTextures[ textureDir ] = asyncTexture;
    TacJobQueue::Instance->Push( job );
    *ppTexture = nullptr;
    return;
  }

  UpdateAsyncTexture( ppTexture, textureDir, asyncTexture, errors );
}
void TacTextureAssetManager::UpdateAsyncTexture(
  TacTexture** ppTexture,
  const TacString& key,
  TacAsyncTexture* asyncTexture,
  TacErrors& errors )
{
  TacJob* job = asyncTexture->mJob;
  TacAsyncLoadStatus status = job->GetStatus();
  switch( status )
  {
    case TacAsyncLoadStatus::ThreadQueued:
    {
      *ppTexture = nullptr;
    } break;
    case TacAsyncLoadStatus::ThreadRunning:
    {
      *ppTexture = nullptr;
    } break;
    case TacAsyncLoadStatus::ThreadFailed:
    {
      *ppTexture = nullptr;
      errors = job->mErrors;
      TAC_HANDLE_ERROR( errors );
    } break;
    case TacAsyncLoadStatus::ThreadCompleted:
    {
      asyncTexture->mData->CreateTexture( TacRenderer::Instance, ppTexture, errors );
      TAC_HANDLE_ERROR( errors );
      mLoadingTextures.erase( key );
      delete asyncTexture->mData;
      delete asyncTexture->mJob;
      delete asyncTexture;
      mLoadedTextures[ key ] = *ppTexture;
      break;
    }
    TacInvalidDefaultCase( status );
  }
}
void TacTextureAssetManager::GetTexture(
  TacTexture** ppTexture,
  const TacString& textureFilepath,
  TacErrors& errors )
{
  TacTexture* texture = FindLoadedTexture( textureFilepath );
  if( texture )
  {
    *ppTexture = texture;
    return;
  }

  TacAsyncTexture* asyncTexture = FindLoadingTexture( textureFilepath );
  if( !asyncTexture )
  {
    auto data = new TacAsyncTextureSingleData;
    data->mFilepath = textureFilepath;

    auto job = new TacAsyncTextureSingleJob;
    job->mData = data;

    asyncTexture = new TacAsyncTexture;
    asyncTexture->mData = data;
    asyncTexture->mJob = job;

    mLoadingTextures[ textureFilepath ] = asyncTexture;
    TacJobQueue::Instance->Push( job );
    *ppTexture = nullptr;
    return;
  }

  UpdateAsyncTexture( ppTexture, textureFilepath, asyncTexture, errors );
}
