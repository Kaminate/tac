
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/tacMemory.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacOS.h"
#include "src/common/tacUtility.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacJobQueue.h"
#include "src/common/thirdparty/stb_image.h"

namespace Tac
{

struct AsyncTextureData
{
  virtual ~AsyncTextureData() = default;
  virtual void CreateTexture( Texture** texture, Errors& errors ) = 0;
};

struct AsyncTextureSingleData : AsyncTextureData
{
  void CreateTexture( Texture** texture, Errors& errors ) override;
  Image mImage;
  Vector< char > mImageData;
  String mFilepath;
};
void AsyncTextureSingleData::CreateTexture(
  Texture** texture,
  Errors& errors )
{
  TextureData textureData;
  textureData.mName = mFilepath;
  textureData.mFrame = TAC_STACK_FRAME;
  textureData.myImage = mImage;
  textureData.binding = { Binding::ShaderResource };
  Renderer::Instance->AddTextureResource( texture, textureData, errors );
  TAC_HANDLE_ERROR( errors );
}

struct AsyncTextureCubeData : AsyncTextureData
{
  void CreateTexture(  Texture** texture, Errors& errors ) override;
  Image mImage;
  Vector< char > mImageData[ 6 ];
  String mDir;
};
void AsyncTextureCubeData::CreateTexture(
  Texture** texture,
  Errors& errors )
{
  void* cubedatas[ 6 ];
  for( int i = 0; i < 6; ++i )
    cubedatas[ i ] = mImageData[ i ].data();

  TextureData textureData;
  textureData.mName = mDir;
  textureData.mFrame = TAC_STACK_FRAME;
  textureData.myImage = mImage;
  textureData.binding = { Binding::ShaderResource };
  Renderer::Instance->AddTextureResourceCube( texture, textureData, cubedatas, errors );
  TAC_HANDLE_ERROR( errors );
}

struct AsyncTexture
{
  Job* mJob = nullptr;
  AsyncTextureData* mData = nullptr;
};

struct AsyncTextureSingleJob : Job
{
  void Execute() override;
  AsyncTextureSingleData* mData = nullptr;
};
void AsyncTextureSingleJob::Execute()
{
  Errors& errors = mErrors;

  auto memory = TemporaryMemoryFromFile( mData->mFilepath, errors );
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
  TAC_ON_DESTRUCT( stbi_image_free( loaded ) );

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

  Format format;
  format.mElementCount = desiredChannelCount;
  format.mPerElementByteCount = 1;
  format.mPerElementDataType = GraphicsType::unorm;

  int pitch = x * format.mElementCount * format.mPerElementByteCount;
  int imageDataByteCount = y * pitch;
  mData->mImageData.resize( imageDataByteCount );
  MemCpy( mData->mImageData.data(), loaded, imageDataByteCount );

  Image& image = mData->mImage;
  image.mData = mData->mImageData.data();
  image.mFormat = format;
  image.mWidth = x;
  image.mHeight = y;
  image.mPitch = pitch;
}

struct AsyncTextureCubeJob : Job
{
  void Execute() override;
  AsyncTextureCubeData* mData = nullptr;
};
void AsyncTextureCubeJob::Execute()
{
  Errors& errors = mErrors;

  Vector< String > files;
  OS::GetDirFilesRecursive( files, mData->mDir, errors );
  TAC_HANDLE_ERROR( errors );

  if( files.size() != 6 )
  {
    errors = "found " + ToString( files.size() ) + " textures in " + mData->mDir;
    TAC_HANDLE_ERROR( errors );
  }

  auto TrySortPart = [ & ]( const String& face, int desiredIndex )
  {
    for( int i = 0; i < 6; ++i )
    {
      String filepath = files[ i ];
      if( ToLower( filepath ).find( ToLower( face ) ) == String::npos )
        continue;
      Swap( files[ i ], files[ desiredIndex ] );
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


  Format format;
  format.mElementCount = 4;
  format.mPerElementByteCount = 1;
  format.mPerElementDataType = GraphicsType::unorm;


  int prevWidth = 0;
  int prevHeight = 0;
  for( int iFile = 0; iFile < 6; ++iFile )
  {
    const String& filepath = files[ iFile ];
    auto memory = TemporaryMemoryFromFile( filepath, errors );
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
    TAC_ON_DESTRUCT
    (
      stbi_image_free( loaded );
    prevWidth = x;
    prevHeight = y;
    );

    if( iFile && !( x == prevWidth && y == prevHeight ) )
    {
      const String& filepathPrev = files[ iFile - 1 ];
      errors = filepath + " has dimensions " +
        ToString( x ) + "x" + ToString( y ) +
        " which is different from " + filepathPrev + " dimensions " +
        ToString( prevWidth ) + "x" + ToString( prevHeight );
      TAC_HANDLE_ERROR( errors );
    }

    int pitch = x * format.mElementCount * format.mPerElementByteCount;
    int imageDataByteCount = y * pitch;
    Vector< char >& imageData = mData->mImageData[ iFile ];
    imageData.resize( imageDataByteCount );
    MemCpy( imageData.data(), loaded, imageDataByteCount );
  }

  Image& image = mData->mImage;
  image.mFormat = format;
  image.mWidth = prevWidth;
  image.mHeight = prevHeight;
  image.mPitch = prevWidth * format.mElementCount * format.mPerElementByteCount;
}

TextureAssetManager* TextureAssetManager::Instance = nullptr;
TextureAssetManager::TextureAssetManager()
{
  Instance = this;
}
TextureAssetManager::~TextureAssetManager()
{
  for( auto pair : mLoadedTextures )
  {
    Texture* texture = pair.second;
    Renderer::Instance->RemoveRendererResource( texture );
  }
}
Texture* TextureAssetManager::FindLoadedTexture( const String& key )
{
  auto it = mLoadedTextures.find( key );
  if( it == mLoadedTextures.end() )
    return nullptr;
  return ( *it ).second;
}
AsyncTexture* TextureAssetManager::FindLoadingTexture( const String& key )
{
  auto it = mLoadingTextures.find( key );
  if( it == mLoadingTextures.end() )
    return nullptr;
  return ( *it ).second;
}
void TextureAssetManager::GetTextureCube(
  Texture** ppTexture,
  const String& textureDir,
  Errors& errors )
{
  Texture* texture = FindLoadedTexture( textureDir );
  if( texture )
  {
    *ppTexture = texture;
    return;
  }

  AsyncTexture* asyncTexture = FindLoadingTexture( textureDir );
  if( !asyncTexture )
  {
    auto data = new AsyncTextureCubeData;
    data->mDir = textureDir;

    auto job = new AsyncTextureCubeJob;
    job->mData = data;

    asyncTexture = new AsyncTexture;
    asyncTexture->mJob = job;
    asyncTexture->mData = data;
    mLoadingTextures[ textureDir ] = asyncTexture;
    JobQueue::Instance->Push( job );
    *ppTexture = nullptr;
    return;
  }

  UpdateAsyncTexture( ppTexture, textureDir, asyncTexture, errors );
}
void TextureAssetManager::UpdateAsyncTexture(
  Texture** ppTexture,
  const String& key,
  AsyncTexture* asyncTexture,
  Errors& errors )
{
  Job* job = asyncTexture->mJob;
  AsyncLoadStatus status = job->GetStatus();
  switch( status )
  {
    case AsyncLoadStatus::ThreadQueued:
    {
      *ppTexture = nullptr;
    } break;
    case AsyncLoadStatus::ThreadRunning:
    {
      *ppTexture = nullptr;
    } break;
    case AsyncLoadStatus::ThreadFailed:
    {
      *ppTexture = nullptr;
      errors = job->mErrors;
      TAC_HANDLE_ERROR( errors );
    } break;
    case AsyncLoadStatus::ThreadCompleted:
    {
      asyncTexture->mData->CreateTexture( ppTexture, errors );
      TAC_HANDLE_ERROR( errors );
      mLoadingTextures.erase( key );
      delete asyncTexture->mData;
      delete asyncTexture->mJob;
      delete asyncTexture;
      mLoadedTextures[ key ] = *ppTexture;
      break;
    }
    TAC_INVALID_DEFAULT_CASE( status );
  }
}
void TextureAssetManager::GetTexture(
  Texture** ppTexture,
  const String& textureFilepath,
  Errors& errors )
{
  Texture* texture = FindLoadedTexture( textureFilepath );
  if( texture )
  {
    *ppTexture = texture;
    return;
  }

  AsyncTexture* asyncTexture = FindLoadingTexture( textureFilepath );
  if( !asyncTexture )
  {
    auto data = new AsyncTextureSingleData;
    data->mFilepath = textureFilepath;

    auto job = new AsyncTextureSingleJob;
    job->mData = data;

    asyncTexture = new AsyncTexture;
    asyncTexture->mData = data;
    asyncTexture->mJob = job;

    mLoadingTextures[ textureFilepath ] = asyncTexture;
    JobQueue::Instance->Push( job );
    *ppTexture = nullptr;
    return;
  }

  UpdateAsyncTexture( ppTexture, textureFilepath, asyncTexture, errors );
}

}

