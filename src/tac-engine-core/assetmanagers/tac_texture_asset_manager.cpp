#include "tac_texture_asset_manager.h" // self-inc

#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/job/tac_job_queue.h"
#include "tac-engine-core/settings/tac_settings_root.h"
#include "tac-engine-core/thirdparty/stb_image.h"
#include "tac-engine-core/thirdparty/stb_image_write.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/tac_ints.h"

namespace Tac
{

#define TAC_TEST_MIPS_BY_SAVING_TO_DISC()            TAC_IS_DEBUG_MODE() && 0
#define TAC_TEST_MIPS_BY_ASSIGNING_A_COLOR_PER_MIP() TAC_IS_DEBUG_MODE() && 0

  // -----------------------------------------------------------------------------------------------

  static const int sBPP{ 4 };

  struct Texel
  {
    Texel() = default;
    Texel( u8 r, u8 g, u8 b )
    {
        mRgba[ 0 ] = r;
        mRgba[ 1 ] = g;
        mRgba[ 2 ] = b;
        mRgba[ 3 ] = 255;
    }
    u8 mRgba[ sBPP ];
  };

  struct AsyncSubresourceData
  {
    const Texel* GetTexel( int x, int y ) const;
    dynmc Texel* GetTexel( int x, int y ) dynmc;
    int          GetTexelIndex( int x, int y ) const;

    int              mPitch {};
    Vector< char >   mBytes {};
  };

  const Texel* AsyncSubresourceData::GetTexel( int x, int y ) const
  {
    return ( const Texel* )&mBytes[ GetTexelIndex( x, y ) ];
  };

  dynmc Texel* AsyncSubresourceData::GetTexel( int x, int y ) dynmc
  {
    return ( dynmc Texel* )&mBytes[ GetTexelIndex( x, y ) ];
  };

  int          AsyncSubresourceData::GetTexelIndex( int x, int y ) const
  {
    return x * sBPP + y * mPitch ;
  }

  // -----------------------------------------------------------------------------------------------

  struct TextureLoadJob : public Job
  {
  public:
    struct Params
    {
      AssetPathStringView mFilepath;
    };

    TextureLoadJob( Params );
    Render::TextureHandle CreateTexture( Errors& );

  protected:
    void Execute( Errors& ) override;

  private:

    Render::TextureHandle CreateTexSingle( Errors& );
    Render::TextureHandle CreateTexCubemap( Errors& );
    void                  ExecuteTexSingleJob( Errors& );
    int                   CalculateMipCount( int, int, SettingsNode );
    void                  ExecuteTexCubemapJob( Errors& );
    void                  GenerateMip( int );

#if TAC_TEST_MIPS_BY_ASSIGNING_A_COLOR_PER_MIP()
    void                  TestMipsByAssigningAColorPerMip();
#endif

#if TAC_TEST_MIPS_BY_SAVING_TO_DISC()
    void                  TestMipsBySavingToDisk();
#endif


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


  struct LoadedTexture
  {
    Render::TextureHandle           mTextureHandle;
    Render::IBindlessArray::Binding mBinding;
  };

  struct LoadedTextureMap : public Map< StringID, LoadedTexture >
  {
    const LoadedTexture*            FindLoadedTexture( AssetPathStringView ) const;
    Render::TextureHandle           FindTextureHandle( AssetPathStringView ) const;
    Render::IBindlessArray::Binding FindBindlessIndex( AssetPathStringView ) const;
    Render::IBindlessArray*         GetBindlessArray() const;
  };

  using LoadingTextureMap = Map< StringID, TextureLoadJob* >;

  // -----------------------------------------------------------------------------------------------

  static LoadingTextureMap       mLoadingTextures;
  static Render::IBindlessArray* sBindlessArray;
  static LoadedTextureMap        mLoadedTextures;

  // -----------------------------------------------------------------------------------------------

  TextureLoadJob::TextureLoadJob( Params params )
  {
    mFilepath = params.mFilepath;
    mIsCubemap = FileSys::IsDirectory( mFilepath );
  }

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

  void                  TextureLoadJob::Execute( Errors& errors )
  {
    if( mIsCubemap )
      ExecuteTexCubemapJob( errors );
    else
      ExecuteTexSingleJob( errors );
  }

  int                   TextureLoadJob::CalculateMipCount( int w, int h, SettingsNode settingsNode )
  {
    int n{ 1 };
    const bool genMips{ settingsNode.GetChild( "gen mips" ).GetValueWithFallback( true ) };
    if( genMips )
    {
      for( ;; )
      {
        w /= 2;
        h /= 2;
        if( !w || !h )
          break;

        n++;
      }
    }

    return n;
  }

  void                  TextureLoadJob::ExecuteTexSingleJob( Errors& errors )
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
    TAC_ON_DESTRUCT( settingsRoot.Flush( errors ) );
    TAC_CALL( settingsRoot.Init( metaPath, errors ) );

    SettingsNode settingsNode{ settingsRoot.GetRootNode() };
    mIs_sRGB = settingsNode.GetChild( "is sRGB" ).GetValueWithFallback( true );
    

    int x;
    int y;
    int previousChannelCount;
    const int requiredChannelCount{ sBPP };

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
        for( int j{}; j < x; ++j )
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

    const int pitch{ x * sBPP };
    const int imageDataByteCount{ y * pitch };

    mTexFmt = mIs_sRGB ? Render::TexFmt::kRGBA8_unorm_srgb : Render::TexFmt::kRGBA8_unorm;
    mImage = Render::Image
    {
      .mWidth  { x },
      .mHeight { y },
      .mFormat { mTexFmt },
    };

    const int subresourceCount{ CalculateMipCount( x, y, settingsNode ) };
    

    mSubresources.resize( subresourceCount );

    AsyncSubresourceData& mip0{ mSubresources[ 0 ] };
    mip0.mBytes.resize( imageDataByteCount );
    mip0.mPitch = pitch;
    MemCpy( mip0.mBytes.data(), loaded, imageDataByteCount );

    for( int currMip{ 1 }; currMip < subresourceCount; ++currMip )
      GenerateMip( currMip );

#if TAC_TEST_MIPS_BY_SAVING_TO_DISC()
    TestMipsBySavingToDisk();
#endif

#if TAC_TEST_MIPS_BY_ASSIGNING_A_COLOR_PER_MIP()
    TestMipsByAssigningAColorPerMip();
#endif
  }

#if TAC_TEST_MIPS_BY_SAVING_TO_DISC()
  // verify correctness of GenerateMip()
  void                  TextureLoadJob::TestMipsBySavingToDisk()
  {
    const int nSubRsc{ mSubresources.size() };
    for( int iSubRsc{}; iSubRsc < nSubRsc; ++iSubRsc )
    {
      const AsyncSubresourceData& data{ mSubresources[ iSubRsc ] };
      const int w{ mImage.mWidth >> iSubRsc };
      const int h{ mImage.mHeight >> iSubRsc };

      dynmc String path{ mFilepath.u8string() };
      const int lastDot{ path.find_last_of( "." ) };
      TAC_ASSERT( lastDot != String::npos );
      path = path.substr( 0, lastDot );
      path += ".mip" + ToString( iSubRsc ) + ".deleteme.png";

      const int writeResult{ stbi_write_png( path.data(),
                                              w, h, sBPP, data.mBytes.data(), data.mPitch ) };
      const bool writeSucceeded{ writeResult != 0 };
      TAC_ASSERT( writeSucceeded );
    }
  }
#endif

#if TAC_TEST_MIPS_BY_ASSIGNING_A_COLOR_PER_MIP()
  void                  TextureLoadJob::TestMipsByAssigningAColorPerMip()
  {

    const Texel colors[ 20 ]
    {
      Texel( 230, 25, 75 ),   // 0 Red
      Texel( 60, 180, 75 ),   // 1 Green
      Texel( 255, 225, 25 ),  // 2 Yellow
      Texel( 0, 130, 200 ),   // 3 Blue
      Texel( 245, 130, 48 ),  // 4 Orange
      Texel( 145, 30, 180 ),  // 5 Purple
      Texel( 70, 240, 240 ),  // 6 Cyan
      Texel( 240, 50, 230 ),  // 7 Magenta
      Texel( 210, 245, 60 ),  // 8 Lime
      Texel( 250, 190, 212 ), // 9 Pink
      Texel( 0, 128, 128 ),   // 10 Teal
      Texel( 220, 190, 255 ), // 11 Lavender
      Texel( 170, 110, 40 ),  // 12 Brown
      Texel( 255, 250, 200 ), // 13 Beige
      Texel( 128, 0, 0 ),     // 14 Maroon
      Texel( 170, 255, 195 ), // 15 Mint
      Texel( 128, 128, 0 ),   // 16 Olive
      Texel( 255, 215, 180 ), // 17 Apricot
      Texel( 0, 0, 128 ),     // 18 Navy
      Texel( 128, 128, 128 ), // 19 Grey
    };

    const int n{ mSubresources.size() };
    for( int i{}; i < n; ++i )
    {
      int w{ mImage.mWidth >> i };
      int h{ mImage.mHeight >> i };
      AsyncSubresourceData& mip{ mSubresources[ i ] };
      void* data{ mip.mBytes.data() };
      const Texel& color{ colors[ i ] };
      for( int r{}; r < h; ++r )
      {
        for( int c{}; c < w; ++c )
        {
          *mip.GetTexel( c, r ) = color;
          //MemCpy( ( char* )data + r * mip.mPitch + c * sBPP, color.mBytes, sBPP );
        }
      }
    }
  }
#endif

  void                  TextureLoadJob::GenerateMip( int currMip )
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
        const int prevXL{ currX * 2 };
        const int prevXR{ currX * 2 + 1 };
        const int prevYT{ currY * 2 };
        const int prevYB{ currY * 2 + 1 };
        Texel prevTL{ *prevData.GetTexel( prevXL, prevYT ) };
        Texel prevTR{ *prevData.GetTexel( prevXR, prevYT ) };
        Texel prevBL{ *prevData.GetTexel( prevXL, prevYB ) };
        Texel prevBR{ *prevData.GetTexel( prevXR, prevYB ) };

        Texel* currTexel{ currData.GetTexel( currX, currY ) };

        for( int iChannel{}; iChannel < sBPP; ++iChannel )
        {
          if( mIs_sRGB )
          {
            const float prevFilteredLinear
            {
              0.25f * Pow( prevTL.mRgba[ iChannel ] / 255.0f, 2.2f ) +
              0.25f * Pow( prevTR.mRgba[ iChannel ] / 255.0f, 2.2f ) +
              0.25f * Pow( prevBL.mRgba[ iChannel ] / 255.0f, 2.2f ) +
              0.25f * Pow( prevBR.mRgba[ iChannel ] / 255.0f, 2.2f )
            };

            currTexel->mRgba[ iChannel ] = u8( Pow( prevFilteredLinear, 1 / 2.2f ) * 255 );
          }
          else
          {
            currTexel->mRgba[ iChannel ] = (
               prevTL.mRgba[ iChannel ] +
               prevTR.mRgba[ iChannel ] +
               prevBL.mRgba[ iChannel ] +
               prevBR.mRgba[ iChannel ] ) / sBPP;

          }
        }
      }
    }
  }

  void                  TextureLoadJob::ExecuteTexCubemapJob( Errors& errors )
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

    int prevW {};
    int prevH {};
    for( int iFile {}; iFile < 6; ++iFile )
    {
      const FileSys::Path& filepath { files[ iFile ] };
      TAC_CALL( const String memory { LoadFilePath( filepath, errors ) });

      int x;
      int y;
      int previousChannelCount;
      const int requiredChannelCount{ sBPP };
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

      const int pitch { x * sBPP };
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

  static TextureLoadJob* FindLoadingTexture( const StringID& key )
  {
    auto it{ mLoadingTextures.find( key ) };
    if( it == mLoadingTextures.end() )
      return nullptr;
    auto& [_, job] { *it};
    return job;
  }

  static void            UpdateTextureLoadJob( const AssetPathStringView& key,
                                               TextureLoadJob* asyncTexture,
                                               Errors& errors )
  {
    const JobState status{ asyncTexture->GetStatus() };
    const StringID id( key );
    if( status == JobState::ThreadFinished )
    {
      TAC_RAISE_ERROR_IF( asyncTexture->mErrors, asyncTexture->mErrors.ToString() );
      TAC_CALL( const Render::TextureHandle texture{ asyncTexture->CreateTexture( errors ) } );
      mLoadingTextures.erase( id );
      TAC_DELETE asyncTexture;

      if( !sBindlessArray )
      {
        Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
        const Render::IBindlessArray::Params bindlessArrayParams
        {
          .mHandleType { Render::HandleType::kTexture },
          .mBinding    { Render::Binding::ShaderResource },
        };
        sBindlessArray = renderDevice->CreateBindlessArray( bindlessArrayParams );
        TAC_ASSERT( sBindlessArray );
      }

      TAC_CALL( const Render::IBindlessArray::Binding binding{
        sBindlessArray->Bind( texture, errors ) } );

      const LoadedTexture loadedTexture
      {
        .mTextureHandle { texture },
        .mBinding       { binding },
      };
      mLoadedTextures[ id ] = loadedTexture;
    }
  }

  static void            LoadTextureAux( TextureLoadJob::Params params,
                                         Errors& errors )
  {
    if( params.mFilepath.empty() )
      return;

    const StringID id( params.mFilepath );
    if( TextureLoadJob* asyncTexture { FindLoadingTexture( id ) } )
    {
      UpdateTextureLoadJob( params.mFilepath, asyncTexture, errors );
      return;
    }

    TextureLoadJob* asyncTexture{ TAC_NEW TextureLoadJob( params ) };

    mLoadingTextures[ id ] = asyncTexture;
    JobQueuePush( asyncTexture );
  }

  // -----------------------------------------------------------------------------------------------

  Render::TextureHandle
    TextureAssetManager::GetTexture( const AssetPathStringView textureFilepath,
                                                         Errors& errors )
  {
    if( Render::TextureHandle texture{ mLoadedTextures.FindTextureHandle( textureFilepath ) };
        texture.IsValid() )
      return texture;

    const TextureLoadJob::Params params{ .mFilepath  { textureFilepath }, };
    LoadTextureAux( params, errors );
    return {};
  }

  Render::IBindlessArray::Binding
    TextureAssetManager::GetBindlessIndex( const AssetPathStringView textureFilepath,
                                           Errors& errors )
  {
    if( textureFilepath.empty() )
      return {};

    if( Render::IBindlessArray::Binding binding{
      mLoadedTextures.FindBindlessIndex( textureFilepath ) };
      binding.IsValid() )
      return binding;

    const TextureLoadJob::Params params { .mFilepath  { textureFilepath }, };
    LoadTextureAux( params, errors );
    return {};
  }

  Render::IBindlessArray*
    TextureAssetManager::GetBindlessArray()
  {
    return sBindlessArray;
  }

  // -----------------------------------------------------------------------------------------------
   
  const LoadedTexture*            LoadedTextureMap::FindLoadedTexture( AssetPathStringView asset ) const 
  {
    auto it{ Find( asset ) };
    if( it == end() )
      return nullptr;
    auto& [_, texture] { *it};
    return &texture;
  }

  Render::TextureHandle           LoadedTextureMap::FindTextureHandle( AssetPathStringView asset ) const
  {
    auto it{ Find( asset ) };
    if( it == end() )
      return {};
    auto& [_, texture] { *it};
    return texture.mTextureHandle;
  }

  Render::IBindlessArray::Binding LoadedTextureMap::FindBindlessIndex( AssetPathStringView asset ) const
  {
    auto it{ Find( asset ) };
    if( it == end() )
      return {};
    auto& [_, texture] { *it};
    return texture.mBinding;
  }

  Render::IBindlessArray*         LoadedTextureMap::GetBindlessArray()const
  {
    return sBindlessArray;
  }

  // -----------------------------------------------------------------------------------------------
} // namespace Tac
