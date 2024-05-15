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
      FileSys::Path mPath;
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
    void                  GenerateMips( bool issRGB );
    void                  GenerateMip( bool issRGB, int mip );


    Vector< AsyncSubresourceData > mSubresources;
    bool                           mIsCubemap;
    Render::Image                  mImage;

    //                             for a single texture, this is a file on desk,
    //                             for a cubemap texture, this is a folder containing 6 files
    FileSys::Path                  mFilepath;
  };


  // -----------------------------------------------------------------------------------------------

  static Map< StringID, TextureLoadJob* >       mLoadingTextures;
  static Map< StringID, Render::TextureHandle > mLoadedTextures;


  TextureLoadJob::TextureLoadJob( Params params )
  {
    mFilepath = params.mPath;
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

    const Render::CreateTextureParams createTextureParams
    {
       .mImage        { mImage },
       .mMipCount     { n },
       .mSubresources { subresources.data(), n },
       .mBinding      { Render::Binding::ShaderResource },
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
    settingsRoot.Init( metaPath, errors );

    SettingsNode settingsNode{ settingsRoot.GetRootNode() };
    const bool issRGB{ settingsNode.GetChild( "is sRGB" ).GetValueWithFallback( true ) };
    const bool genMips{ settingsNode.GetChild( "gen mips" ).GetValueWithFallback( true ) };
    settingsRoot.Flush( errors );

    int x;
    int y;
    int previousChannelCount;
    int desiredChannelCount{ 4 };

    // rgba
    const auto memoryByteCount{ ( int )memory.size() };
    const auto memoryData{ ( const stbi_uc* )memory.data() };
    stbi_uc* loaded{ stbi_load_from_memory( memoryData,
                                             memoryByteCount,
                                             &x,
                                             &y,
                                             &previousChannelCount,
                                             desiredChannelCount ) };
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

    const Render::Format format
    {
      .mElementCount = desiredChannelCount,
      .mPerElementByteCount = 1,
      .mPerElementDataType = Render::GraphicsType::unorm
    };
    const int pitch{ x * format.mElementCount * format.mPerElementByteCount };
    const int imageDataByteCount{ y * pitch };

    mImage = Render::Image
    {
      .mWidth = x,
      .mHeight = y,
      .mFormat = format,
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
    MemCpy( mip0.mBytes.data(), loaded, imageDataByteCount );

    if( genMips )
      GenerateMips( issRGB );

  }

  void TextureLoadJob::GenerateMip( bool issRGB, int currMip )
  {

    const Render::Format& format{ mImage.mFormat };
    const int texelByteCount{ format.CalculateTotalByteCount() };

    const int prevMip{ currMip - 1 };
    const int prevW{ mImage.mWidth >> prevMip };
    const int prevH{ mImage.mHeight >> prevMip };
    const AsyncSubresourceData& prevData{ mSubresources[ prevMip ] };

    const int currW{ mImage.mWidth >> currMip };
    const int currH{ mImage.mHeight >> currMip };
    const int currPitch{currW * texelByteCount};
    AsyncSubresourceData& currData{ mSubresources[ currMip ] };

    currData.mPitch = currPitch;
    currData.mBytes.resize( currPitch * currH );

    char* currRow{ currData.mBytes.data() };
    int currX{};
    char* currTexel{};
    for( int currY{}; currY < currH; ++currY, currRow += currPitch )
    {
      for( currX = 0, currTexel = currRow; currX < currW; ++currX, currTexel += texelByteCount )
      {
        const char* prevTexelTL{ prevData.mBytes.data()
          + ( currY * 2 ) * prevData.mPitch
          + ( currX * 2 ) };
        const char* prevTexelTR{ prevTexelTL + texelByteCount };
        const char* prevTexelBL{ prevTexelTL + prevData.mPitch };
        const char* prevTexelBR{ prevTexelTL + prevData.mPitch + texelByteCount };


        for( int iChannel{}; iChannel < format.mElementCount; ++iChannel )
        {
          const int channelByteOffset{ iChannel * format.mPerElementByteCount };
          const char* currChannel{ currTexel + channelByteOffset };
          const char* prevChannelTL{ prevTexelTL + channelByteOffset };
          const char* prevChannelTR{ prevTexelTR + channelByteOffset };
          const char* prevChannelBL{ prevTexelBL + channelByteOffset };
          const char* prevChannelBR{ prevTexelBR + channelByteOffset };

          if( format.mPerElementDataType == Render::GraphicsType::unorm &&
              format.mPerElementByteCount == 1 &&
              issRGB )
          {
            const float prevTLLinear{ Pow( *( u8* )prevChannelTL / 255.0f, 2.2f ) };
            const float prevTRLinear{ Pow( *( u8* )prevChannelTR / 255.0f, 2.2f ) };
            const float prevBLLinear{ Pow( *( u8* )prevChannelBL / 255.0f, 2.2f ) };
            const float prevBRLinear{ Pow( *( u8* )prevChannelBR / 255.0f, 2.2f ) };
            const float prevFilteredLinear{
              ( prevTLLinear + prevTRLinear + prevBLLinear + prevBRLinear ) / 4 };

            u8& curr_sRGB{ *( u8* )currChannel };
            curr_sRGB = u8( Pow( prevFilteredLinear, 1 / 2.2f ) / 255 );
          }
          else
          {
            TAC_ASSERT_UNIMPLEMENTED;
          }

        }


      }
    }

  }

  void TextureLoadJob::GenerateMips( bool issRGB )
  {
    const int subresourceCount{ mSubresources.size() };
    for( int currMip{ 1 }; currMip < subresourceCount; ++currMip )
      GenerateMip( issRGB, currMip );
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


    const Render::Format format
    {
      .mElementCount { 4 },
      .mPerElementByteCount { 1 },
      .mPerElementDataType { Render::GraphicsType::unorm },
    };
    int prevW { 0 };
    int prevH { 0 };
    for( int iFile { 0 }; iFile < 6; ++iFile )
    {
      const FileSys::Path& filepath { files[ iFile ] };
      TAC_CALL( const String memory { LoadFilePath( filepath, errors ) });

      int x;
      int y;
      int previousChannelCount;
      // rgba
      const auto memoryByteCount { ( int )memory.size() };
      const auto memoryData { ( const stbi_uc* )memory.data() };
      stbi_uc* loaded{ stbi_load_from_memory( memoryData,
                                               memoryByteCount,
                                               &x,
                                               &y,
                                               &previousChannelCount,
                                               format.mElementCount ) };
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

      const int pitch { x * format.mElementCount * format.mPerElementByteCount };
      const int imageDataByteCount { y * pitch };

      AsyncSubresourceData& subresource{ mSubresources[ iFile ] };
      subresource.mPitch = pitch;
      subresource.mBytes.resize( imageDataByteCount );
      MemCpy( subresource.mBytes.data(), loaded, imageDataByteCount );
    }

    mImage = Render::Image
    {
      .mWidth = prevW,
      .mHeight = prevH,
      .mFormat = format,
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
      .mPath      { textureFilepath },
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
      .mPath      { textureDir },
      .mIsCubemap { true },
    };
    TextureLoadJob* asyncTexture = TAC_NEW TextureLoadJob(params);
    mLoadingTextures[ textureDir ] = asyncTexture;
    JobQueuePush( asyncTexture );
    return {};
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::TextureAssetManager
