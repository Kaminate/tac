#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacJobQueue.h"
#include "src/common/tacMemory.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/tacOS.h"
#include "src/common/tacUtility.h"
#include "src/common/thirdparty/stb_image.h"
namespace Tac
{
  namespace TextureAssetManager
  {
    struct AsyncTextureData
    {
      virtual ~AsyncTextureData() = default;
      virtual void CreateTexture( Render::TextureHandle* texture, Tac::Errors& errors ) = 0;
    };

    struct AsyncTexture
    {
      Job* mJob = nullptr;
      AsyncTextureData* mData = nullptr;
    };
    static std::map< StringID, AsyncTexture* > mLoadingTextures;
    static std::map< StringID, Render::TextureHandle > mLoadedTextures;


    struct AsyncTextureSingleData : AsyncTextureData
    {
      void CreateTexture( Render::TextureHandle* texture, Tac::Errors& errors ) override
      {
        Render::TexSpec commandData = {};
        commandData.mBinding = Binding::ShaderResource;
        commandData.mImage = mImage;
        commandData.mImageBytes = mImageData.data();
        commandData.mPitch = mPitch; // mImage.mFormat.CalculateTotalByteCount() * mImage.mWidth;
        *texture = Render::CreateTexture( mFilepath, commandData, TAC_STACK_FRAME );
        TAC_HANDLE_ERROR( errors );
      }
      int mPitch = 0;
      Image mImage;
      Vector< char > mImageData;
      String mFilepath;
    };


    struct AsyncTextureCubeData : AsyncTextureData
    {
      void CreateTexture( Render::TextureHandle* texture, Tac::Errors& errors ) override
      {
        Render::TexSpec commandData = {};
        commandData.mBinding = Binding::ShaderResource;
        commandData.mImage = mImage;
        commandData.mImageBytesCubemap[ 0 ] = mImageData[ 0 ].data();
        commandData.mImageBytesCubemap[ 1 ] = mImageData[ 1 ].data();
        commandData.mImageBytesCubemap[ 2 ] = mImageData[ 2 ].data();
        commandData.mImageBytesCubemap[ 3 ] = mImageData[ 3 ].data();
        commandData.mImageBytesCubemap[ 4 ] = mImageData[ 4 ].data();
        commandData.mImageBytesCubemap[ 5 ] = mImageData[ 5 ].data();
        commandData.mPitch = mPitch;
        *texture = Render::CreateTexture( mDir, commandData, TAC_STACK_FRAME );
        TAC_HANDLE_ERROR( errors );
      }
      int mPitch = 0;
      Image mImage;
      Vector< char > mImageData[ 6 ];
      String mDir;
    };


    struct AsyncTextureSingleJob : Job
    {
      void Execute() override
      {
        Tac::Errors& errors = mErrors;

        auto memory = TemporaryMemoryFromFile( mData->mFilepath, errors );
        TAC_HANDLE_ERROR( errors );

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
              uint8_t* r = l++;
              uint8_t* g = l++;
              uint8_t* b = l++;
              uint8_t* a = l++;
              const float percent = *a / 255.0f;
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

        const int pitch = x * format.mElementCount * format.mPerElementByteCount;
        const int imageDataByteCount = y * pitch;
        mData->mImageData.resize( imageDataByteCount );
        MemCpy( mData->mImageData.data(), loaded, imageDataByteCount );

        Image& image = mData->mImage;
        image.mFormat = format;
        image.mWidth = x;
        image.mHeight = y;
        mData->mPitch = pitch;
      }
      AsyncTextureSingleData* mData = nullptr;
    };


    struct AsyncTextureCubeJob : Job
    {
      void Execute() override
      {
        Tac::Errors& errors = mErrors;

        Vector< String > files;
        OS::GetDirFilesRecursive( files, mData->mDir, errors );
        TAC_HANDLE_ERROR( errors );

        if( files.size() != 6 )
        {
          const String errorMsg = "found " + ToString( files.size() ) + " textures in " + mData->mDir;
          TAC_RAISE_ERROR( errorMsg, errors );
        }

        auto TrySortPart = [&]( StringView face, int desiredIndex )
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
          StringView filepath = files[ iFile ];
          auto memory = TemporaryMemoryFromFile( filepath, errors );
          TAC_HANDLE_ERROR( mErrors );

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
            const StringView filepathPrev = files[ iFile - 1 ];
            const String errorMsg = filepath + " has dimensions " +
              ToString( x ) + "x" + ToString( y ) +
              " which is different from " + filepathPrev + " dimensions " +
              ToString( prevWidth ) + "x" + ToString( prevHeight );
            TAC_RAISE_ERROR( errorMsg, errors );
          }

          const int pitch = x * format.mElementCount * format.mPerElementByteCount;
          const int imageDataByteCount = y * pitch;
          Vector< char >& imageData = mData->mImageData[ iFile ];
          imageData.resize( imageDataByteCount );
          MemCpy( imageData.data(), loaded, imageDataByteCount );
          mData->mPitch = pitch;
        }

        Image& image = mData->mImage;
        image.mFormat = format;
        image.mWidth = prevWidth;
        image.mHeight = prevHeight;
      }
      AsyncTextureCubeData* mData = nullptr;
    };


    static Render::TextureHandle FindLoadedTexture( StringView key )
    {
      auto it = mLoadedTextures.find( key );
      if( it == mLoadedTextures.end() )
        return Render::TextureHandle();
      return ( *it ).second;
    }

    static AsyncTexture* FindLoadingTexture( StringView key )
    {
      auto it = mLoadingTextures.find( key );
      if( it == mLoadingTextures.end() )
        return nullptr;
      return ( *it ).second;
    }


    static void UpdateAsyncTexture( StringView key,
                                    AsyncTexture* asyncTexture,
                                    Tac::Errors& errors )
    {
      const Job* job = asyncTexture->mJob;
      const JobState status = job->GetStatus();
      if( status == JobState::ThreadFinished )
      {
        if( job->mErrors )
        {
          errors = job->mErrors;
          TAC_HANDLE_ERROR( errors );
        }
        else
        {
          Render::TextureHandle texture;
          asyncTexture->mData->CreateTexture( &texture, errors );
          TAC_HANDLE_ERROR( errors );
          mLoadingTextures.erase( key );
          delete asyncTexture->mData;
          delete asyncTexture->mJob;
          delete asyncTexture;
          mLoadedTextures[ key ] = texture;
        }
      }
    }

    Render::TextureHandle GetTexture( StringView textureFilepath,
                                      Errors& errors )
    {
      Render::TextureHandle texture = FindLoadedTexture( textureFilepath );
      if( texture.IsValid() )
        return texture;

      AsyncTexture* asyncTexture = FindLoadingTexture( textureFilepath );
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

    Render::TextureHandle GetTextureCube( StringView textureDir,
                                          Errors& errors )
    {
      Render::TextureHandle texture = FindLoadedTexture( textureDir );
      if( texture.IsValid() )
        return texture;

      AsyncTexture* asyncTexture = FindLoadingTexture( textureDir );
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
  }
}
