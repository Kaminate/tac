#include "tac_dxgi.h" // self-inc

#include "tac-win32/tac_win32.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/tac_ints.h"

#include <d3dcommon.h> // WKPDID_D3DDebugObjectName

#pragma comment( lib, "DXGI.lib" )

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  // static functions

  static const char* DXGI_ERROR_ToString( const HRESULT res )
  {
    switch( res )
    {
#define CASE( c ) case c: return #c
      CASE( DXGI_ERROR_ACCESS_DENIED );
      CASE( DXGI_ERROR_ACCESS_LOST );
      CASE( DXGI_ERROR_ALREADY_EXISTS );
      CASE( DXGI_ERROR_CANNOT_PROTECT_CONTENT );
      CASE( DXGI_ERROR_DEVICE_HUNG );
      CASE( DXGI_ERROR_DEVICE_REMOVED );
      CASE( DXGI_ERROR_DEVICE_RESET );
      CASE( DXGI_ERROR_DRIVER_INTERNAL_ERROR );
      CASE( DXGI_ERROR_FRAME_STATISTICS_DISJOINT );
      CASE( DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE );
      CASE( DXGI_ERROR_INVALID_CALL );
      CASE( DXGI_ERROR_MORE_DATA );
      CASE( DXGI_ERROR_NAME_ALREADY_EXISTS );
      CASE( DXGI_ERROR_NONEXCLUSIVE );
      CASE( DXGI_ERROR_NOT_CURRENTLY_AVAILABLE );
      CASE( DXGI_ERROR_NOT_FOUND );
      CASE( DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED );
      CASE( DXGI_ERROR_REMOTE_OUTOFMEMORY );
      CASE( DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE );
      CASE( DXGI_ERROR_SDK_COMPONENT_MISSING );
      CASE( DXGI_ERROR_SESSION_DISCONNECTED );
      CASE( DXGI_ERROR_UNSUPPORTED );
      CASE( DXGI_ERROR_WAIT_TIMEOUT );
      CASE( DXGI_ERROR_WAS_STILL_DRAWING );
#undef CASE
    }
    return "unknown";
  }

  static const char* DXGI_ERROR_DescToString( const HRESULT res )
  {
    switch( res )
    {
    case DXGI_ERROR_ACCESS_DENIED:                return "You tried to use a resource to which you did not have the required access privileges.This error is most typically caused when you write to a shared resource with read - only access.";
    case DXGI_ERROR_ACCESS_LOST:                  return "The desktop duplication interface is invalid.The desktop duplication interface typically becomes invalid when a different type of image is displayed on the desktop.";
    case DXGI_ERROR_ALREADY_EXISTS:               return "The desired element already exists.This is returned by DXGIDeclareAdapterRemovalSupport if it is not the first time that the function is called.";
    case DXGI_ERROR_CANNOT_PROTECT_CONTENT:       return "DXGI can't provide content protection on the swap chain. This error is typically caused by an older driver, or when you use a swap chain that is incompatible with content protection.";
    case DXGI_ERROR_DEVICE_HUNG:                  return "The application's device failed due to badly formed commands sent by the application. This is an design-time issue that should be investigated and fixed.";
    case DXGI_ERROR_DEVICE_REMOVED:               return "The video card has been physically removed from the system, or a driver upgrade for the video card has occurred.The application should destroy and recreate the device.For help debugging the problem, call ID3D10Device::GetDeviceRemovedReason.";
    case DXGI_ERROR_DEVICE_RESET:                 return "The device failed due to a badly formed command.This is a run - time issue; The application should destroy and recreate the device.";
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR:        return "The driver encountered a problem and was put into the device removed state.";
    case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:    return "An event( for example, a power cycle ) interrupted the gathering of presentation statistics.";
    case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE: return "The application attempted to acquire exclusive ownership of an output, but failed because some other application( or device within the application ) already acquired ownership.";
    case DXGI_ERROR_INVALID_CALL:                 return "The application provided invalid parameter data; this must be debugged and fixed before the application is released.";
    case DXGI_ERROR_MORE_DATA:                    return "The buffer supplied by the application is not big enough to hold the requested data.";
    case DXGI_ERROR_NAME_ALREADY_EXISTS:          return "The supplied name of a resource in a call to IDXGIResource1::CreateSharedHandle is already associated with some other resource.";
    case DXGI_ERROR_NONEXCLUSIVE:                 return "A global counter resource is in use, and the Direct3D device can't currently use the counter resource.";
    case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:      return "The resource or request is not currently available, but it might become available later.";
    case DXGI_ERROR_NOT_FOUND:                    return "When calling IDXGIObject::GetPrivateData, the GUID passed in is not recognized as one previously passed to IDXGIObject::SetPrivateData or IDXGIObject::SetPrivateDataInterface.When calling IDXGIFactory::EnumAdapters or IDXGIAdapter::EnumOutputs, the enumerated ordinal is out of range.";
    case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:   return "Reserved";
    case DXGI_ERROR_REMOTE_OUTOFMEMORY:           return "Reserved";
    case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:     return "The DXGI output( monitor ) to which the swap chain content was restricted is now disconnected or changed.";
    case DXGI_ERROR_SDK_COMPONENT_MISSING:        return "The operation depends on an SDK component that is missing or mismatched.";
    case DXGI_ERROR_SESSION_DISCONNECTED:         return "The Remote Desktop Services session is currently disconnected.";
    case DXGI_ERROR_UNSUPPORTED:                  return "The requested functionality is not supported by the device or the driver.";
    case DXGI_ERROR_WAIT_TIMEOUT:                 return "The time - out interval elapsed before the next desktop frame was available.";
    case DXGI_ERROR_WAS_STILL_DRAWING:            return "The GPU was busy at the moment when a call was made to perform an operation, and did not execute or schedule the operation.";
    }
    return "unknown";
  }

  static const char* DXGI_SWAP_EFFECT_ToString( const DXGI_SWAP_EFFECT fx )
  {
    switch( fx )
    {
#define CASE( c ) case c: return #c;
      CASE( DXGI_SWAP_EFFECT_DISCARD );
      CASE( DXGI_SWAP_EFFECT_SEQUENTIAL );
      CASE( DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL );
      CASE( DXGI_SWAP_EFFECT_FLIP_DISCARD );
#undef CASE
    }
    return "unknown";
  }

  static String FormattedSwapEffect( const DXGI_SWAP_EFFECT fx )
  {
    return String() + DXGI_SWAP_EFFECT_ToString( fx ) + "(" + Tac::ToString( ( int )fx ) + ")";
  }

  static String TryInferDXGIErrorStr( const HRESULT res )
  {
    if( res == S_OK )
      return "There is no error, it is S_OK";

    return String() + DXGI_ERROR_ToString( res ) + "(" + DXGI_ERROR_DescToString( res ) + ")";
  }

  static PCom<IDXGIAdapter1> GetBestAdapter( IDXGIFactory1* factory, Errors& errors )
  {
    PCom<IDXGIAdapter1> bestAdapter;
    DXGI_ADAPTER_DESC1 bestdesc{};

    UINT iAdapter { 0 };
    for( ;; )
    {
      PCom<IDXGIAdapter1> currAdapter;
      if( S_OK != factory->EnumAdapters1( iAdapter++, currAdapter.CreateAddress() ) )
          break;

      DXGI_ADAPTER_DESC1 desc{};
      TAC_DXGI_CALL_RET( {}, currAdapter->GetDesc1( &desc ));
      if( bestAdapter && desc.DedicatedVideoMemory < bestdesc.DedicatedVideoMemory  )
        continue;

      bestdesc = desc;
      bestAdapter = currAdapter;
    }

    return bestAdapter;
  }

  // -----------------------------------------------------------------------------------------------

  // structs

  struct DXGIImpl
  {
    PCom<IDXGIFactory4> mFactory;

    DXGI_ADAPTER_DESC3  mAdapterDesc{};
    PCom<IDXGIAdapter4> mAdapter;

    void Init( Errors& );
  };

  struct FormatPair
  {
    Format         mFormat;
    DXGI_FORMAT    mFormatDXGI;
  };

  // -----------------------------------------------------------------------------------------------

  // static variables

  static DXGIImpl sImpl;

  // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model
  //   bad: DXGI_SWAP_EFFECT_DISCARD
  //   bad: DXGI_SWAP_EFFECT_SEQUENTIAL
  //   good: DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
  //   good: DXGI_SWAP_EFFECT_FLIP_DISCARD
  static const DXGI_SWAP_EFFECT SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  static const FormatPair gFormatPairs[] =
  {
    FormatPair{ { 1, sizeof( u32 ), GraphicsType::real  }, DXGI_FORMAT_R32_FLOAT          },
    FormatPair{ { 2, sizeof( u32 ), GraphicsType::real  }, DXGI_FORMAT_R32G32_FLOAT       },
    FormatPair{ { 3, sizeof( u32 ), GraphicsType::real  }, DXGI_FORMAT_R32G32B32_FLOAT    },
    FormatPair{ { 4, sizeof( u16 ), GraphicsType::real  }, DXGI_FORMAT_R16G16B16A16_FLOAT },
    FormatPair{ { 4, sizeof( u32 ), GraphicsType::real  }, DXGI_FORMAT_R32G32B32A32_FLOAT },
    FormatPair{ { 1, sizeof( u8 ),  GraphicsType::unorm }, DXGI_FORMAT_R8_UNORM           },
    FormatPair{ { 2, sizeof( u8 ),  GraphicsType::unorm }, DXGI_FORMAT_R8G8_UNORM         },
    FormatPair{ { 4, sizeof( u8 ),  GraphicsType::unorm }, DXGI_FORMAT_R8G8B8A8_UNORM     },
    FormatPair{ { 1, sizeof( u16 ), GraphicsType::uint  }, DXGI_FORMAT_R16_UINT           },
    FormatPair{ { 1, sizeof( u32 ), GraphicsType::uint  }, DXGI_FORMAT_R32_UINT           },
    FormatPair{ { 1, sizeof( u16 ), GraphicsType::unorm }, DXGI_FORMAT_R16_UNORM          },
  };

}

namespace Tac
{

  void Render::DXGIImpl::Init( Errors& errors )
  {
    // Only CreateDXGIFactory2 allows the use of flags
    const UINT flags { IsDebugMode ? DXGI_CREATE_FACTORY_DEBUG : 0 };
    PCom< IDXGIFactory2 > factory2;
    TAC_DXGI_CALL( CreateDXGIFactory2( flags, factory2.iid(), factory2.ppv() ) );
    TAC_ASSERT( factory2 );

    mFactory = factory2.QueryInterface< IDXGIFactory4 >();
    TAC_ASSERT( mFactory );
    DXGISetObjectName( ( IDXGIFactory4* )mFactory, "my-dxgi-factory" );

    TAC_CALL( auto bestAdapter{ GetBestAdapter( ( IDXGIFactory1* )mFactory, errors ) } );

    mAdapter = bestAdapter.QueryInterface<IDXGIAdapter4>();
    TAC_ASSERT(mAdapter);
    TAC_DXGI_CALL( mAdapter->GetDesc3( &mAdapterDesc ) );
    DXGISetObjectName( ( IDXGIAdapter* )mAdapter, "my-dxgi-adaptor" );
  }

  // -----------------------------------------------------------------------------------------------


  //void DXGI::CheckHDRSupport()
  //{
  //  // http://asawicki.info/news_1703_programming_hdr_monitor_support_in_direct3d.html
  //
  //  Errors errors; // ignored
  //
  //  // https://docs.microsoft.com/en-us/windows/desktop/api/dxgi/nn-dxgi-idxgioutput
  //  // An IDXGIOutput interface represents an adapter output (such as a monitor).
  //
  //  bool supportsHDR = false;
  //  UINT iOutput = 0;
  //  IDXGIOutput* output;
  //  Vector< IDXGIOutput* > outputs;
  //  while( mDxgiAdapter4->EnumOutputs( iOutput, &output ) != DXGI_ERROR_NOT_FOUND )
  //  {
  //    outputs.push_back( output );
  //    ++iOutput;
  //  }
  //
  //  for( IDXGIOutput* output : outputs )
  //  {
  //    IDXGIOutput6* output6;
  //    HRESULT hr = output->QueryInterface( IID_PPV_ARGS( &output6 ) );
  //    if( FAILED( hr ) )
  //      continue;
  //    
  //    DXGI_OUTPUT_DESC1 outputDesc;
  //    hr = output6->GetDesc1( &outputDesc );
  //    if( FAILED( hr ) )
  //      continue;
  //
  //    // I'm getting DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, which according to
  //    // https://docs.microsoft.com/en-us/windows/desktop/api/dxgicommon/ne-dxgicommon-dxgi_color_space_type
  //    //   Note that this is often implemented with a linear segment,
  //    //   but in that case the exponent is corrected to stay aligned with a gamma 2.2 curve. 
  //    //   This is usually used with 8 or 10 bit color channels.
  //    outputDesc.ColorSpace;
  //  }
  //}

  void Render::DXGIInit( Errors& errors ) { sImpl.Init( errors ); }



  void Render::DXGIUninit() { sImpl = {}; }

#if 0
  DXGI_FORMAT      Render::DXGIGetSwapChainFormat()
  {
      // Standard way of implementing hdr in games is to use 16 bit floating backbuffer, and
      // giving player brightness/gamma controls (?)
      //
      // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space
      //   For presentation, integer display formats (DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) contain
      //   sRGB gamma-corrected data.
      //
      //   Float display formats (DXGI_FORMAT_R16G16B16A16_FLOAT) contain linear data.

      //return DXGI_FORMAT_R8G8B8A8_UNORM;
      return DXGI_FORMAT_R16G16B16A16_FLOAT;
  }
#endif


  PCom<IDXGISwapChain4> Render::DXGICreateSwapChain( const SwapChainCreateInfo& info,
                                                     Errors& errors )
  {

    const DXGI_SAMPLE_DESC SampleDesc = 
    {
      .Count { 1 },
    };

    const DXGI_SWAP_CHAIN_DESC1 scd1 =
    {
      .Width       { ( UINT )info.mWidth },
      .Height      { ( UINT )info.mHeight },
      .Format      { info.mFmt },
      .SampleDesc  { SampleDesc },
      .BufferUsage { DXGI_USAGE_RENDER_TARGET_OUTPUT },
      .BufferCount { ( UINT )info.mBufferCount },
      .SwapEffect  { SwapEffect },
      .Flags       { DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH },
    };

    const DXGI_RATIONAL RefreshRate
    {
      .Numerator { 1 },
      .Denominator { 60 },
    };

    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC scfsd 
    {
      .RefreshRate { RefreshRate },
      .ScanlineOrdering { DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED },
      .Scaling { DXGI_MODE_SCALING_UNSPECIFIED },
      .Windowed { TRUE },
    };

    PCom<IDXGISwapChain1> swapChain1;

    const HRESULT createSwapChainHR{
      sImpl.mFactory->CreateSwapChainForHwnd( info.mDevice,
                                              info.mHwnd,
                                              &scd1,
                                              &scfsd,
                                              nullptr,
                                              swapChain1.CreateAddress() ) };
    if( FAILED( createSwapChainHR ) )
    {
      const DWORD dwError{ ( DWORD )HRESULT_CODE( createSwapChainHR ) }; // ???
      const String dxgiErrStr { TryInferDXGIErrorStr( createSwapChainHR ) };
      const String win32ErrStr { Win32ErrorStringFromDWORD( dwError ) };
      TAC_RAISE_ERROR_RETURN( String()
                              + "CreateSwapChainForHwnd failed, "
                              + dxgiErrStr
                              + win32ErrStr, {} );
    }

    return swapChain1.QueryInterface<IDXGISwapChain4>();
  }

  DXGI_FORMAT Render::TexFmtToDxgiFormat( TexFmt fmt )
  {
    switch( fmt )
    {
    case Tac::Render::TexFmt::kUnknown:      return DXGI_FORMAT_UNKNOWN;
    case Tac::Render::TexFmt::kD24S8:        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case Tac::Render::TexFmt::kRGBA16F:      return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default: TAC_ASSERT_INVALID_CASE( fmt ); return DXGI_FORMAT_UNKNOWN;
    }
  }


  DXGI_FORMAT      Render::GetDXGIFormatDepth( const int i )
  {
    switch( i )
    {
      // unorm here, float there...  hmm is that ok?
      case 2: return DXGI_FORMAT_D16_UNORM;
      case 4: return DXGI_FORMAT_D32_FLOAT;
      default: TAC_ASSERT_INVALID_CODE_PATH; return DXGI_FORMAT_UNKNOWN;
    }
  }

#if 0
  Render::Format Render::GetFormat( const DXGI_FORMAT format )
  {
    for( const Render::FormatPair& formatPair : gFormatPairs )
      if( formatPair.mFormatDXGI == format )
        return formatPair.mFormat;
    TAC_ASSERT_INVALID_CODE_PATH;
    return {};
  }
#endif


  DXGI_FORMAT      Render::GetDXGIFormatTextureTypeless( int i )
  {
    switch( i )
    {
      case 2: return DXGI_FORMAT_R16_TYPELESS;
      case 4: return DXGI_FORMAT_R32_TYPELESS;
      default: TAC_ASSERT_INVALID_CODE_PATH; return DXGI_FORMAT_UNKNOWN;
    }
  }

  DXGI_FORMAT Render::GetDXGIFormatTexture( const Format textureFormat )
  {
    TAC_ASSERT_MSG( textureFormat.mPerElementByteCount != 16,
                    "You're making a depth buffer, right?"
                    "Byte count should be 2, aka sizeof( u16 ), not 16" );

    for( const FormatPair& formatPair : gFormatPairs )
      if( formatPair.mFormat.mElementCount == textureFormat.mElementCount &&
          formatPair.mFormat.mPerElementByteCount == textureFormat.mPerElementByteCount &&
          formatPair.mFormat.mPerElementDataType == textureFormat.mPerElementDataType )
        return formatPair.mFormatDXGI;

    // try again, but bump the element count
    // ^ 2021-06-25 i dont know if this is good,
    for( const FormatPair& formatPair : gFormatPairs )
      if( formatPair.mFormat.mElementCount >= textureFormat.mElementCount && // note the >=
          formatPair.mFormat.mPerElementByteCount == textureFormat.mPerElementByteCount &&
          formatPair.mFormat.mPerElementDataType == textureFormat.mPerElementDataType )
        return formatPair.mFormatDXGI;

    TAC_ASSERT_INVALID_CODE_PATH;
    return DXGI_FORMAT_UNKNOWN;
  }


  void Render::DXGISetObjectName( IDXGIObject* object, const StringView& name )
  {
    TAC_ASSERT( object );
    // https://docs.microsoft.com/en-us/windows/desktop/api/dxgi/nf-dxgi-idxgiobject-setprivatedata
    const HRESULT hr = object->SetPrivateData( WKPDID_D3DDebugObjectName,
                                               name.size(),
                                               name.data() );
    TAC_ASSERT( hr == S_OK );
  }

  String Render::DXGIGetObjectName(IDXGIObject* obj)
  {
    TAC_ASSERT( obj );
    const int kBufSize { 256 };
    char buf[ kBufSize ]{};
    UINT size = kBufSize;
    const HRESULT getHr = obj->GetPrivateData( WKPDID_D3DDebugObjectName, &size, buf );
    TAC_ASSERT( SUCCEEDED( getHr ) || getHr == DXGI_ERROR_NOT_FOUND );
    return buf;
  }



  // Appends the failed function call error message to Errors
  String Render::DXGICallAux( const char* fnCallWithArgs, HRESULT res )
  {
    String inferredErrorMessage { TryInferDXGIErrorStr( res ) };
    String str{ String()
      + fnCallWithArgs + " returned 0x" + Tac::Itoa( ( int )res )
      + "(" + inferredErrorMessage + ")" };
    return str;
  }

  PCom<IDXGIAdapter4> Render::DXGIGetBestAdapter()
  {
    TAC_ASSERT( sImpl.mAdapter );
    return sImpl.mAdapter;
  }

  void Render::CheckSwapEffect( const DXGI_SWAP_EFFECT cur, Errors& errors )
  {
    const DXGI_SWAP_EFFECT tgt = SwapEffect;
    TAC_RAISE_ERROR_IF( cur != tgt, String() +
                        "The swap chain effect is " + FormattedSwapEffect( cur ) + " "
                        "when it was expected to be " + FormattedSwapEffect( tgt ) );
  }

} // namespace Tac
