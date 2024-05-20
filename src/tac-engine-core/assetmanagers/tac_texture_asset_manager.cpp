#include "tac_texture_asset_manager.h" // self-inc

//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/string/tac_string_identifier.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-engine-core/job/tac_job_queue.h"
#include "tac-engine-core/thirdparty/stb_image.h"
#include "tac-engine-core/settings/tac_settings_root.h"


namespace Tac::TextureAssetManager
{
  // -----------------------------------------------------------------------------------------------

  struct AsyncSubresourceData
  {
    int              mPitch {};
    Vector< char >   mBytes;
  };

  struct TextureLoadJob : public Job
  {
  public:
    struct Params
    {
      FileSys::Path mFilepath;
      bool          mIsCubemap{};
    };

    TextureLoadJob( Params );
    Render::TextureHandle CreateTexture( Errors& );

  protected:
    void Execute( Errors& ) override;

  private:

    Render::TextureHandle CreateTexSingle( Errors& );
    Render::TextureHandle CreateTexCubemap( Errors& );
    void                  ExecuteTexSingleJob( Errors& );
    void                  ExecuteTexCubemapJob( Errors& );
    void                  GenerateMip( int );


    Vector< AsyncSubresourceData > mSubresources;
    bool                           mIsCubemap{};
    bool                           mIs_sRGB{};
    Render::Image                  mImage;
    Render::TexFmt                 mTexFmt{ Render::TexFmt::kUnknown };

    //                             for a single texture, this is a file on desk,
    //                             for a cubemap texture, this is a folder containing 6 files
    FileSys::Path                  mFilepath;
  };


  // -----------------------------------------------------------------------------------------------

  static Map< StringID, TextureLoadJob* >       mLoadingTextures;
  static Map< StringID, Render::TextureHandle > mLoadedTextures;


  TextureLoadJob::TextureLoadJob( Params params )
  {
    mFilepath = params.mFilepath;
    mIsCubemap = params.mIsCubemap;
  }

  // -----------------------------------------------------------------------------------------------

  Render::TextureHandle TextureLoadJob::CreateTexture( Errors& errors )
  {
    return mIsCubemap ? CreateTexCubemap( errors ) : CreateTexSingle( errors );
  }

  Render::TextureHandle TextureLoadJob::CreateTexSingle( Errors& errors )
  {
    const int n{ mSubresources.size() };
    Vector< Render::CreateTextureParams::Subresource > subresources( n );
    for( int i{}; i < n; ++i )
    {
      const AsyncSubresourceData& src{ mSubresources[ i ] };
      const void* bytes{ src.mBytes.data() };
      subresources[ i ] = Render::CreateTextureParams::Subresource
      {
        .mBytes{ bytes },
        .mPitch{ src.mPitch },
      };
    }

    const String name{ mFilepath.filename().u8string() };

    const Render::CreateTextureParams createTextureParams
    {
       .mImage        { mImage },
       .mMipCount     { n },
       .mSubresources { subresources.data(), n },
       .mBinding      { Render::Binding::ShaderResource },
       .mOptionalName { name },
       .mStackFrame   { TAC_STACK_FRAME },
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    return renderDevice->CreateTexture( createTextureParams, errors );
  }

  Render::TextureHandle TextureLoadJob::CreateTexCubemap( Errors& errors )
  {
    Render::CreateTextureParams::Subresource subresources[ 6 ];
    Render::CreateTextureParams::CubemapFaces cubemapFaces;

    // TODO: cubemap mipmaps ( edge filtering )
    mSubresources.resize( 6 );

    for( int iFace{}; iFace < 6; ++iFace )
    {
      const AsyncSubresourceData& faceMip0{ mSubresources[ iFace ] };
      const void* bytes{ faceMip0.mBytes.data() };

      Render::CreateTextureParams::Subresource& subresource{ subresources[ iFace ] };

      subresource = Render::CreateTextureParams::Subresource
      {
        .mBytes{ bytes },
        .mPitch{ faceMip0.mPitch },
      };

      cubemapFaces[ iFace ] = Render::CreateTextureParams::CubemapFace
      {
        .mSubresource{  &subresource  },
      };
    }

    const Render::CreateTextureParams commandData
    { 
      .mImage        { mImage },
      .mCubemapFaces { cubemapFaces },
      .mBinding      { Render::Binding::ShaderResource },
      .mStackFrame   { TAC_STACK_FRAME },
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    return renderDevice->CreateTexture( commandData, errors );
  }

  // -----------------------------------------------------------------------------------------------

  void TextureLoadJob::Execute( Errors& errors )
  {
    if( mIsCubemap )
      ExecuteTexCubemapJob( errors );
    else
      ExecuteTexSingleJob( errors );
  }

  void TextureLoadJob::ExecuteTexSingleJob( Errors& errors )
  {
    TAC_CALL( const String memory{ FileSys::LoadFilePath( mFilepath, errors ) } );

    const FileSys::Path metaPath{
      [ & ]()
      {
        FileSys::Path path { mFilepath };
        path += ".meta";
        return path;
      }( ) };

    SettingsRoot settingsRoot;
    TAC_CALL( settingsRoot.Init( metaPath, errors ) );

    SettingsNode settingsNode{ settingsRoot.GetRootNode() };
    mIs_sRGB = settingsNode.GetChild( "is sRGB" ).GetValueWithFallback( true );
    
    const bool genMips{ settingsNode.GetChild( "gen mips" ).GetValueWithFallback( true ) };
    TAC_CALL( settingsRoot.Flush( errors ) );

    int x;
    int y;
    int previousChannelCount;
    const int requiredChannelCount{ 4 };

    // rgba
    const auto memoryByteCount{ ( int )memory.size() };
    const auto memoryData{ ( const stbi_uc* )memory.data() };
    stbi_uc* loaded{ stbi_load_from_memory( memoryData,
                                             memoryByteCount,
                                             &x,
                                             &y,
                                             &previousChannelCount,
                                             requiredChannelCount ) };
    TAC_ON_DESTRUCT( stbi_image_free( loaded ) );

    bool shouldConvertToPremultipliedAlpha{ true };
    if( shouldConvertToPremultipliedAlpha )
    {
      stbi_uc* l{ loaded };
      for( int i{}; i < y; ++i )
      {
        for( int j{ 0 }; j < x; ++j )
        {
          u8* r = l++;
          u8* g = l++;
          u8* b = l++;
          u8* a = l++;
          const float percent{ *a / 255.0f };
          *r = ( u8 )( *r * percent );
          *g = ( u8 )( *g * percent );
          *b = ( u8 )( *b * percent );
        }
      }
    }

    const int pitch{ x * 4 };
    const int imageDataByteCount{ y * pitch };

    mTexFmt = mIs_sRGB ? Render::TexFmt::kRGBA8_unorm_srgb : Render::TexFmt::kRGBA8_unorm;
    mImage = Render::Image
    {
      .mWidth  { x },
      .mHeight { y },
      .mFormat { mTexFmt },
    };

    const int subresourceCount{
      [ & ]()
      {
        if( !genMips )
          return 1;

        int w { x };
        int n { 1 };
        while( w /= 2 )
          n++;

        return n;
      }( ) };

    mSubresources.resize( subresourceCount );

    AsyncSubresourceData& mip0{ mSubresources[ 0 ] };
    mip0.mBytes.resize( imageDataByteCount );
    mip0.mPitch = pitch;
    MemCpy( mip0.mBytes.data(), loaded, imageDataByteCount );

    if( genMips )
      for( int currMip{ 1 }; currMip < subresourceCount; ++currMip )
        GenerateMip( currMip );

  }


  void TextureLoadJob::GenerateMip( int currMip )
  {
    const int prevMip{ currMip - 1 };
    const int prevW{ mImage.mWidth >> prevMip };
    const int prevH{ mImage.mHeight >> prevMip };
    const AsyncSubresourceData& prevData{ mSubresources[ prevMip ] };

    const int currW{ mImage.mWidth >> currMip };
    const int currH{ mImage.mHeight >> currMip };
    const int currPitch{ currW * 4 };
    AsyncSubresourceData& currData{ mSubresources[ currMip ] };

    currData.mPitch = currPitch;
    currData.mBytes.resize( currPitch * currH );

    for( int currY{}; currY < currH; ++currY )
    {
      for( int currX{}; currX < currW; ++currX )
      {
        for( int iChannel{}; iChannel < 4; ++iChannel )
        {
          const int prevOffset
          {
            ( currX * 2 ) * 4 +
            ( currY * 2 ) * prevData.mPitch +
            iChannel
          };

          const u8 prevChannelTL{ ( u8 )prevData.mBytes[ prevOffset ] };
          const u8 prevChannelTR{ ( u8 )prevData.mBytes[ prevOffset + 1 ] };
          const u8 prevChannelBL{ ( u8 )prevData.mBytes[ prevOffset + prevData.mPitch ] };
          const u8 prevChannelBR{ ( u8 )prevData.mBytes[ prevOffset + prevData.mPitch + 1 ] };

          u8& currChannel{ ( u8& )currData.mBytes[ currX + currY * currPitch + iChannel ] };

          if( mIs_sRGB )
          {
            const float prevFilteredLinear
            {
              0.25f * Pow( prevChannelTL / 255.0f, 2.2f ) +
              0.25f * Pow( prevChannelTR / 255.0f, 2.2f ) +
              0.25f * Pow( prevChannelBL / 255.0f, 2.2f ) +
              0.25f * Pow( prevChannelBR / 255.0f, 2.2f )
            };

            currChannel = u8( Pow( prevFilteredLinear, 1 / 2.2f ) * 255 );
          }
          else
          {
            currChannel = ( prevChannelTL + prevChannelTR + prevChannelBL + prevChannelBR ) / 4;
          }
        }
      }
    }
  }

  void TextureLoadJob::ExecuteTexCubemapJob( Errors& errors )
  {

    TAC_CALL( Vector< FileSys::Path > files{
      FileSys::IterateFiles( mFilepath, FileSys::IterateType::Recursive, errors ) } );

    if( files.size() != 6 )
    {
      const String errorMsg{ "found "
      + ToString( files.size() )
      + " textures in "
      + mFilepath.u8string() };

      TAC_RAISE_ERROR( errorMsg);
    }

    auto TrySortPart = [ & ]( StringView face, int desiredIndex )
    {
      for( int i{}; i < 6; ++i )
      {
        FileSys::Path filepath { files[ i ] };
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

    // todo: load hdr cubemaps in rgb16f?
    mTexFmt = Render::TexFmt::kRGBA8_unorm;

    int prevW { 0 };
    int prevH { 0 };
    for( int iFile { 0 }; iFile < 6; ++iFile )
    {
      const FileSys::Path& filepath { files[ iFile ] };
      TAC_CALL( const String memory { LoadFilePath( filepath, errors ) });

      int x;
      int y;
      int previousChannelCount;
      const int requiredChannelCount{ 4 };
      // rgba
      const auto memoryByteCount { ( int )memory.size() };
      const auto memoryData { ( const stbi_uc* )memory.data() };
      stbi_uc* loaded{ stbi_load_from_memory( memoryData,
                                               memoryByteCount,
                                               &x,
                                               &y,
                                               &previousChannelCount,
                                               requiredChannelCount ) };
      TAC_ON_DESTRUCT
      (
        stbi_image_free( loaded );
        prevW = x;
        prevH = y;
      );

      if( iFile && !( x == prevW && y == prevH ) )
      {
        const FileSys::Path& filepathPrev { files[ iFile - 1 ] };
        const String msg{ String()
          + filepath.u8string() + " (" + ToString( x ) + "x" + ToString( y ) + ")"
          + " has different dimensions from "
          + filepathPrev.u8string() + "(" + ToString( prevW ) + "x" + ToString( prevH ) + ")" };
        TAC_RAISE_ERROR( msg );
      }

      const int pitch { x * 4 };
      const int imageDataByteCount { y * pitch };

      AsyncSubresourceData& subresource{ mSubresources[ iFile ] };
      subresource.mPitch = pitch;
      subresource.mBytes.resize( imageDataByteCount );
      MemCpy( subresource.mBytes.data(), loaded, imageDataByteCount );
    }

    mImage = Render::Image
    {
      .mWidth  { prevW },
      .mHeight { prevH },
      .mFormat { mTexFmt },
    };
  }

  // -----------------------------------------------------------------------------------------------

  static Render::TextureHandle FindLoadedTexture( const StringID& key )
  {
    return mLoadedTextures.FindVal( key ).GetValueOr( {} );
  }

  static TextureLoadJob*       FindLoadingTexture( const StringID& key )
  {
    return mLoadingTextures.FindVal( key ).GetValueOr( {} );
  }

  static void                  UpdateTextureLoadJob( const AssetPathStringView& key,
                                                   TextureLoadJob* asyncTexture,
                                                   Errors& errors )
  {
    const JobState status { asyncTexture->GetStatus() };
    const StringID id ( key );
    if( status == JobState::ThreadFinished )
    {
      TAC_RAISE_ERROR_IF( asyncTexture->mErrors, asyncTexture->mErrors.ToString() );
      TAC_CALL( Render::TextureHandle texture { asyncTexture->CreateTexture( errors )  } );
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

    if( Render::TextureHandle texture{ FindLoadedTexture( id ) }; texture.IsValid() )
      return texture;

    if( TextureLoadJob* asyncTexture { FindLoadingTexture( id ) } )
    {
      UpdateTextureLoadJob( textureFilepath, asyncTexture, errors );
      return {};
    }

    const TextureLoadJob::Params params
    {
      .mFilepath  { textureFilepath },
      .mIsCubemap { false },
    };
    TextureLoadJob* asyncTexture{ TAC_NEW TextureLoadJob( params ) };

    mLoadingTextures[ textureFilepath ] = asyncTexture;
    JobQueuePush( asyncTexture );
    return {};
  }

  Render::TextureHandle GetTextureCube( const AssetPathStringView& textureDir, Errors& errors )
  {
    const StringID id ( textureDir);
    if( const Render::TextureHandle texture { FindLoadedTexture( id ) }; texture.IsValid() )
      return texture;

    if( TextureLoadJob* asyncTexture { FindLoadingTexture( id ) } )
    {
      UpdateTextureLoadJob( textureDir, asyncTexture, errors );
      return {};
    }

    const TextureLoadJob::Params params
    {
      .mFilepath  { textureDir },
      .mIsCubemap { true },
    };
    TextureLoadJob* asyncTexture = TAC_NEW TextureLoadJob(params);
    mLoadingTextures[ textureDir ] = asyncTexture;
    JobQueuePush( asyncTexture );
    return {};
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::TextureAssetManager
