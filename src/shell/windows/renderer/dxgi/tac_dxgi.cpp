#include "src/shell/windows/renderer/dxgi/tac_dxgi.h"
#include "src/shell/windows/tac_win32.h"
#include "src/common/string/tac_string.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/containers/tac_array.h"

#include <d3dcommon.h> // WKPDID_D3DDebugObjectName

import std; // #include <sstream> // std::stringstream

using std::uint32_t;
using std::uint16_t;
using std::uint8_t;

#include <dxgi1_6.h> // IDXGIFactory4, IDXGIAdapter4

#pragma comment( lib, "DXGI.lib" )

namespace Tac
{

    IDXGIFactory4* mFactory = nullptr;
    IDXGIAdapter4* mDxgiAdapter4 = nullptr;


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
  void DXGIInit( Errors& errors )
  {
    const UINT flags = IsDebugMode ? DXGI_CREATE_FACTORY_DEBUG : 0;
    const HRESULT hr = CreateDXGIFactory2( flags, IID_PPV_ARGS( &mFactory ) );
    TAC_RAISE_ERROR_IF( FAILED( hr ), "failed to create dxgi factory" );
    DXGISetObjectName( mFactory, "my-dxgi-factory-4" );

    IDXGIAdapter1* dxgiAdapter1;
    SIZE_T maxDedicatedVideoMemory = 0;
    for( UINT i = 0; mFactory->EnumAdapters1( i, &dxgiAdapter1 ) != DXGI_ERROR_NOT_FOUND; ++i )
    {
      TAC_ON_DESTRUCT( dxgiAdapter1->Release() );

      DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
      dxgiAdapter1->GetDesc1( &dxgiAdapterDesc1 );
      if( dxgiAdapterDesc1.DedicatedVideoMemory < maxDedicatedVideoMemory )
        continue;

      maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
      TAC_DXGI_CALL( dxgiAdapter1->QueryInterface, IID_PPV_ARGS( &mDxgiAdapter4 ) );
    }

    DXGISetObjectName( mDxgiAdapter4, "my-dxgi-adaptor-4" );
  }

  void DXGIUninit()
  {
    TAC_RELEASE_IUNKNOWN( mFactory );
    TAC_RELEASE_IUNKNOWN( mDxgiAdapter4 );
  }

  IDXGISwapChain4* DXGICreateSwapChain( HWND hwnd,
                                        IUnknown* pDevice,
                                        int bufferCount,
                                        UINT width,
                                        UINT height,
                                        Errors& errors )
  {

    const DXGI_SAMPLE_DESC SampleDesc = 
    {
      .Count = 1,
    };

    const DXGI_SWAP_CHAIN_DESC1 scd1 =
    {
      .Width = width,
      .Height = height,

      // Standard way of implementing hdr in games is to use 16 bit floating backbuffer, and
      // giving player brightness/gamma controls (?)
      //
      // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space
      //   For presentation, integer-valued display formats (such as DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
      //   always contain sRGB gamma-corrected data.
      //   Float-valued display formats (ie DXGI_FORMAT_R16G16B16A16_FLOAT) contain linear-valued data.
      //.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      .Format = DXGI_FORMAT_R16G16B16A16_FLOAT, // which windows should be SRGB and which not?

      .SampleDesc = SampleDesc,
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = (UINT)bufferCount,

      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
      .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
    };

    const DXGI_RATIONAL RefreshRate
    {
      .Numerator = 1,
      .Denominator = 60,
    };

    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC scfsd = {
      .RefreshRate = RefreshRate,
      .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
      .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
      .Windowed = TRUE,
    };

    IDXGISwapChain1* swapChain1;

    // This call deprecates IDXGIFactory::CreateSwapChain
    const HRESULT createSwapChainHR = mFactory->CreateSwapChainForHwnd( pDevice,
                                                                        hwnd,
                                                                        &scd1,
                                                                        &scfsd,
                                                                        nullptr,
                                                                        &swapChain1 );
    if( FAILED( createSwapChainHR ) )
    {
      const DWORD dwError = HRESULT_CODE( createSwapChainHR ); // ???

      errors.Append( TryInferDXGIErrorStr( createSwapChainHR ) );
      errors.Append( Win32ErrorStringFromDWORD( dwError ) );
      TAC_RAISE_ERROR_RETURN( "Failed to create swap chain", nullptr );
    }

    IDXGISwapChain4* swapChain4;
    const HRESULT scQueryHR = swapChain1->QueryInterface( IID_PPV_ARGS( &swapChain4 ) );
    TAC_RAISE_ERROR_IF_RETURN( FAILED( scQueryHR ), "Failed to query swap chain4", nullptr );

    return swapChain4;
  }

  struct FormatPair
  {
    Render::Format mFormat;
    DXGI_FORMAT    mFormatDXGI;
  };

  static const FormatPair gFormatPairs[] =
  {
    FormatPair{ { 1, sizeof( uint32_t ), Render::GraphicsType::real  }, DXGI_FORMAT_R32_FLOAT          },
    FormatPair{ { 2, sizeof( uint32_t ), Render::GraphicsType::real  }, DXGI_FORMAT_R32G32_FLOAT       },
    FormatPair{ { 3, sizeof( uint32_t ), Render::GraphicsType::real  }, DXGI_FORMAT_R32G32B32_FLOAT    },
    FormatPair{ { 4, sizeof( uint16_t ), Render::GraphicsType::real  }, DXGI_FORMAT_R16G16B16A16_FLOAT },
    FormatPair{ { 4, sizeof( uint32_t ), Render::GraphicsType::real  }, DXGI_FORMAT_R32G32B32A32_FLOAT },
    FormatPair{ { 1, sizeof( uint8_t ),  Render::GraphicsType::unorm }, DXGI_FORMAT_R8_UNORM           },
    FormatPair{ { 2, sizeof( uint8_t ),  Render::GraphicsType::unorm }, DXGI_FORMAT_R8G8_UNORM         },
    FormatPair{ { 4, sizeof( uint8_t ),  Render::GraphicsType::unorm }, DXGI_FORMAT_R8G8B8A8_UNORM     },
    FormatPair{ { 1, sizeof( uint16_t ), Render::GraphicsType::uint  }, DXGI_FORMAT_R16_UINT           },
    FormatPair{ { 1, sizeof( uint32_t ), Render::GraphicsType::uint  }, DXGI_FORMAT_R32_UINT           },
    FormatPair{ { 1, sizeof( uint16_t ), Render::GraphicsType::unorm }, DXGI_FORMAT_R16_UNORM          },
  };

  DXGI_FORMAT      GetDXGIFormatDepth( const int i )
  {
    switch( i )
    {
      // unorm here, float there...  hmm is that ok?
      case 2: return DXGI_FORMAT_D16_UNORM;
      case 4: return DXGI_FORMAT_D32_FLOAT;
      default: TAC_ASSERT_INVALID_CODE_PATH; return DXGI_FORMAT_UNKNOWN;
    }
  }

  Render::Format GetFormat( const DXGI_FORMAT format )
  {
    for( const FormatPair& formatPair : gFormatPairs )
      if( formatPair.mFormatDXGI == format )
        return formatPair.mFormat;
    TAC_ASSERT_INVALID_CODE_PATH;
    return {};
  }

  DXGI_FORMAT      GetDXGIFormatTextureTypeless( int i )
  {
    constexpr int c16 = sizeof( std::uint16_t );
    constexpr int c32 = sizeof( std::uint32_t );
    switch( i )
    {
      case c16: return DXGI_FORMAT_R16_TYPELESS;
      case c32: return DXGI_FORMAT_R32_TYPELESS;
      default: TAC_ASSERT_INVALID_CODE_PATH; return DXGI_FORMAT_UNKNOWN;
    }
  }

  DXGI_FORMAT GetDXGIFormatTexture( const Render::Format textureFormat )
  {
    TAC_ASSERT_MSG( textureFormat.mPerElementByteCount != 16,
                    "You're making a depth buffer, right?"
                    "Byte count should be 2, aka sizeof( uint16_t ), not 16" );

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


  void DXGISetObjectName( IDXGIObject* object, const StringView& name )
  {
    TAC_ASSERT( object );
    // https://docs.microsoft.com/en-us/windows/desktop/api/dxgi/nf-dxgi-idxgiobject-setprivatedata
    const HRESULT hr = object->SetPrivateData( WKPDID_D3DDebugObjectName,
                                               name.size(),
                                               name.data() );
    TAC_ASSERT( hr == S_OK );
  }

  String DXGIGetObjectName(IDXGIObject* obj)
  {
    TAC_ASSERT( obj );
    const int kBufSize = 256;
    char buf[ kBufSize ]{};
    UINT size = kBufSize;
    const HRESULT getHr = obj->GetPrivateData( WKPDID_D3DDebugObjectName, &size, buf );
    TAC_ASSERT( SUCCEEDED( getHr ) || getHr == DXGI_ERROR_NOT_FOUND );
    return buf;
  }


  const char* TryInferDXGIErrorStr( HRESULT res )
  {
    // https://docs.microsoft.com/en-us/windows/desktop/direct3ddxgi/dxgi-error
    switch( res )
    {
      case DXGI_ERROR_ACCESS_DENIED:
        return "DXGI_ERROR_ACCESS_DENIED You tried to use a resource to which you did not have the required access privileges.This error is most typically caused when you write to a shared resource with read - only access.";
      case DXGI_ERROR_ACCESS_LOST:
        return "DXGI_ERROR_ACCESS_LOST The desktop duplication interface is invalid.The desktop duplication interface typically becomes invalid when a different type of image is displayed on the desktop.";
      case DXGI_ERROR_ALREADY_EXISTS:
        return "DXGI_ERROR_ALREADY_EXISTS The desired element already exists.This is returned by DXGIDeclareAdapterRemovalSupport if it is not the first time that the function is called.";
      case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
        return "DXGI_ERROR_CANNOT_PROTECT_CONTENT DXGI can't provide content protection on the swap chain. This error is typically caused by an older driver, or when you use a swap chain that is incompatible with content protection.";
      case DXGI_ERROR_DEVICE_HUNG:
        return "DXGI_ERROR_DEVICE_HUNG The application's device failed due to badly formed commands sent by the application. This is an design-time issue that should be investigated and fixed.";
      case DXGI_ERROR_DEVICE_REMOVED:
        return "DXGI_ERROR_DEVICE_REMOVED The video card has been physically removed from the system, or a driver upgrade for the video card has occurred.The application should destroy and recreate the device.For help debugging the problem, call ID3D10Device::GetDeviceRemovedReason.";
      case DXGI_ERROR_DEVICE_RESET:
        return "DXGI_ERROR_DEVICE_RESET The device failed due to a badly formed command.This is a run - time issue; The application should destroy and recreate the device.";
      case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
        return "DXGI_ERROR_DRIVER_INTERNAL_ERROR The driver encountered a problem and was put into the device removed state.";
      case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
        return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT An event( for example, a power cycle ) interrupted the gathering of presentation statistics.";
      case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
        return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE The application attempted to acquire exclusive ownership of an output, but failed because some other application( or device within the application ) already acquired ownership.";
      case DXGI_ERROR_INVALID_CALL:
        return "DXGI_ERROR_INVALID_CALL The application provided invalid parameter data; this must be debugged and fixed before the application is released.";
      case DXGI_ERROR_MORE_DATA:
        return "DXGI_ERROR_MORE_DATA The buffer supplied by the application is not big enough to hold the requested data.";
      case DXGI_ERROR_NAME_ALREADY_EXISTS:
        return "DXGI_ERROR_NAME_ALREADY_EXISTS The supplied name of a resource in a call to IDXGIResource1::CreateSharedHandle is already associated with some other resource.";
      case DXGI_ERROR_NONEXCLUSIVE:
        return "DXGI_ERROR_NONEXCLUSIVE A global counter resource is in use, and the Direct3D device can't currently use the counter resource.";
      case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
        return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE The resource or request is not currently available, but it might become available later.";
      case DXGI_ERROR_NOT_FOUND:
        return "DXGI_ERROR_NOT_FOUND When calling IDXGIObject::GetPrivateData, the GUID passed in is not recognized as one previously passed to IDXGIObject::SetPrivateData or IDXGIObject::SetPrivateDataInterface.When calling IDXGIFactory::EnumAdapters or IDXGIAdapter::EnumOutputs, the enumerated ordinal is out of range.";
      case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
        return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED Reserved";
      case DXGI_ERROR_REMOTE_OUTOFMEMORY:
        return "DXGI_ERROR_REMOTE_OUTOFMEMORY Reserved";
      case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
        return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE The DXGI output( monitor ) to which the swap chain content was restricted is now disconnected or changed.";
      case DXGI_ERROR_SDK_COMPONENT_MISSING:
        return "DXGI_ERROR_SDK_COMPONENT_MISSING The operation depends on an SDK component that is missing or mismatched.";
      case DXGI_ERROR_SESSION_DISCONNECTED:
        return "DXGI_ERROR_SESSION_DISCONNECTED The Remote Desktop Services session is currently disconnected.";
      case DXGI_ERROR_UNSUPPORTED:
        return "DXGI_ERROR_UNSUPPORTED The requested functionality is not supported by the device or the driver.";
      case DXGI_ERROR_WAIT_TIMEOUT:
        return "DXGI_ERROR_WAIT_TIMEOUT The time - out interval elapsed before the next desktop frame was available.";
      case DXGI_ERROR_WAS_STILL_DRAWING:
        return "DXGI_ERROR_WAS_STILL_DRAWING The GPU was busy at the moment when a call was made to perform an operation, and did not execute or schedule the operation.";
      case S_OK:
        return "S_OK";
      default:
        return nullptr;
    }
  }

  // Appends the failed function call error message to Errors
  void DXGICallAux( const char* fnCallWithArgs, HRESULT res, Errors& errors )
  {
    std::stringstream ss;
    ss << fnCallWithArgs << " returned 0x" << std::hex << res;
    const char* inferredErrorMessage = TryInferDXGIErrorStr( res );
    if( inferredErrorMessage )
    {
      ss << "(";
      ss << inferredErrorMessage;
      ss << ")";
    }

    errors.Append( ss.str().c_str() );
  }

  IDXGIAdapter* DXGIGetAdapter()
  {
    TAC_ASSERT( mDxgiAdapter4 );
    return mDxgiAdapter4;
  }

}
