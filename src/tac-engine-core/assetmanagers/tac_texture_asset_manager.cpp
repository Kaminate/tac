#include "tac_texture_asset_manager.h" // self-inc

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/filesystem/tac_asset.h"
//#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/string/tac_string_identifier.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-engine-core/job/tac_job_queue.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-engine-core/thirdparty/stb_image.h"


namespace Tac::TextureAssetManager
{

  // -----------------------------------------------------------------------------------------------

  struct AsyncTextureData
  {
    virtual ~AsyncTextureData() = default;
    virtual Render::TextureHandle CreateTexture( Errors& ) = 0;
  };

  struct AsyncTexture
  {
    ~AsyncTexture();
    Job*              mJob = nullptr;
    AsyncTextureData* mData = nullptr;
  };

  struct AsyncTextureSingleData : AsyncTextureData
  {
    Render::TextureHandle CreateTexture( Errors& ) override;
    int              mPitch = 0;
    Render::Image    mImage;
    Vector< char >   mImageData;
    Filesystem::Path mFilepath;
  };

  struct AsyncTextureCubeData : AsyncTextureData
  {
    Render::TextureHandle CreateTexture( Errors& ) override;

    int              mPitch = 0;
    Render::Image    mImage;
    Vector< char >   mImageData[ 6 ];
    Filesystem::Path mDir;
  };

  struct AsyncTextureSingleJob : Job
  {
    void Execute() override;
    AsyncTextureSingleData* mData = nullptr;
  };

  struct AsyncTextureCubeJob : Job
  {
    void Execute() override;
    AsyncTextureCubeData* mData = nullptr;
  };

  // -----------------------------------------------------------------------------------------------

  static Map< StringID, AsyncTexture* >         mLoadingTextures;
  static Map< StringID, Render::TextureHandle > mLoadedTextures;

  // -----------------------------------------------------------------------------------------------

  AsyncTexture::~AsyncTexture()
  {
      TAC_DELETE mJob;
      TAC_DELETE mData;
      mJob = nullptr;
      mData = nullptr;
  }


  // -----------------------------------------------------------------------------------------------

  Render::TextureHandle AsyncTextureSingleData::CreateTexture( Errors& errors )
  {
      const Render::CreateTextureParams createTextureParams
      {
         .mImage = mImage,
         .mPitch = mPitch,
         .mImageBytes = mImageData.data(),
         .mBinding = Render::Binding::ShaderResource,
         .mStackFrame = TAC_STACK_FRAME,
      };
      return Render::RenderApi::GetRenderDevice()->CreateTexture( createTextureParams, errors );
  }

  // -----------------------------------------------------------------------------------------------

  Render::TextureHandle AsyncTextureCubeData::CreateTexture(  Errors& errors )
  {
    const Render::CreateTextureParams commandData =
    { 
      .mImage = mImage,
      .mPitch = mPitch,
      .mImageBytesCubemap
      {
        mImageData[ 0 ].data(),
        mImageData[ 1 ].data(),
        mImageData[ 2 ].data(),
        mImageData[ 3 ].data(),
        mImageData[ 4 ].data(),
        mImageData[ 5 ].data()
      },
      .mBinding = Render::Binding::ShaderResource,
      .mStackFrame  = TAC_STACK_FRAME ,
    };
    return Render::RenderApi::GetRenderDevice()->CreateTexture( commandData, errors );
  }

  // -----------------------------------------------------------------------------------------------

  void AsyncTextureSingleJob::Execute()
  {
    Errors& errors = mErrors;

    const String memory = TAC_CALL( LoadFilePath( mData->mFilepath, errors ));

    int x;
    int y;
    int previousChannelCount;
    int desiredChannelCount = 4;

    // rgba
    const auto memoryByteCount = ( int )memory.size();
    const auto memoryData = ( const stbi_uc* )memory.data();
    stbi_uc* loaded = stbi_load_from_memory( memoryData,
                                             memoryByteCount,
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
          u8* r = l++;
          u8* g = l++;
          u8* b = l++;
          u8* a = l++;
          const float percent = *a / 255.0f;
          *r = ( u8 )( *r * percent );
          *g = ( u8 )( *g * percent );
          *b = ( u8 )( *b * percent );
        }
      }
    }

    const Render::Format format{ .mElementCount = desiredChannelCount,
                                 .mPerElementByteCount = 1,
                                 .mPerElementDataType = Render::GraphicsType::unorm};
    const int pitch = x * format.mElementCount * format.mPerElementByteCount;
    const int imageDataByteCount = y * pitch;
    mData->mImageData.resize( imageDataByteCount );
    MemCpy( mData->mImageData.data(), loaded, imageDataByteCount );

    Render::Image& image = mData->mImage;
    image.mFormat = format;
    image.mWidth = x;
    image.mHeight = y;
    mData->mPitch = pitch;
  }

  // -----------------------------------------------------------------------------------------------

  void AsyncTextureCubeJob::Execute()
  {
    Errors& errors = mErrors;

    Vector< Filesystem::Path > files = TAC_CALL( Filesystem::IterateFiles( mData->mDir,
                                                                 Filesystem::IterateType::Recursive,
                                                                 errors ));

    if( files.size() != 6 )
    {
      const String errorMsg
        = "found "
        + ToString( files.size() )
        + " textures in "
        + mData->mDir.u8string();
      TAC_RAISE_ERROR( errorMsg);
    }

    auto TrySortPart = [ & ]( StringView face, int desiredIndex )
    {
      for( int i = 0; i < 6; ++i )
      {
        Filesystem::Path filepath = files[ i ];
        if( ToLower( filepath.u8string() ).find( ToLower( face ) ) == String::npos )
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


    const Render::Format format
    {
      .mElementCount = 4,
      .mPerElementByteCount = 1,
      .mPerElementDataType = Render::GraphicsType::unorm
    };
    int prevWidth = 0;
    int prevHeight = 0;
    for( int iFile = 0; iFile < 6; ++iFile )
    {
      const Filesystem::Path& filepath = files[ iFile ];
      const String memory = TAC_CALL( LoadFilePath( filepath, errors ));

      int x;
      int y;
      int previousChannelCount;
      // rgba
      const auto memoryByteCount = ( int )memory.size();
      const auto memoryData = ( const stbi_uc* )memory.data();
      stbi_uc* loaded = stbi_load_from_memory( memoryData,
                                               memoryByteCount,
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
        const Filesystem::Path& filepathPrev = files[ iFile - 1 ];
        String errorMsg;
        errorMsg += filepath.u8string();
        errorMsg += " has dimensions ";
        errorMsg += ToString( x );
        errorMsg += "x";
        errorMsg += ToString( y );
        errorMsg += " which is different from ";
        errorMsg += filepathPrev.u8string();
        errorMsg += ", which has dimensions ";
        errorMsg += ToString( prevWidth );
        errorMsg += "x";
        errorMsg += ToString( prevHeight );
        TAC_RAISE_ERROR( errorMsg);
      }

      const int pitch = x * format.mElementCount * format.mPerElementByteCount;
      const int imageDataByteCount = y * pitch;
      Vector< char >& imageData = mData->mImageData[ iFile ];
      imageData.resize( imageDataByteCount );
      MemCpy( imageData.data(), loaded, imageDataByteCount );
      mData->mPitch = pitch;
    }

    Render::Image& image = mData->mImage;
    image.mFormat = format;
    image.mWidth = prevWidth;
    image.mHeight = prevHeight;
  }

  // -----------------------------------------------------------------------------------------------

  static Render::TextureHandle FindLoadedTexture( const StringID& key )
  {
    return mLoadedTextures.FindVal( key ).GetValueOr( {} );
    //auto it = mLoadedTextures.find( key );
    //return it == mLoadedTextures.end() ? Render::TextureHandle() : ( *it ).second;
  }

  static AsyncTexture*         FindLoadingTexture( const StringID& key )
  {
    return mLoadingTextures.FindVal( key ).GetValueOr( {} );
    //auto it = mLoadingTextures.find( key );
    //return it == mLoadingTextures.end() ? nullptr : ( *it ).second;
  }

  static void                  UpdateAsyncTexture( const AssetPathStringView& key,
                                                   AsyncTexture* asyncTexture,
                                                   Errors& errors )
  {
    const Job* job = asyncTexture->mJob;
    const JobState status = job->GetStatus();
    const StringID id ( key );
    if( status == JobState::ThreadFinished )
    {
      TAC_RAISE_ERROR_IF( job->mErrors, job->mErrors.ToString() );

      Render::TextureHandle texture = TAC_CALL( asyncTexture->mData->CreateTexture( errors ) );
      mLoadingTextures.erase( id );
      TAC_DELETE asyncTexture;
      mLoadedTextures[ id ] = texture;
    }
  }

  // -----------------------------------------------------------------------------------------------

  Render::TextureHandle GetTexture( const AssetPathStringView& textureFilepath, Errors& errors )
  {
    if( textureFilepath.empty() )
      return {};

    const StringID id( textureFilepath );

    Render::TextureHandle texture = FindLoadedTexture( id );
    if( texture.IsValid() )
      return texture;

    AsyncTexture* asyncTexture = FindLoadingTexture( id );
    if( asyncTexture )
    {
      UpdateAsyncTexture( textureFilepath, asyncTexture, errors );
      return texture;
    }

    auto data = TAC_NEW AsyncTextureSingleData;
    data->mFilepath = textureFilepath;

    auto job = TAC_NEW AsyncTextureSingleJob;
    job->mData = data;

    asyncTexture = TAC_NEW AsyncTexture;
    asyncTexture->mData = data;
    asyncTexture->mJob = job;

    mLoadingTextures[ textureFilepath ] = asyncTexture;
    JobQueuePush( job );
    return texture;
  }

  Render::TextureHandle GetTextureCube( const AssetPathStringView& textureDir, Errors& errors )
  {
    const StringID id ( textureDir);
    const Render::TextureHandle texture = FindLoadedTexture( id );
    if( texture.IsValid() )
      return texture;

    AsyncTexture* asyncTexture = FindLoadingTexture( id );
    if( asyncTexture )
    {
      UpdateAsyncTexture( textureDir, asyncTexture, errors );
      return texture;
    }

    auto data = TAC_NEW AsyncTextureCubeData;
    data->mDir = textureDir;

    auto job = TAC_NEW AsyncTextureCubeJob;
    job->mData = data;

    asyncTexture = TAC_NEW AsyncTexture;
    asyncTexture->mJob = job;
    asyncTexture->mData = data;
    mLoadingTextures[ textureDir ] = asyncTexture;
    JobQueuePush( job );
    return texture;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::TextureAssetManager
