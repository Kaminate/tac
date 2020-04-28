#include "src/common/containers/tacArray.h"
#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/math/tacMath.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacMemory.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacShell.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/windows/tacDXGI.h"
#include "src/shell/windows/tacRendererDirectX11.h"

#include <initguid.h>
#include <dxgidebug.h>
#include <D3DCompiler.h> // D3DCOMPILE_...
#include <d3dcommon.h> // WKPDID_D3DDebugObjectName

#include <utility> // std::pair
#include <sstream> // std::stringstream



#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "D3DCompiler.lib" )

namespace Tac
{
  static void AssertRenderThread()
  {
    const bool isMainThread = gThreadType == ThreadType::Main;
    TAC_ASSERT( isMainThread );
  }


#define TAC_DX11_CALL( errors, call, ... )\
{\
  HRESULT result = call( __VA_ARGS__ );\
  if( FAILED( result ) )\
  {\
    DX11CallAux( TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )", result, errors );\
  }\
}

  static String GetDirectX11ShaderPath( String shaderName )
  {
    return "assets/hlsl/" + shaderName + ".fx";
  }

  static String TryInferDX11ErrorStr( HRESULT res )
  {
    switch( res )
    {
      case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND	The file was not found.";
      case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS	There are too many unique instances of a particular type of state object.";
      case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS	There are too many unique instances of a particular type of view object.";
      case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD: return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD	The first call to ID3D11DeviceContext::Map after either ID3D11Device::CreateDeferredContext or ID3D11DeviceContext::FinishCommandList per Resource was not D3D11_MAP_WRITE_DISCARD.";

        // move to Dxgi.cpp?
      case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL The method call is invalid.For example, a method's parameter may not be a valid pointer.";
      case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING The previous blit operation that is transferring information to or from this surface is incomplete.";
      case DXGI_ERROR_ACCESS_DENIED: return "DXGI_ERROR_ACCESS_DENIED You tried to use a resource to which you did not have the required access privileges.This error is most typically caused when you write to a shared resource with read - only access.";
      case DXGI_ERROR_ACCESS_LOST: return "DXGI_ERROR_ACCESS_LOST The desktop duplication interface is invalid.The desktop duplication interface typically becomes invalid when a different type of image is displayed on the desktop.";
      case DXGI_ERROR_ALREADY_EXISTS: return "DXGI_ERROR_ALREADY_EXISTS The desired element already exists.This is returned by DXGIDeclareAdapterRemovalSupport if it is not the first time that the function is called.";
      case DXGI_ERROR_CANNOT_PROTECT_CONTENT: return "DXGI_ERROR_CANNOT_PROTECT_CONTENT DXGI can't provide content protection on the swap chain. This error is typically caused by an older driver, or when you use a swap chain that is incompatible with content protection.";
      case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG The application's device failed due to badly formed commands sent by the application. This is an design-time issue that should be investigated and fixed.";
      case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED The video card has been physically removed from the system, or a driver upgrade for the video card has occurred.The application should destroy and recreate the device.For help debugging the problem, call ID3D10Device::GetDeviceRemovedReason.";
      case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET The device failed due to a badly formed command.This is a run - time issue; The application should destroy and recreate the device.";
      case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR The driver encountered a problem and was put into the device removed state.";
      case DXGI_ERROR_FRAME_STATISTICS_DISJOINT: return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT An event( for example, a power cycle ) interrupted the gathering of presentation statistics.";
      case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE: return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE The application attempted to acquire exclusive ownership of an output, but failed because some other application( or device within the application ) already acquired ownership.";
      case DXGI_ERROR_MORE_DATA: return "DXGI_ERROR_MORE_DATA The buffer supplied by the application is not big enough to hold the requested data.";
      case DXGI_ERROR_NAME_ALREADY_EXISTS: return "DXGI_ERROR_NAME_ALREADY_EXISTS The supplied name of a resource in a call to IDXGIResource1::CreateSharedHandle is already associated with some other resource.";
      case DXGI_ERROR_NONEXCLUSIVE: return "DXGI_ERROR_NONEXCLUSIVE A global counter resource is in use, and the Direct3D device can't currently use the counter resource.";
      case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE The resource or request is not currently available, but it might become available later.";
      case DXGI_ERROR_NOT_FOUND: return "DXGI_ERROR_NOT_FOUND When calling IDXGIObject::GetPrivateData, the GUID passed in is not recognized as one previously passed to IDXGIObject::SetPrivateData or IDXGIObject::SetPrivateDataInterface.When calling IDXGIFactory::EnumAdapters or IDXGIAdapter::EnumOutputs, the enumerated ordinal is out of range.";
      case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED: return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED Reserved";
      case DXGI_ERROR_REMOTE_OUTOFMEMORY: return "DXGI_ERROR_REMOTE_OUTOFMEMORY Reserved";
      case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE: return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE The DXGI output( monitor ) to which the swap chain content was restricted is now disconnected or changed.";
      case DXGI_ERROR_SDK_COMPONENT_MISSING: return "DXGI_ERROR_SDK_COMPONENT_MISSING The operation depends on an SDK component that is missing or mismatched.";
      case DXGI_ERROR_SESSION_DISCONNECTED: return "DXGI_ERROR_SESSION_DISCONNECTED The Remote Desktop Services session is currently disconnected.";
      case DXGI_ERROR_UNSUPPORTED: return "DXGI_ERROR_UNSUPPORTED The requested functionality is not supported by the device or the driver.";
      case DXGI_ERROR_WAIT_TIMEOUT: return "DXGI_ERROR_WAIT_TIMEOUT The time - out interval elapsed before the next desktop frame was available.";

      case E_FAIL: return "E_FAIL	Attempted to create a device with the debug layer enabled and the layer is not installed.";
      case E_INVALIDARG: return "E_INVALIDARG	An invalid parameter was passed to the returning function.";
      case E_OUTOFMEMORY: return "E_OUTOFMEMORY	Direct3D could not allocate sufficient memory to complete the call.";
      case E_NOTIMPL: return "E_NOTIMPL	The method call isn't implemented with the passed parameter combination.";
      case S_FALSE: return "S_FALSE	Alternate success value, indicating a successful but nonstandard completion( the precise meaning depends on context ).";
      case S_OK: return "S_OK	No error occurred.";
      default: return "idk lol";
    }
  }

  static void DX11CallAux( const char* fnCallWithArgs, HRESULT res, Errors& errors )
  {
    std::stringstream ss;
    ss << fnCallWithArgs << " returned 0x" << std::hex << res;
    String inferredErrorMessage = TryInferDX11ErrorStr( res );
    if( !inferredErrorMessage.empty() )
    {
      ss << "(";
      ss << inferredErrorMessage;
      ss << ")";
    }
    errors.mMessage = ss.str().c_str();
  }

  static void ReportLiveObjects()
  {
    if( !IsDebugMode() )
      return;
    auto Dxgidebughandle = GetModuleHandle( "Dxgidebug.dll" );
    if( !Dxgidebughandle )
      return;

    auto myDXGIGetDebugInterface =
      ( HRESULT( WINAPI * )( REFIID, void** ) )GetProcAddress(
        Dxgidebughandle,
        "DXGIGetDebugInterface" );

    if( !myDXGIGetDebugInterface )
      return;
    IDXGIDebug* myIDXGIDebug;
    HRESULT hr = myDXGIGetDebugInterface( IID_PPV_ARGS( &myIDXGIDebug ) );
    if( FAILED( hr ) )
      return;
    myIDXGIDebug->ReportLiveObjects(
      DXGI_DEBUG_ALL,
      DXGI_DEBUG_RLO_ALL );
    myIDXGIDebug->Release();
  }

  static D3D11_TEXTURE_ADDRESS_MODE GetAddressMode( AddressMode addressMode )
  {
    switch( addressMode )
    {
      case AddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP;
      case AddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP;
      case AddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
        TAC_INVALID_DEFAULT_CASE( addressMode );
    }
    return D3D11_TEXTURE_ADDRESS_WRAP;
  }

  static D3D11_COMPARISON_FUNC GetCompare( Comparison compare )
  {
    switch( compare )
    {
      case Comparison::Always: return D3D11_COMPARISON_ALWAYS;
      case Comparison::Never: return D3D11_COMPARISON_NEVER;
        TAC_INVALID_DEFAULT_CASE( compare );
    }
    return D3D11_COMPARISON_ALWAYS;
  };

  static D3D11_FILTER GetFilter( Filter filter )
  {
    switch( filter )
    {
      case Filter::Linear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      case Filter::Point: return D3D11_FILTER_MIN_MAG_MIP_POINT;
      case Filter::Aniso: return D3D11_FILTER_ANISOTROPIC;
        TAC_INVALID_DEFAULT_CASE( filter );
    }
    return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  };

  static D3D11_COMPARISON_FUNC GetDepthFunc( DepthFunc depthFunc )
  {
    switch( depthFunc )
    {
      case DepthFunc::Less: return D3D11_COMPARISON_LESS;
      case DepthFunc::LessOrEqual: return D3D11_COMPARISON_LESS_EQUAL;
        TAC_INVALID_DEFAULT_CASE( depthFunc );
    }
    return D3D11_COMPARISON_LESS;
  }

  static D3D11_USAGE GetUsage( Access access )
  {
    switch( access )
    {
      case Access::Default: return D3D11_USAGE_DEFAULT;
      case Access::Dynamic: return D3D11_USAGE_DYNAMIC;
      case Access::Static: return D3D11_USAGE_IMMUTABLE;
        TAC_INVALID_DEFAULT_CASE( access );
    }
    return D3D11_USAGE_DEFAULT;
  }

  static UINT GetCPUAccessFlags( CPUAccess access )
  {
    UINT result = 0;
    if( ( int )access & ( int )CPUAccess::Read )
      result |= D3D11_CPU_ACCESS_WRITE;
    if( ( int )access & ( int )CPUAccess::Write )
      result |= D3D11_CPU_ACCESS_WRITE;
    return result;
  }
  //static UINT GetCPUAccessFlags( const std::set< CPUAccess >& access )
  //{
  //  std::map< CPUAccess, UINT > accessMap;
  //  accessMap[ CPUAccess::Read ] = D3D11_CPU_ACCESS_READ;
  //  accessMap[ CPUAccess::Write ] = D3D11_CPU_ACCESS_WRITE;
  //  UINT result = 0;
  //  for( auto accessType : access )
  //    result |= accessMap[ accessType ];
  //  return result;
  //}

  //static UINT GetBindFlags( const std::set< Binding > & binding )
  //{
  //  UINT BindFlags = 0;

  //  if( Contains( binding, Binding::RenderTarget ) )
  //    BindFlags |= D3D11_BIND_RENDER_TARGET;
  //  if( Contains( binding, Binding::ShaderResource ) )
  //    BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  //  return BindFlags;
  //}
  static UINT GetBindFlags( Binding binding )
  {
    UINT BindFlags = 0;
    if( ( int )binding & ( int )Binding::RenderTarget )
      BindFlags |= D3D11_BIND_RENDER_TARGET;
    if( ( int )binding & ( int )Binding::ShaderResource )
      BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    return BindFlags;
  }
  static UINT GetMiscFlags( Binding binding )
  {
    if( ( int )binding & ( int )Binding::RenderTarget &&
      ( int )binding & ( int )Binding::ShaderResource )
      return D3D11_RESOURCE_MISC_GENERATE_MIPS;
    return 0;
  }

  //static D3D11_MAP GetD3D11_MAP( Map mapType )
  //{
  //  switch( mapType )
  //  {
  //    case Map::Read: return D3D11_MAP_READ;
  //    case Map::Write: return D3D11_MAP_WRITE;
  //    case Map::ReadWrite: return D3D11_MAP_READ_WRITE;
  //    case Map::WriteDiscard: return  D3D11_MAP_WRITE_DISCARD;
  //      TAC_INVALID_DEFAULT_CASE( mapType );
  //  }
  //  return D3D11_MAP_READ_WRITE;
  //}

  static D3D11_BLEND GetBlend( BlendConstants c )
  {
    switch( c )
    {
      case BlendConstants::OneMinusSrcA:
        return D3D11_BLEND_INV_SRC_ALPHA;
      case BlendConstants::SrcA:
        return D3D11_BLEND_SRC_ALPHA;
      case BlendConstants::SrcRGB:
        return D3D11_BLEND_SRC_COLOR;
      case BlendConstants::Zero:
        return D3D11_BLEND_ZERO;
      case BlendConstants::One:
        return D3D11_BLEND_ONE;
      default:
        TAC_ASSERT( false );
        return D3D11_BLEND_ZERO;
    }
  };

  static D3D11_BLEND_OP GetBlendOp( BlendMode mode )
  {
    switch( mode )
    {
      case BlendMode::Add:
        return D3D11_BLEND_OP_ADD;
      default:
        TAC_ASSERT( false );
        return D3D11_BLEND_OP_ADD;
    }
  };

  static D3D11_FILL_MODE GetFillMode( FillMode fillMode )
  {
    switch( fillMode )
    {
      case FillMode::Solid: return D3D11_FILL_SOLID;
      case FillMode::Wireframe: return D3D11_FILL_WIREFRAME;
        TAC_INVALID_DEFAULT_CASE( fillMode );
    }
    return ( D3D11_FILL_MODE )0;
  }

  static D3D11_CULL_MODE GetCullMode( CullMode cullMode )
  {
    switch( cullMode )
    {
      case CullMode::None: return D3D11_CULL_NONE;
      case CullMode::Back: return D3D11_CULL_BACK;
      case CullMode::Front: return D3D11_CULL_FRONT;
        TAC_INVALID_DEFAULT_CASE( cullMode );
    }
    return ( D3D11_CULL_MODE )0;
  }

  //static D3D11_PRIMITIVE_TOPOLOGY GetPrimTop( PrimitiveTopology primTop )
  //{
  //  switch( primTop )
  //  {
  //    case TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  //    case LineList:return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
  //      TAC_INVALID_DEFAULT_CASE( primTop );
  //  }
  //  return ( D3D11_PRIMITIVE_TOPOLOGY )0;
  //}

  //static bool AreEqual(
  //  const Vector< const Texture* >& a,
  //  const Vector< const Texture* >& b )
  //{
  //  const int n = a.size();
  //  if( n != b.size() )
  //    return false;
  //  for( int i = 0; i < n; ++i )
  //    if( a[ i ] != b[ i ] )
  //      return false;
  //  return true;
  //}

  //DX11Window::~DX11Window()
  //{
  //  mBackbufferColor->Clear();
  //  mDepthBuffer->Clear();
  //  if( mSwapChain )
  //  {
  //    mSwapChain->Release();
  //    mSwapChain = nullptr;
  //  }
  //  Renderer::Instance->RemoveRendererResource( mBackbufferColor );
  //  Renderer::Instance->RemoveRendererResource( mDepthBuffer );
  //}

  //void DX11Window::OnResize( Errors& errors )
  //{
  //  // The buffers MUST be cleared prior to calling ResizeBuffers ( think )
  //  mBackbufferColor->Clear();
  //  mDepthBuffer->Clear();

  //  // Set this number to zero to preserve the existing number of buffers in the swap chain
  //  UINT bufferCount = 0;

  //  // If this call ever fails, it probably means that one of the backbuffer related
  //  // resources needs to be Release()'d
  //  // ie: the rtv, srv, or the texture used to create it
  //  TAC_DXGI_CALL( errors, mSwapChain->ResizeBuffers,
  //                 bufferCount,
  //                 mDesktopWindow->mWidth,
  //                 mDesktopWindow->mHeight,
  //                 DXGI_FORMAT_UNKNOWN, // preserve existing format
  //                 DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH );
  //  CreateRenderTarget( errors );
  //  TAC_HANDLE_ERROR( errors );
  //}

  //void DX11Window::SwapBuffers( Errors & errors )
  //{
  //  bool mVsyncEnabled = true;
  //  UINT syncInterval = mVsyncEnabled ? 1 : 0;
  //  mSwapChain->Present( syncInterval, 0 );
  //  //mBufferIndex = ( mBufferIndex + 1 ) % mBackbufferColors.size();
  //}

  //void DX11Window::CreateRenderTarget( Errors& errors )
  //{
  //  auto renderer = ( RendererDirectX11* )Renderer::Instance;
  //  ID3D11Device* device = RendererDirectX11::Instance->mDevice;
  //  DXGI_SWAP_CHAIN_DESC desc;
  //  mSwapChain->GetDesc( &desc );

  //  // Color
  //  {
  //    ID3D11Texture2D* pBackBuffer;
  //    TAC_DXGI_CALL( errors, mSwapChain->GetBuffer, 0, IID_PPV_ARGS( &pBackBuffer ) );
  //    ID3D11RenderTargetView* rtv = nullptr;
  //    D3D11_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr;
  //    TAC_DX11_CALL( errors, device->CreateRenderTargetView,
  //                   pBackBuffer,
  //                   rtvDesc,
  //                   &rtv );
  //    TAC_HANDLE_ERRORS( errors );
  //    RendererDirectX11::Instance->SetDebugName( rtv, "backbuffer color rtv" );
  //    if( !mBackbufferColor )
  //    //{
  //    //  TextureData textureData = {};
  //    //  textureData.myImage.mWidth = desc.BufferDesc.Width;
  //    //  textureData.myImage.mHeight = desc.BufferDesc.Height;
  //    //  textureData.mName = "tac backbuffer color";
  //    //  textureData.mFrame = TAC_STACK_FRAME;
  //    //  Renderer::Instance->AddRendererResource( &mBackbufferColor, textureData );
  //    //}
  //    mBackbufferColor->mRTV = rtv;
  //    pBackBuffer->Release();
  //  }

  //  // Depth
  //  //{
  //  //  if( mDepthBuffer )
  //  //  {
  //  //    mDepthBuffer->width = desc.BufferDesc.Width;
  //  //    mDepthBuffer->height = desc.BufferDesc.Height;
  //  //    ( ( DepthBufferDX11* )mDepthBuffer )->Init( errors );
  //  //    TAC_HANDLE_ERROR( errors );
  //  //  }
  //  //  else
  //  //  {
  //  //    DepthBufferData depthBufferData;
  //  //    depthBufferData.width = desc.BufferDesc.Width;
  //  //    depthBufferData.height = desc.BufferDesc.Height;
  //  //    depthBufferData.mName = "backbuffer depth";
  //  //    depthBufferData.mFrame = TAC_STACK_FRAME;
  //  //    Renderer::Instance->AddDepthBuffer( &mDepthBuffer, depthBufferData, errors );
  //  //    TAC_HANDLE_ERROR( errors );
  //  //  }
  //  //}
  //}

  //void DX11Window::GetCurrentBackbufferTexture( Texture** texture )
  //{
  //  *texture = mBackbufferColor;
  //}

  RendererDirectX11* RendererDirectX11::Instance = nullptr;
  RendererDirectX11::RendererDirectX11()
  {
    RendererDirectX11::Instance = this;
  }
  RendererDirectX11::~RendererDirectX11()
  {
    mDxgi.Uninit();
    if( mDeviceContext )
    {
      mDeviceContext->Release();
      mDeviceContext = nullptr;
    }
    if( mDevice )
    {
      mDevice->Release();
      mDevice = nullptr;
    }
    if( mInfoQueueDEBUG )
    {
      mInfoQueueDEBUG->Release();
      mInfoQueueDEBUG = nullptr;
    }
    if( mUserAnnotationDEBUG )
    {
      mUserAnnotationDEBUG->Release();
      mUserAnnotationDEBUG = nullptr;
    }
    ReportLiveObjects();
  }
  void RendererDirectX11::Init( Errors& errors )
  {
    UINT createDeviceFlags = 0;
    if( IsDebugMode() )
      createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

    D3D_FEATURE_LEVEL featureLevel;
    auto featureLevelArray = MakeArray< D3D_FEATURE_LEVEL >( D3D_FEATURE_LEVEL_11_0 );

    IDXGIAdapter* pAdapter = NULL;
    D3D_DRIVER_TYPE DriverType = D3D_DRIVER_TYPE_HARDWARE;
    HMODULE Software = NULL;

    TAC_DX11_CALL( errors,
                   D3D11CreateDevice,
                   pAdapter,
                   DriverType,
                   Software,
                   createDeviceFlags,
                   featureLevelArray.data(),
                   featureLevelArray.size(),
                   D3D11_SDK_VERSION,
                   &mDevice,
                   &featureLevel,
                   &mDeviceContext );
    TAC_HANDLE_ERROR( errors );
    // If you're directx is crashing / throwing exception, don't forget to check
    // your output window, it likes to put error messages there
    if( IsDebugMode() )
    {
      TAC_DX11_CALL( errors, mDevice->QueryInterface, IID_PPV_ARGS( &mInfoQueueDEBUG ) );
      TAC_HANDLE_ERROR( errors );
      TAC_DX11_CALL( errors, mDeviceContext->QueryInterface, IID_PPV_ARGS( &mUserAnnotationDEBUG ) );
      TAC_HANDLE_ERROR( errors );
    }

    mDxgi.Init( errors );
    TAC_HANDLE_ERROR( errors );

  }
  //void RendererDirectX11::RenderFlush()
  //{
  //  AssertRenderThread();
  //  TAC_ASSERT( gThreadType == ThreadType::Main );
  //  mDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

  //  for( DrawCall2& drawCall : mDrawCall2s )
  //  {
  //    if( mCurrentlyBoundShader != drawCall.mShader )
  //    {
  //      auto shaderDX11 = ( ShaderDX11 * )drawCall.mShader;
  //      ID3D11VertexShader* vertexShader = nullptr;
  //      ID3D11PixelShader* pixelShader = nullptr;
  //      if( drawCall.mShader.IsValid() )
  //      {
  //        vertexShader = shaderDX11->mLoadData.mVertexShader;
  //        pixelShader = shaderDX11->mLoadData.mPixelShader;
  //        if( drawCall.mShader->mCBuffers.empty() )
  //          TAC_ASSERT_MESSAGE( "You probably forgot" );
  //        for( CBuffer* cbufferr : drawCall.mShader->mCBuffers )
  //        {
  //          auto cbuffer = ( CBufferDX11* )cbufferr;
  //          ID3D11Buffer* buffer = cbuffer->mDXObj;
  //          auto buffers = MakeArray< ID3D11Buffer* >( buffer );
  //          mDeviceContext->VSSetConstantBuffers( cbuffer->shaderRegister, buffers.size(), buffers.data() );
  //          mDeviceContext->PSSetConstantBuffers( cbuffer->shaderRegister, buffers.size(), buffers.data() );
  //        }
  //      }
  //      mDeviceContext->VSSetShader( vertexShader, nullptr, 0 );
  //      mDeviceContext->PSSetShader( pixelShader, nullptr, 0 );
  //      mCurrentlyBoundShader = shaderDX11;
  //    }

  //    if( mCurrentlyBoundVertexBuffer != drawCall.mVertexBuffer )
  //    {
  //      int startSlot = 0;
  //      const int NUM_VBOS = 16;
  //      ID3D11Buffer* vertexBufferHandles[ NUM_VBOS ] = {};
  //      UINT strides[ NUM_VBOS ] = {};
  //      UINT offsets[ NUM_VBOS ] = {};
  //      auto vertexBufferDX11 = ( VertexBufferDX11* )drawCall.mVertexBuffer;
  //      Vector< VertexBufferDX11* > vertexBuffers;
  //      if( vertexBufferDX11 )
  //      {
  //        vertexBuffers.push_back( vertexBufferDX11 );
  //      }
  //      auto vertexBufferCount = ( UINT )vertexBuffers.size();
  //      for( int i = 0; i < ( int )vertexBufferCount; ++i )
  //      {
  //        VertexBuffer* vertexBuffer = vertexBuffers[ i ];
  //        auto myVertexBufferDX11 = ( VertexBufferDX11* )vertexBuffer;
  //        ID3D11Buffer* vertexBufferHandle = myVertexBufferDX11->mDXObj;
  //        strides[ i ] = myVertexBufferDX11->mStrideBytesBetweenVertexes;
  //        vertexBufferHandles[ i ] = vertexBufferHandle;
  //      }

  //      mDeviceContext->IASetVertexBuffers(
  //        startSlot,
  //        vertexBufferCount,
  //        vertexBufferHandles,
  //        strides,
  //        offsets );
  //      mCurrentlyBoundVertexBuffer = ( VertexBufferDX11* )drawCall.mVertexBuffer;
  //    }

  //    if( mCurrentlyBoundIndexBuffer != drawCall.mIndexBuffer )
  //    {
  //      auto indexBufferDX11 = ( IndexBufferDX11* )drawCall.mIndexBuffer;
  //      ID3D11Buffer* buffer = nullptr;
  //      DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
  //      if( indexBufferDX11 )
  //      {
  //        buffer = indexBufferDX11->mDXObj;
  //        format = indexBufferDX11->mFormat;
  //      }
  //      mDeviceContext->IASetIndexBuffer( buffer, format, 0 );
  //      mCurrentlyBoundIndexBuffer = indexBufferDX11;

  //      if( false )
  //        std::cout << "changing index buffer to "
  //        << ( void* )indexBufferDX11
  //        << " "
  //        << indexBufferDX11->mIndexCount << std::endl;
  //    }

  //    if( mCurrentlyBoundBlendState != drawCall.mBlendState )
  //    {
  //      auto blendStateDX11 = ( BlendStateDX11* )drawCall.mBlendState;
  //      ID3D11BlendState* pBlendState = nullptr; // default blend state, overwrites dst with src pixels
  //      if( blendStateDX11 )
  //        pBlendState = blendStateDX11->mDXObj;
  //      v4 blendFactorRGBA = { 1.0f, 1.0f, 1.0f, 1.0f };
  //      uint32_t sampleMask = 0xffffffff;
  //      mDeviceContext->OMSetBlendState(
  //        pBlendState,
  //        blendFactorRGBA.data(),
  //        sampleMask );
  //      mCurrentlyBoundBlendState = blendStateDX11;
  //    }

  //    if( mCurrentlyBoundRasterizerState != drawCall.mRasterizerState )
  //    {
  //      auto rasterizerStateDX11 = ( RasterizerStateDX11* )drawCall.mRasterizerState;
  //      ID3D11RasterizerState* pRasterizerState = nullptr;
  //      if( rasterizerStateDX11 )
  //        pRasterizerState = rasterizerStateDX11->mDXObj;
  //      mDeviceContext->RSSetState( pRasterizerState );
  //      mCurrentlyBoundRasterizerState = rasterizerStateDX11;
  //    }

  //    if( mCurrentlyBoundDepthState != drawCall.mDepthState )
  //    {
  //      auto depthStateDX11 = ( DepthStateDX11* )drawCall.mDepthState;
  //      uint32_t stencilRef = 0;
  //      ID3D11DepthStencilState *pDepthStencilState = nullptr;
  //      if( depthStateDX11 )
  //        pDepthStencilState = depthStateDX11->mDXObj;
  //      mDeviceContext->OMSetDepthStencilState(
  //        pDepthStencilState,
  //        stencilRef );
  //      mCurrentlyBoundDepthState = depthStateDX11;
  //    }

  //    if( mCurrentlyBoundVertexFormat != drawCall.mVertexFormat )
  //    {
  //      auto vertexFormatDX11 = ( VertexFormatDX11* )drawCall.mVertexFormat;
  //      ID3D11InputLayout* pInputLayout = nullptr;
  //      if( vertexFormatDX11 )
  //        pInputLayout = vertexFormatDX11->mDXObj;
  //      mDeviceContext->IASetInputLayout( pInputLayout );
  //      mCurrentlyBoundVertexFormat = vertexFormatDX11;
  //    }

  //    if( drawCall.mUniformDst )
  //    {
  //      drawCall.mUniformDst->SendUniforms( drawCall.mUniformSrcc.data() );
  //    }

  //    if( mCurrentlyBoundView != drawCall.mRenderView )
  //    {
  //      if( drawCall.mRenderView )
  //      {
  //        // set & clear render target
  //        auto textureDX11 = ( TextureDX11* )drawCall.mRenderView->mFramebuffer;
  //        auto depthBufferDX11 = ( DepthBufferDX11* )drawCall.mRenderView->mFramebufferDepth;
  //        auto renderTargetView = ( ID3D11RenderTargetView* )textureDX11->mRTV;
  //        auto renderTargetViews = MakeArray<ID3D11RenderTargetView*>( renderTargetView );
  //        ID3D11DepthStencilView *pDepthStencilView = depthBufferDX11->mDSV;
  //        mDeviceContext->OMSetRenderTargets(
  //          ( UINT )renderTargetViews.size(),
  //          renderTargetViews.data(),
  //          pDepthStencilView );
  //        if( !Contains( mFrameBoundRenderViews, drawCall.mRenderView ) )
  //        {
  //          mFrameBoundRenderViews.push_back( drawCall.mRenderView );
  //          mDeviceContext->ClearRenderTargetView(
  //            renderTargetView,
  //            drawCall.mRenderView->mClearColorRGBA.data() );
  //          UINT clearFlags = D3D11_CLEAR_DEPTH; // | D3D11_CLEAR_STENCIL;
  //          FLOAT valueToClearDepthTo = 1.0f;
  //          mDeviceContext->ClearDepthStencilView( pDepthStencilView, clearFlags, valueToClearDepthTo, 0 );
  //        }


  //        // set scissor rect
  //        ScissorRect mScissorRect = drawCall.mRenderView->mScissorRect;
  //        D3D11_RECT r;
  //        r.left = ( LONG )mScissorRect.mXMinRelUpperLeftCornerPixel;
  //        r.top = ( LONG )mScissorRect.mYMinRelUpperLeftCornerPixel;
  //        r.right = ( LONG )mScissorRect.mXMaxRelUpperLeftCornerPixel;
  //        r.bottom = ( LONG )mScissorRect.mYMaxRelUpperLeftCornerPixel;
  //        mDeviceContext->RSSetScissorRects( 1, &r );

  //        // set viewport rect
  //        Viewport viewportRect = drawCall.mRenderView->mViewportRect;
  //        TAC_ASSERT( viewportRect.mViewportPixelWidthIncreasingRight > 0 );
  //        TAC_ASSERT( viewportRect.mViewportPixelHeightIncreasingUp > 0 );
  //        FLOAT TopLeftX = viewportRect.mViewportBottomLeftCornerRelFramebufferBottomLeftCornerX;
  //        FLOAT TopLeftY
  //          = textureDX11->myImage.mHeight
  //          - viewportRect.mViewportBottomLeftCornerRelFramebufferBottomLeftCornerY
  //          - viewportRect.mViewportPixelHeightIncreasingUp;
  //        D3D11_VIEWPORT vp;
  //        vp.Width = viewportRect.mViewportPixelWidthIncreasingRight;
  //        vp.Height = viewportRect.mViewportPixelHeightIncreasingUp;
  //        vp.MinDepth = 0.0f;
  //        vp.MaxDepth = 1.0f;
  //        vp.TopLeftX = TopLeftX;
  //        vp.TopLeftY = TopLeftY;
  //        mDeviceContext->RSSetViewports( 1, &vp );
  //      }

  //      mCurrentlyBoundView = drawCall.mRenderView;
  //    }

  //    if( !AreEqual( drawCall.mTextures, mCurrentlyBoundTextures ) )
  //    {
  //      Vector< ID3D11ShaderResourceView* > srvs;
  //      srvs.reserve( drawCall.mTextures.size() );
  //      for( const Tac::Texture* texture : drawCall.mTextures )
  //      {
  //        if( !texture )
  //          continue;
  //        auto textureDX11 = ( TextureDX11* )texture;
  //        ID3D11ShaderResourceView* srv = textureDX11->mSrv;
  //        if( !srv )
  //          TAC_ASSERT_MESSAGE( "%s should be created with shader bind flags", textureDX11->mName.c_str() );
  //        srvs.push_back( srv );
  //      }

  //      mDeviceContext->VSSetShaderResources( 0, srvs.size(), srvs.data() );
  //      mDeviceContext->PSSetShaderResources( 0, srvs.size(), srvs.data() );
  //      mCurrentlyBoundTextures = drawCall.mTextures;
  //    }

  //    if( drawCall.mSamplerState != mCurrentlyBoundSamplerState )
  //    {
  //      auto samplerStateDX11 = ( SamplerStateDX11* )drawCall.mSamplerState;
  //      auto samplerStates = MakeArray< ID3D11SamplerState* >(
  //        samplerStateDX11 ? samplerStateDX11->mDXObj : nullptr );
  //      mDeviceContext->VSSetSamplers( 0, samplerStates.size(), samplerStates.data() );
  //      mDeviceContext->PSSetSamplers( 0, samplerStates.size(), samplerStates.data() );
  //      mCurrentlyBoundSamplerState = samplerStateDX11;
  //    }

  //    static PrimitiveTopology primitiveTopology = PrimitiveTopology::Count;
  //    if( drawCall.mPrimitiveTopology != primitiveTopology )
  //    {
  //      primitiveTopology = drawCall.mPrimitiveTopology;
  //      D3D11_PRIMITIVE_TOPOLOGY primtopdx11 = GetPrimTop( primitiveTopology );
  //      mDeviceContext->IASetPrimitiveTopology( primtopdx11 );
  //    }

  //    if( drawCall.mIndexBuffer && drawCall.mIndexCount )
  //    {
  //      TAC_ASSERT( mCurrentlyBoundShader );
  //      mDeviceContext->DrawIndexed( drawCall.mIndexCount, drawCall.mStartIndex, 0 );
  //    }
  //    else if( drawCall.mVertexBuffer && drawCall.mVertexCount )
  //    {
  //      mDeviceContext->Draw( drawCall.mVertexCount, 0 );
  //    }
  //  }
  //  mDrawCall2s.clear();
  //}
  //void RendererDirectX11::Render( Errors& errors )
  //{
  //  AssertRenderThread();
  //  TAC_PROFILE_BLOCK;
  //  RenderFlush();

  //  for( DX11Window* window : mWindows )
  //  {
  //    window->SwapBuffers( errors );
  //    TAC_HANDLE_ERROR( errors );
  //  }

  //  mCurrentlyBoundView = nullptr;
  //  mFrameBoundRenderViews.clear();
  //}

  static void PopCheep( const char*& bufferPos )
  {
    TAC_ASSERT( bufferPos[ 0 ] == 'e' );
    TAC_ASSERT( bufferPos[ 1 ] == 'n' );
    TAC_ASSERT( bufferPos[ 2 ] == 'd' );
    bufferPos += 3;
  }

  void RendererDirectX11::Render2( Render::Frame* frame, Errors& errors )
  {
    // factor out this while loop out of rendererDX11 and rendererOGL
    const char* bufferBegin = frame->mCommandBuffer.mBuffer.data();
    const char* bufferEnd = bufferBegin + frame->mCommandBuffer.mBuffer.size();
    const char* bufferPos = bufferBegin;

    //Render::CommandDataCreateTexture* commandDataCreateTexture = nullptr;
    //Render::CommandDataCreateFramebuffer* commandDataCreateFramebuffer = nullptr;
    //Render::CommandDataUpdateTextureRegion* commandDataUpdateTextureRegion = nullptr;
    //Render::CommandDataUpdateBuffer* commandDataUpdateBuffer = nullptr;

    static int i = 0;
    ++i;

    while( bufferPos < bufferEnd )
    {
      auto renderCommandType = ( Render::CommandType* )bufferPos;
      bufferPos += sizeof( Render::CommandType );

      auto stackFrame = ( StackFrame* )bufferPos;
      bufferPos += sizeof( StackFrame );
      TAC_UNUSED_PARAMETER( stackFrame );


      switch( *renderCommandType )
      {
        case Render::CommandType::CreateVertexBuffer:
        {
          auto resourceId = ( Render::VertexBufferHandle* )bufferPos;
          bufferPos += sizeof( Render::VertexBufferHandle );
          auto commandDataCreateBuffer = ( Render::CommandDataCreateVertexBuffer* ) bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateVertexBuffer );
          PopCheep( bufferPos );
          AddVertexBuffer( *resourceId, commandDataCreateBuffer, errors );
        } break;

        case Render::CommandType::CreateIndexBuffer:
        {
          auto resourceId = ( Render::IndexBufferHandle* )bufferPos;
          bufferPos += sizeof( Render::IndexBufferHandle );
          auto commandDataCreateBuffer = ( Render::CommandDataCreateIndexBuffer* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateIndexBuffer );
          PopCheep( bufferPos );
          AddIndexBuffer( *resourceId, commandDataCreateBuffer, errors );
        } break;

        case Render::CommandType::CreateTexture:
        {
          auto resourceId = ( Render::TextureHandle* )bufferPos;
          bufferPos += sizeof( Render::TextureHandle );
          auto commandData = ( Render::CommandDataCreateTexture* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateTexture );
          PopCheep( bufferPos );
          AddTexture( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::CreateFramebuffer:
        {
          auto resourceId = ( Render::FramebufferHandle* )bufferPos;
          bufferPos += sizeof( Render::FramebufferHandle );
          auto commandData = ( Render::CommandDataCreateFramebuffer* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateFramebuffer );
          PopCheep( bufferPos );
          AddFramebuffer( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::DestroyVertexBuffer:
        {
          auto resourceId = ( Render::VertexBufferHandle* )bufferPos;
          bufferPos += sizeof( Render::VertexBufferHandle );
          PopCheep( bufferPos );
          RemoveVertexBuffer( *resourceId, errors );
        } break;

        case Render::CommandType::DestroyIndexBuffer:
        {
          auto resourceId = ( Render::IndexBufferHandle* )bufferPos;
          bufferPos += sizeof( Render::IndexBufferHandle );
          PopCheep( bufferPos );
          RemoveIndexBuffer( *resourceId, errors );
        } break;

        case Render::CommandType::DestroyTexture:
        {
          auto resourceId = ( Render::TextureHandle* )bufferPos;
          bufferPos += sizeof( Render::TextureHandle );
          RemoveTexture( *resourceId, errors );
          PopCheep( bufferPos );
        } break;

        case Render::CommandType::DestroyFramebuffer:
        {
          auto resourceId = ( Render::FramebufferHandle* )bufferPos;
          bufferPos += sizeof( Render::FramebufferHandle );
          PopCheep( bufferPos );
          RemoveFramebuffer( *resourceId, errors );
        } break;

        case Render::CommandType::UpdateTextureRegion:
        {
          auto resourceId = ( Render::TextureHandle* )bufferPos;
          bufferPos += sizeof( Render::TextureHandle );
          auto commandData = ( Render::CommandDataUpdateTextureRegion* )bufferPos;
          bufferPos += sizeof( Render::CommandDataUpdateTextureRegion );
          PopCheep( bufferPos );
          UpdateTextureRegion( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::UpdateVertexBuffer:
        {
          auto resourceId = ( Render::VertexBufferHandle* )bufferPos;
          bufferPos += sizeof( Render::VertexBufferHandle );
          auto commandData = ( Render::CommandDataUpdateBuffer* )bufferPos;
          bufferPos += sizeof( Render::CommandDataUpdateBuffer );
          PopCheep( bufferPos );

          ID3D11Buffer* buffer = mVertexBuffers[ resourceId->mResourceId ].mBuffer;
          UpdateBuffer( buffer, commandData->mBytes, commandData->mByteCount, errors );
        } break;

        case Render::CommandType::UpdateIndexBuffer:
        {
          auto resourceId = ( Render::IndexBufferHandle* )bufferPos;
          bufferPos += sizeof( Render::IndexBufferHandle );
          auto commandData = ( Render::CommandDataUpdateBuffer* )bufferPos;
          bufferPos += sizeof( Render::CommandDataUpdateBuffer );
          PopCheep( bufferPos );
          ID3D11Buffer* buffer = mIndexBuffers[ resourceId->mResourceId ].mBuffer;
          UpdateBuffer( buffer, commandData->mBytes, commandData->mByteCount, errors );
        } break;

        //case Render::CommandType::UpdateConstantBuffer:
        //{
        //  auto resourceId = ( Render::ConstantBufferHandle* )bufferPos;
        //  bufferPos += sizeof( Render::ConstantBufferHandle );
        //  auto commandData = ( Render::CommandDataUpdateBuffer* )bufferPos;
        //  bufferPos += sizeof( Render::CommandDataUpdateBuffer );
        //  PopCheep( bufferPos );
        //  UpdateConstantBuffer( *resourceId, commandData, errors );
        //} break;

        case Render::CommandType::CreateBlendState:
        {
          auto resourceId = ( Render::BlendStateHandle* )bufferPos;
          bufferPos += sizeof( Render::BlendStateHandle );
          auto commandData = ( Render::CommandDataCreateBlendState* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateBlendState );
          PopCheep( bufferPos );
          AddBlendState( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::CreateConstantBuffer:
        {
          auto resourceId = ( Render::ConstantBufferHandle* )bufferPos;
          bufferPos += sizeof( Render::ConstantBufferHandle );
          auto commandData = ( Render::CommandDataCreateConstantBuffer* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateConstantBuffer );
          PopCheep( bufferPos );
          AddConstantBuffer( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::CreateDepthState:
        {
          auto resourceId = ( Render::DepthStateHandle* )bufferPos;
          bufferPos += sizeof( Render::DepthStateHandle );
          auto commandData = ( Render::CommandDataCreateDepthState* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateDepthState );
          PopCheep( bufferPos );
          AddDepthState( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::CreateRasterizerState:
        {
          auto resourceId = ( Render::RasterizerStateHandle* )bufferPos;
          bufferPos += sizeof( Render::RasterizerStateHandle );
          auto commandData = ( Render::CommandDataCreateRasterizerState* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateRasterizerState );
          PopCheep( bufferPos );
          AddRasterizerState( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::CreateSamplerState:
        {
          auto resourceId = ( Render::SamplerStateHandle* )bufferPos;
          bufferPos += sizeof( Render::SamplerStateHandle );
          auto commandData = ( Render::CommandDataCreateSamplerState* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateSamplerState );
          PopCheep( bufferPos );
          AddSamplerState( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::CreateShader:
        {
          auto resourceId = ( Render::ShaderHandle* )bufferPos;
          bufferPos += sizeof( Render::ShaderHandle );
          auto commandData = ( Render::CommandDataCreateShader* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateShader );
          PopCheep( bufferPos );
          AddShader( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::CreateVertexFormat:
        {
          auto resourceId = ( Render::VertexFormatHandle* )bufferPos;
          bufferPos += sizeof( Render::VertexFormatHandle );
          auto commandData = ( Render::CommandDataCreateVertexFormat* )bufferPos;
          bufferPos += sizeof( Render::CommandDataCreateVertexFormat );
          PopCheep( bufferPos );
          AddVertexFormat( *resourceId, commandData, errors );
        } break;

        case Render::CommandType::DestroyBlendState:
        {
          auto resourceId = ( Render::BlendStateHandle* )bufferPos;
          bufferPos += sizeof( Render::BlendStateHandle );
          PopCheep( bufferPos );
          RemoveBlendState( *resourceId, errors );
        } break;

        case Render::CommandType::DestroyConstantBuffer:
        {
          auto resourceId = ( Render::ConstantBufferHandle* )bufferPos;
          bufferPos += sizeof( Render::ConstantBufferHandle );
          PopCheep( bufferPos );
          RemoveConstantBuffer( *resourceId, errors );
        } break;

        case Render::CommandType::DestroyDepthState:
        {
          auto resourceId = ( Render::DepthStateHandle* )bufferPos;
          bufferPos += sizeof( Render::DepthStateHandle );
          PopCheep( bufferPos );
          RemoveDepthState( *resourceId, errors );
        } break;

        case Render::CommandType::DestroyRasterizerState:
        {
          auto resourceId = ( Render::RasterizerStateHandle* )bufferPos;
          bufferPos += sizeof( Render::RasterizerStateHandle );
          PopCheep( bufferPos );
          RemoveRasterizerState( *resourceId, errors );
        } break;

        case Render::CommandType::DestroySamplerState:
        {
          auto resourceId = ( Render::SamplerStateHandle* )bufferPos;
          bufferPos += sizeof( Render::SamplerStateHandle );
          PopCheep( bufferPos );
          RemoveSamplerState( *resourceId, errors );
        } break;

        case Render::CommandType::DestroyShader:
        {
          auto resourceId = ( Render::ShaderHandle* )bufferPos;
          bufferPos += sizeof( Render::ShaderHandle );
          PopCheep( bufferPos );
          RemoveShader( *resourceId, errors );
        } break;

        case Render::CommandType::DestroyVertexFormat:
        {
          auto resourceId = ( Render::VertexFormatHandle* )bufferPos;
          bufferPos += sizeof( Render::VertexFormatHandle );
          PopCheep( bufferPos );
          RemoveVertexFormat( *resourceId, errors );
        } break;

        default:
          TAC_UNIMPLEMENTED;
      }
    }

    ID3D11BlendState* blendState = nullptr;
    ID3D11DepthStencilState* depthStencilState = nullptr;
    for( int iDrawCall = 0; iDrawCall < frame->mDrawCallCount; ++iDrawCall )
    {
      Render::DrawCall3* drawCall = &frame->mDrawCalls[ iDrawCall ];
      drawCall->mBlendStateHandle;

      if( drawCall->mBlendStateHandle.IsValid() && blendState != mBlendStates[ drawCall->mBlendStateHandle.mResourceId ] )
      {
        blendState = mBlendStates[ drawCall->mBlendStateHandle.mResourceId ];
        const FLOAT blendFactorRGBA[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        const UINT sampleMask = 0xffffffff;
        mDeviceContext->OMSetBlendState( blendState, blendFactorRGBA, sampleMask );
      }

      if( drawCall->mDepthStateHandle.IsValid() && depthStencilState != mDepthStencilStates[ drawCall->mDepthStateHandle.mResourceId ] )
      {
        depthStencilState = mDepthStencilStates[ drawCall->mDepthStateHandle.mResourceId ];
        const UINT stencilRef = 0;
        mDeviceContext->OMSetDepthStencilState( depthStencilState, stencilRef );
      }

      if( drawCall->mIndexBufferHandle.IsValid() )
      {
        const IndexBuffer* indexBuffer = &mIndexBuffers[ drawCall->mIndexBufferHandle.mResourceId ];
        const DXGI_FORMAT dxgiFormat = GetDXGIFormat( indexBuffer->mFormat );
        const UINT byteOffset = drawCall->mStartIndex * indexBuffer->mFormat.mPerElementByteCount;
        mDeviceContext->IASetIndexBuffer( indexBuffer->mBuffer,
                                          dxgiFormat,
                                          byteOffset );
      }

      if( drawCall->mVertexBufferHandle.IsValid() )
      {
        const VertexBuffer* vertexBuffer = &mVertexBuffers[ drawCall->mVertexBufferHandle.mResourceId ];
        const UINT startSlot = 0;
        const UINT NumBuffers = 1;
        const UINT Strides[ NumBuffers ] = { ( UINT )vertexBuffer->mStride };
        const UINT ByteOffsets[ NumBuffers ] = { ( UINT )( drawCall->mStartVertex * vertexBuffer->mStride ) };
        ID3D11Buffer* buffers[ NumBuffers ] = { vertexBuffer->mBuffer };
        mDeviceContext->IASetVertexBuffers( startSlot,
                                            NumBuffers,
                                            buffers,
                                            Strides,
                                            ByteOffsets );
      }

      if( drawCall->mRasterizerStateHandle.IsValid() )
      {
        ID3D11RasterizerState* rasterizerState = mRasterizerStates[ drawCall->mRasterizerStateHandle.mResourceId ];
        mDeviceContext->RSSetState( rasterizerState );
      }

      if( drawCall->mSamplerStateHandle.IsValid() )
      {
        const UINT StartSlot = 0;
        const UINT NumSamplers = 1;
        ID3D11SamplerState* Samplers[] = { mSamplerStates[ drawCall->mSamplerStateHandle.mResourceId ] };
        mDeviceContext->VSSetSamplers( StartSlot, NumSamplers, Samplers );
        mDeviceContext->PSSetSamplers( StartSlot, NumSamplers, Samplers );
      }

      for( int iConstantBuffer = 0;
           iConstantBuffer < drawCall->mUpdateConstantBuffers.mUpdateConstantBufferDataCount;
           ++iConstantBuffer )
      {
        Render::UpdateConstantBuffers::UpdateConstantBuffer* stuff =
          &drawCall->mUpdateConstantBuffers.mUpdateConstantBufferDatas[ iConstantBuffer ];
        ID3D11Buffer* buffer = mConstantBuffers[ stuff->mConstantBufferHandle.mResourceId ].mBuffer;
        stuff->mData.mBytes;
        UpdateBuffer( buffer, stuff->mData.mBytes, stuff->mData.mByteCount, errors );
      }
    }
  }
  void RendererDirectX11::SwapBuffers()
  {
    for( int iFramebuffer = 0; iFramebuffer < Render::kMaxFramebuffers; ++iFramebuffer )
    {
      Framebuffer* framebuffer = &mFramebuffers[ iFramebuffer ];
      if( !framebuffer->mSwapChain )
        continue;
      const UINT SyncInterval = 0;
      const UINT Flags = 0;
      framebuffer->mSwapChain->Present( SyncInterval, Flags );
    }
  }

  //void RendererDirectX11::CreateWindowContext( DesktopWindow* desktopWindow, Errors& errors )
  //{
  //  AssertRenderThread();
  //  auto hwnd = ( HWND )desktopWindow->mOperatingSystemHandle;
  //  IUnknown* pDevice = mDevice;
  //  IDXGISwapChain* mSwapChain;
  //  int bufferCount = 4;
  //  mDxgi.CreateSwapChain( hwnd,
  //                         pDevice,
  //                         bufferCount,
  //                         desktopWindow->mWidth,
  //                         desktopWindow->mHeight,
  //                         &mSwapChain,
  //                         errors );
  //  TAC_HANDLE_ERROR( errors );
  //  auto dx11Window = new DX11Window();
  //  dx11Window->mSwapChain = mSwapChain;
  //  dx11Window->CreateRenderTarget( errors );
  //  dx11Window->mDesktopWindow = desktopWindow;
  //  TAC_HANDLE_ERROR( errors );
  //  mWindows.push_back( dx11Window );
  //  desktopWindow->mRendererData = dx11Window;
  //  desktopWindow->mOnDestroyed.AddCallbackFunctional( []( DesktopWindow* desktopWindow )
  //                                                     {
  //                                                       RendererDirectX11* renderer = RendererDirectX11::Instance;
  //                                                       auto dx11Window = ( DX11Window* )desktopWindow->mRendererData;
  //                                                       for( DX11Window*& window : renderer->mWindows )
  //                                                       {
  //                                                         if( window != dx11Window )
  //                                                           continue;
  //                                                         delete window;
  //                                                         window = renderer->mWindows.back();
  //                                                         renderer->mWindows.pop_back();
  //                                                         break;
  //                                                       }
  //                                                     } );
  //}

  //void RendererDirectX11::AddVertexBuffer( VertexBuffer** outputVertexBuffer, const VertexBufferData& vbData, Errors& errors )
  //{
  //  AssertRenderThread();
  //  D3D11_BUFFER_DESC bd = {};
  //  bd.ByteWidth = vbData.mNumVertexes * vbData.mStrideBytesBetweenVertexes;
  //  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  //  bd.Usage = GetUsage( vbData.mAccess );
  //  if( vbData.mAccess == Access::Dynamic )
  //    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  //  D3D11_SUBRESOURCE_DATA initData = {};
  //  D3D11_SUBRESOURCE_DATA* pInitialData = nullptr;
  //  if( vbData.mOptionalData )
  //  {
  //    initData.pSysMem = vbData.mOptionalData;
  //    pInitialData = &initData;
  //  }
  //  ID3D11Buffer* buffer;
  //  TAC_DX11_CALL( errors, mDevice->CreateBuffer, &bd, pInitialData, &buffer );
   // TAC_HANDLE_ERROR( errors );
  //  SetDebugName( buffer, vbData.mName + " verts" );
  //  VertexBufferDX11* vertexBuffer;
  //  AddRendererResource( &vertexBuffer, vbData );
  //  vertexBuffer->mDXObj = buffer;
  //  *outputVertexBuffer = vertexBuffer;
  //}

  //void RendererDirectX11::AddIndexBuffer(
  //  IndexBuffer** outputIndexBuffer,
  //  const IndexBufferData& indexBufferData,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  TAC_ASSERT( indexBufferData.mIndexCount > 0 );
  //  UINT totalBufferSize
  //    = indexBufferData.mIndexCount
  //    * indexBufferData.mFormat.mPerElementByteCount
  //    * indexBufferData.mFormat.mElementCount;
  //  D3D11_BUFFER_DESC bd = {};
  //  bd.ByteWidth = totalBufferSize;
  //  bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  //  bd.Usage = GetUsage( indexBufferData.mAccess );
  //  if( indexBufferData.mAccess == Access::Dynamic )
  //    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  //  ID3D11Buffer* mDXObj = nullptr;
  //  D3D11_SUBRESOURCE_DATA initData = {};
  //  D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
  //  if( indexBufferData.mData )
  //  {
  //    pInitData = &initData;
  //    initData.pSysMem = indexBufferData.mData;
  //  }
  //  TAC_DX11_CALL( errors, mDevice->CreateBuffer, &bd, pInitData, &mDXObj );

    //TAC_HANDLE_ERROR( errors );
  //  SetDebugName( mDXObj, indexBufferData.mName + " indexes" );
  //  IndexBufferDX11* indexBuffer;
  //  AddRendererResource( &indexBuffer, indexBufferData );
  //  indexBuffer->mFormat = GetDXGIFormat( indexBufferData.mFormat );
  //  indexBuffer->mDXObj = mDXObj;
  //  *outputIndexBuffer = indexBuffer;
  //}

  //void RendererDirectX11::ClearColor(
  //  Tac::Texture* texture,
  //  v4 rgba )
  //{
  //  AssertRenderThread();
  //  TAC_ASSERT( texture );
  //  auto* textureDX11 = ( TextureDX11* )texture;
  //  TAC_ASSERT( textureDX11->mRTV );
  //  mDeviceContext->ClearRenderTargetView(
  //    textureDX11->mRTV,
  //    &rgba[ 0 ] );
  //}

  //void RendererDirectX11::ClearDepthStencil(
  //  DepthBuffer* depthBuffer,
  //  bool shouldClearDepth,
  //  float depth,
  //  bool shouldClearStencil,
  //  uint8_t stencil )
  //{
  //  AssertRenderThread();
  //  UINT clearFlags = 0;
  //  if( shouldClearDepth ) clearFlags |= D3D11_CLEAR_DEPTH;
  //  if( shouldClearStencil ) clearFlags |= D3D11_CLEAR_STENCIL;
  //  TAC_ASSERT( depthBuffer );
  //  auto depthBufferDX11 = ( DepthBufferDX11* )depthBuffer;
  //  ID3D11DepthStencilView *pDepthStencilView = depthBufferDX11->mDSV;
  //  TAC_ASSERT( pDepthStencilView );
  //  mDeviceContext->ClearDepthStencilView( pDepthStencilView, clearFlags, depth, stencil );
  //}

  static void CompileShaderFromString(
    ID3DBlob** ppBlobOut,
    const String& shaderStr,
    const char* entryPoint,
    const char* shaderModel,
    Errors& errors )
  {
    AssertRenderThread();
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    if( IsDebugMode() )
    {
      dwShaderFlags |= D3DCOMPILE_DEBUG;
      dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    }

    ID3DBlob* pErrorBlob;
    HRESULT hr = D3DCompile(
      shaderStr.data(),
      shaderStr.size(),
      nullptr,
      nullptr, // D3D_SHADER_MACRO* pDefines,
      nullptr, // ID3DInclude* pInclude,
      entryPoint,
      shaderModel,
      dwShaderFlags,
      0,
      ppBlobOut,
      &pErrorBlob );
    if( FAILED( hr ) )
    {
      errors += ( const char* )pErrorBlob->GetBufferPointer();
    }
  }
  //void RendererDirectX11::AddShader( Shader** outputShader, const ShaderData& shaderData, Errors& errors )
  //{
  //  AssertRenderThread();
  //  for( CBuffer* cbuf : shaderData.mCBuffers )
  //    TAC_ASSERT( cbuf );
  //  ShaderDX11* shader;
  //  AddRendererResource( &shader, shaderData );
  //  if( !shader->mShaderPath.empty() )
  //  {
  //    shader->mShaderPath = GetDirectX11ShaderPath( shader->mShaderPath );
  //    for( ;; )
  //    {
  //      ReloadShader( shader, errors );
  //      if( IsDebugMode() && errors )
  //      {
  //        DebugBreak();
  //        errors = "";
  //        continue;
  //      }
  //      break;
  //    }
  //  }
  //  else if( !shader->mShaderStr.empty() )
  //  {
  //    LoadShaderInternal( &shader->mLoadData, shader->mName, shader->mShaderStr, errors );
  //    if( IsDebugMode() )
  //    {
  //      TAC_ASSERT( errors.empty() );
  //    }
  //  }
  //  else
  //  {
  //    TAC_INVALID_CODE_PATH;
  //  }
  //  mShaders.insert( shader );
  //  *outputShader = shader;
  //}

  void RendererDirectX11::LoadShaderInternal(
    ShaderDX11LoadData* loadData,
    String name,
    String str,
    Errors& errors )
  {
    AssertRenderThread();
    auto temporaryMemory = TemporaryMemoryFromFile( GetDirectX11ShaderPath( "common" ), errors );
    TAC_HANDLE_ERROR( errors );

    String common( temporaryMemory.data(), ( int )temporaryMemory.size() );
    str = common + str;

    // vertex shader
    {
      ID3DBlob* pVSBlob;

      CompileShaderFromString(
        &pVSBlob,
        str,
        "VS",
        "vs_4_0",
        errors );
      TAC_HANDLE_ERROR( errors );
      TAC_ON_DESTRUCT( pVSBlob->Release() );

      TAC_DX11_CALL(
        errors,
        mDevice->CreateVertexShader,
        pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(),
        nullptr,
        &loadData->mVertexShader );
      SetDebugName( loadData->mVertexShader, name + " vtx shader" );

      TAC_DX11_CALL(
        errors,
        D3DGetBlobPart,
        pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(),
        D3D_BLOB_INPUT_SIGNATURE_BLOB,
        0,
        &loadData->mInputSig );
    }

    // pixel shader
    {
      ID3DBlob* pPSBlob;
      CompileShaderFromString(
        &pPSBlob,
        str,
        "PS",
        "ps_4_0",
        errors );
      TAC_HANDLE_ERROR( errors );
      TAC_ON_DESTRUCT( pPSBlob->Release() );

      TAC_DX11_CALL(
        errors,
        mDevice->CreatePixelShader,
        pPSBlob->GetBufferPointer(),
        pPSBlob->GetBufferSize(),
        nullptr,
        &loadData->mPixelShader );
      SetDebugName( loadData->mPixelShader, name + " px shader" );
    }
  }

  //void RendererDirectX11::ReloadShader( Shader* shader, Errors& errors )
  //{
  //  AssertRenderThread();
  //  TAC_ASSERT( shader );
  //  auto* shaderDX11 = ( ShaderDX11* )shader;
  //  if( shaderDX11->mShaderPath.empty() )
  //    return;
  //  auto temporaryMemory = TemporaryMemoryFromFile( shaderDX11->mShaderPath, errors );
  //  TAC_HANDLE_ERROR( errors );
  //  String shaderStr( temporaryMemory.data(), ( int )temporaryMemory.size() );
  //  ShaderDX11LoadData loadData;
  //  LoadShaderInternal( &loadData, shader->mName, shaderStr, errors );
  //  TAC_HANDLE_ERROR( errors );
  //  shaderDX11->mLoadData.Release();
  //  shaderDX11->mLoadData = loadData;
  //}

  //void RendererDirectX11::GetShaders( Vector< Shader* >&shaders )
  //{
  //  AssertRenderThread();
  //  for( auto shader : mShaders )
  //    shaders.push_back( shader );
  //}

  //void RendererDirectX11::AddSamplerState(
  //  SamplerState** outputSamplerState,
  //  const SamplerStateData& samplerStateData,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  D3D11_SAMPLER_DESC desc = {};
  //  desc.Filter = GetFilter( samplerStateData.filter );
  //  desc.AddressU = GetAddressMode( samplerStateData.u );
  //  desc.AddressV = GetAddressMode( samplerStateData.v );
  //  desc.AddressW = GetAddressMode( samplerStateData.w );
  //  desc.ComparisonFunc = GetCompare( samplerStateData.compare );
  //  desc.BorderColor[ 0 ] = 1;
  //  desc.BorderColor[ 1 ] = 0;
  //  desc.BorderColor[ 2 ] = 0;
  //  desc.BorderColor[ 3 ] = 1;
  //  ID3D11SamplerState* samplerState;
  //  TAC_DX11_CALL( errors, mDevice->CreateSamplerState, &desc, &samplerState );
   // TAC_HANDLE_ERROR( errors );
  //  SetDebugName( samplerState, samplerStateData.mName );
  //  SamplerStateDX11* samplerStateDX11;
  //  AddRendererResource( &samplerStateDX11, samplerStateData );
  //  samplerStateDX11->mDXObj = samplerState;
  //  *outputSamplerState = samplerStateDX11;
  //}

  //void RendererDirectX11::AddSampler(
  //  const String& name,
  //  Shader* shader,
  //  ShaderType shaderType,
  //  int samplerIndex )
  //{
  //  AssertRenderThread();
  //  auto* shaderDX11 = ( ShaderDX11* )shader;
  //  auto mySampler = new Sampler();
  //  mySampler->mName = name;
  //  mySampler->mSamplerIndex = samplerIndex;
  //  mySampler->mShaderType = shaderType;
  //  shaderDX11->mSamplers.push_back( mySampler );
  //}

  //void RendererDirectX11::SetSamplerState(
  //  const String& samplerName,
  //  SamplerState* samplerState )
  //{
  //  AssertRenderThread();
    //Assert( mCurrentShader );
    //auto sampler = mCurrentShader->FindSampler( samplerName );
    //Assert( sampler );
    //auto& samplers = mCurrentSamplers[ sampler->mShaderType ];
    //int requiredSize = sampler->mSamplerIndex + 1;
    //if( samplers.size() < requiredSize )
    //  samplers.resize( requiredSize );
    //samplers[ sampler->mSamplerIndex ] = ( SamplerStateDX11* )samplerState;
    //mCurrentSamplersDirty.insert( sampler->mShaderType );
  //}

  //void RendererDirectX11::AddTextureResourceCube(
  //  Tac::Texture** texture,
  //  const TextureData& textureData,
  //  void** sixCubeDatas,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  const Image& myImage = textureData.myImage;
  //  D3D11_TEXTURE2D_DESC texDesc = {};
  //  texDesc.Width = myImage.mWidth;
  //  texDesc.Height = myImage.mHeight;
  //  texDesc.MipLevels = 1;
  //  texDesc.SampleDesc.Count = 1;
  //  texDesc.ArraySize = 6;
  //  texDesc.Format = GetDXGIFormat( myImage.mFormat );
  //  texDesc.Usage = GetUsage( textureData.access );
  //  texDesc.BindFlags = GetBindFlags( textureData.binding );
  //  texDesc.CPUAccessFlags = GetCPUAccessFlags( textureData.cpuAccess );
  //  texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
  //  // D3D11_SUBRESOURCE_DATA struct Ure
  //  // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476220(v=vs.85).aspx
  //  // You set SysMemPitch to the distance between any two adjacent pixels on different lines.
  //  // You set SysMemSlicePitch to the size of the entire 2D surface in bytes.
  //  D3D11_SUBRESOURCE_DATA subResources[ 6 ] = {};
  //  D3D11_SUBRESOURCE_DATA* pSubResource = nullptr;
  //  if( sixCubeDatas )
  //  {
  //    pSubResource = subResources;
  //    for( int i = 0; i < 6; ++i )
  //    {
  //      D3D11_SUBRESOURCE_DATA& subResource = subResources[ i ];
  //      subResource.pSysMem = sixCubeDatas[ i ];
  //      subResource.SysMemPitch = myImage.mPitch;
  //      subResource.SysMemSlicePitch = myImage.mPitch * myImage.mHeight; // <-- I guess
  //    }
  //  }
  //  ID3D11Resource* resource = nullptr;
  //  TAC_DX11_CALL( errors, mDevice->CreateTexture2D, &texDesc, pSubResource, ( ID3D11Texture2D** )&resource );
// TAC_HANDLE_ERROR( errors );
  //  SetDebugName( resource, textureData.mName + " tex2d" );
  //  ID3D11RenderTargetView* rTV = nullptr;
  //  if( Contains( textureData.binding, Binding::RenderTarget ) )
  //  {
  //    TAC_DX11_CALL( errors, mDevice->CreateRenderTargetView,
  //                   resource,
  //                   nullptr,
  //                   &rTV );
// TAC_HANDLE_ERROR( errors );
  //    SetDebugName( rTV, textureData.mName + " rtv" );
  //  }
  //  ID3D11ShaderResourceView* srv = nullptr;
  //  if( Contains( textureData.binding, Binding::ShaderResource ) )
  //  {
  //    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  //    srvDesc.Format = texDesc.Format;
  //    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
  //    srvDesc.TextureCube.MostDetailedMip = 0;
  //    srvDesc.TextureCube.MipLevels = -1;
  //    TAC_DX11_CALL( errors, mDevice->CreateShaderResourceView, resource, &srvDesc, &srv );
// TAC_HANDLE_ERROR( errors );
  //    SetDebugName( srv, textureData.mName + " srv" );
  //  }
  //  TextureDX11* textureDX11;
  //  AddRendererResource( &textureDX11, textureData );
  //  textureDX11->mRTV = rTV;
  //  textureDX11->mSrv = srv;
  //  textureDX11->mDXObj = resource;
  //  *texture = textureDX11;
  //}

  //void RendererDirectX11::AddTextureResource(
  //  Tac::Texture** outputTexture,
  //  const TextureData& textureData,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  ID3D11Resource* dXObj;
  //  CreateTexture(
  //    textureData.myImage,
  //    textureData.mOptionalImageBytes,
  //    &dXObj,
  //    textureData.access,
  //    textureData.cpuAccess,
  //    textureData.binding,
  //    textureData.mName,
  //    errors );
  //  TAC_HANDLE_ERROR( errors );
  //  SetDebugName( dXObj, textureData.mName + " tex2d" );
  //  ID3D11RenderTargetView* rTV = nullptr;
  //  if( Contains( textureData.binding, Binding::RenderTarget ) )
  //  {
  //    TAC_DX11_CALL( errors, mDevice->CreateRenderTargetView,
// TAC_HANDLE_ERROR( errors );
  //                   dXObj,
  //                   nullptr,
  //                   &rTV );
  //    SetDebugName( rTV, textureData.mName + " rtv" );
  //  }
  //  ID3D11ShaderResourceView* srv = nullptr;
  //  if( Contains( textureData.binding, Binding::ShaderResource ) )
  //  {
  //    CreateShaderResourceView(
  //      dXObj,
  //      &srv,
  //      textureData.mName,
  //      errors );
  //    TAC_HANDLE_ERROR( errors );
  //  }
  //  if( Contains( textureData.binding, Binding::RenderTarget ) &&
  //      Contains( textureData.binding, Binding::ShaderResource ) )
  //    mDeviceContext->GenerateMips( srv );
  //  TextureDX11* textureDX11;
  //  AddRendererResource( &textureDX11, textureData );
  //  textureDX11->mRTV = rTV;
  //  textureDX11->mSrv = srv;
  //  textureDX11->mDXObj = dXObj;
  //  *outputTexture = textureDX11;
  //}


  //void RendererDirectX11::AddTexture(
  //  const String& name,
  //  Shader* shaderID,
  //  ShaderType shaderType,
  //  int samplerIndex )
  //{
  //  AssertRenderThread();
  //  TAC_ASSERT( shaderID );
  //  auto* shader = ( ShaderDX11* )shaderID;
  //  if( IsDebugMode() )
  //  {
  //    for( auto sampler : shader->mTextures )
  //    {
  //      // make sure we don't overwrite a texture
  //      if( sampler->mShaderType == shaderType )
  //      {
  //        TAC_ASSERT( sampler->mSamplerIndex != samplerIndex );
  //      }
  //    }
  //  }
  //  auto mySampler = new Sampler();
  //  mySampler->mName = name;
  //  mySampler->mSamplerIndex = samplerIndex;
  //  mySampler->mShaderType = shaderType;
  //  shader->mTextures.push_back( mySampler );
  //}

  //void RendererDirectX11::SetTexture(
  //  const String& name,
  //  Tac::Texture* texture )
  //{
  //  AssertRenderThread();
    //Assert( mCurrentShader );
    //auto sampler = mCurrentShader->FindTexture( name );
    //Assert( sampler );
    //// todo: make this not shit
    //auto& textures = mCurrentTextures[ sampler->mShaderType ];
    //int requiredSize = sampler->mSamplerIndex + 1;
    //if( textures.size() < requiredSize )
    //  textures.resize( requiredSize );
    //textures[ sampler->mSamplerIndex ] = ( TextureDX11* )texture;
    //mCurrentTexturesDirty.insert( sampler->mShaderType );
  //}

  //void RendererDirectX11::CopyTextureRegion(
  //  Tac::Texture* dst,
  //  Image src,
  //  int x,
  //  int y,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  int z = 0;
  //  D3D11_BOX srcBox = {};
  //  srcBox.right = src.mWidth;
  //  srcBox.bottom = src.mHeight;
  //  srcBox.back = 1;
  //  Tac::Texture* srcTexture;
  //  TextureData textureData;
  //  textureData.access = Access::Default;
  //  textureData.binding = {};
  //  textureData.cpuAccess = {};
  //  textureData.mName = "temp copy texture";
  //  textureData.mFrame = TAC_STACK_FRAME;
  //  textureData.myImage = src;
  //  AddTextureResource( &srcTexture, textureData, errors );
  //  TAC_HANDLE_ERROR( errors );
  //  auto srcTextureDX11 = ( TextureDX11* )srcTexture;
  //  auto dstTextureDX11 = ( TextureDX11* )dst;
  //  ID3D11Resource* dstTex = dstTextureDX11->mDXObj;
  //  ID3D11Resource* srcTex = srcTextureDX11->mDXObj;
  //  mDeviceContext->CopySubresourceRegion(
  //    dstTex,
  //    0, // dst subresource
  //    x,
  //    y,
  //    z,
  //    srcTex,
  //    0, // src subresource,
  //    &srcBox );
  //  RemoveRendererResource( srcTexture );
  //}

  // non-virtual ---

  // why does this function exist?
  //void RendererDirectX11::CreateTexture(
  //  const Image& myImage,
  //  void* optionalInitialBytes,
  //  ID3D11Resource** resource,
  //  Access access,
  //  std::set< CPUAccess > cpuAccess,
  //  std::set< Binding > binding,
  //  const String& debugName,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  D3D11_TEXTURE2D_DESC texDesc = {};
  //  texDesc.Width = myImage.mWidth;
  //  texDesc.Height = myImage.mHeight;
  //  // all levels?
  //  //texDesc.MipLevels = 0;
  //  // also all levels?
  //  //texDesc.MipLevels = 5;
  //  texDesc.MipLevels = 1;
  //  //if(
  //  //  debugName != "temp copy texture" &&
  //  //  debugName != "texture atlas"
  //  //  && debugName != "1x1 white"
  //  //    )
  //  //{
  //  //  const char* grassPath = "assets\\grass.png";
  //  //  if(debugName == grassPath)
  //  //  {
  //  //    texDesc.MipLevels = 5;
  //  //  }
  //  //}
  //  UINT MiscFlags = 0;
  //  UINT BindFlags = GetBindFlags( binding );
  //  if( BindFlags & D3D11_BIND_RENDER_TARGET &&
  //      BindFlags & D3D11_BIND_SHADER_RESOURCE )
  //    MiscFlags &= D3D11_RESOURCE_MISC_GENERATE_MIPS;
  //  texDesc.SampleDesc.Count = 1;
  //  texDesc.ArraySize = 1;
  //  texDesc.Format = GetDXGIFormat( myImage.mFormat );
  //  texDesc.Usage = GetUsage( access );
  //  texDesc.BindFlags = BindFlags;
  //  texDesc.CPUAccessFlags = GetCPUAccessFlags( cpuAccess );
  //  texDesc.MiscFlags = MiscFlags;
  //  // D3D11_SUBRESOURCE_DATA struct Ure
  //  // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476220(v=vs.85).aspx
  //  // You set SysMemPitch to the distance between any two adjacent pixels on different lines.
  //  // You set SysMemSlicePitch to the size of the entire 2D surface in bytes.
  //  D3D11_SUBRESOURCE_DATA subResource = {};
  //  subResource.pSysMem = optionalInitialBytes;
  //  subResource.SysMemPitch = myImage.mPitch;
  //  subResource.SysMemSlicePitch = myImage.mPitch * myImage.mHeight; // <-- I guess
  //  D3D11_SUBRESOURCE_DATA* pSubResource = optionalInitialBytes ? &subResource : nullptr;
  //  TAC_DX11_CALL(
  //    errors,
  //    mDevice->CreateTexture2D,
  //    &texDesc,
  //    pSubResource,
  //    ( ID3D11Texture2D** )resource );

    //TAC_HANDLE_ERROR( errors );
  //}

  //void RendererDirectX11::CreateShaderResourceView(
  //  ID3D11Resource* resource,
  //  ID3D11ShaderResourceView** srv,
  //  const String& debugName,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  D3D11_RESOURCE_DIMENSION type;
  //  resource->GetType( &type );
  //  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  //  switch( type )
  //  {
  //    case D3D11_RESOURCE_DIMENSION_UNKNOWN:
  //    {
  //      TAC_ASSERT( false );
  //    } break;
  //    case D3D11_RESOURCE_DIMENSION_BUFFER:
  //    {
  //      TAC_ASSERT( false );
  //    } break;
  //    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
  //    {
  //      TAC_ASSERT( false );
  //    } break;
  //    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
  //    {
  //      D3D11_TEXTURE2D_DESC desc2d;
  //      ( ( ID3D11Texture2D* )resource )->GetDesc( &desc2d );
  //      srvDesc.Format = desc2d.Format;
  //      srvDesc.ViewDimension = desc2d.SampleDesc.Count > 1
  //        ? D3D11_SRV_DIMENSION_TEXTURE2DMS
  //        : D3D11_SRV_DIMENSION_TEXTURE2D;
  //      srvDesc.Texture2D.MipLevels = desc2d.MipLevels;
  //    } break;
  //    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
  //    {
  //      TAC_ASSERT( false );
  //    } break;
  //    default:
  //    {
  //      TAC_ASSERT( false );
  //    } break;
  //  }
  //  TAC_DX11_CALL( errors, mDevice->CreateShaderResourceView, resource, &srvDesc, srv );
// TAC_HANDLE_ERROR( errors );
  //  SetDebugName( *srv, debugName + " srv" );
  //}

  //void RendererDirectX11::AddDepthBuffer(
  //  DepthBuffer** outputDepthBuffer,
  //  const DepthBufferData& depthBufferData,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  DepthBufferDX11* depthBufferDX11;
  //  AddRendererResource( &depthBufferDX11, depthBufferData );
  //  depthBufferDX11->Init( errors );
  //  *outputDepthBuffer = depthBufferDX11;
  //}

  //void RendererDirectX11::AddConstantBuffer(
  //  CBuffer** outputCbuffer,
  //  const CBufferData& cBufferData,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  ID3D11Buffer* cbufferhandle;
  //  D3D11_BUFFER_DESC bd = {};
  //  bd.ByteWidth = RoundUpToNearestMultiple( cBufferData.byteCount, 16 );
  //  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  //  TAC_DX11_CALL( errors, mDevice->CreateBuffer, &bd, nullptr, &cbufferhandle );
// TAC_HANDLE_ERROR( errors );
  //  SetDebugName( cbufferhandle, cBufferData.mName );
  //  CBufferDX11* cBufferDX11;
  //  AddRendererResource( &cBufferDX11, cBufferData );
  //  cBufferDX11->mDXObj = cbufferhandle;
  //  *outputCbuffer = cBufferDX11;
  //}

  //void RendererDirectX11::AddBlendState(
  //  BlendState** outputBlendState,
  //  const BlendStateData& blendStateData,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  D3D11_RENDER_TARGET_BLEND_DESC d3d11rtbd = {};
  //  d3d11rtbd.BlendEnable = true;
  //  d3d11rtbd.SrcBlend = GetBlend( blendStateData.srcRGB );
  //  d3d11rtbd.DestBlend = GetBlend( blendStateData.dstRGB );
  //  d3d11rtbd.BlendOp = GetBlendOp( blendStateData.blendRGB );
  //  d3d11rtbd.SrcBlendAlpha = GetBlend( blendStateData.srcA );
  //  d3d11rtbd.DestBlendAlpha = GetBlend( blendStateData.dstA );
  //  d3d11rtbd.BlendOpAlpha = GetBlendOp( blendStateData.blendA );
  //  d3d11rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  //  D3D11_BLEND_DESC desc = {};
  //  desc.RenderTarget[ 0 ] = d3d11rtbd;
  //  ID3D11BlendState* mDXObj;
  //  TAC_DX11_CALL( errors, mDevice->CreateBlendState, &desc, &mDXObj );
// TAC_HANDLE_ERROR( errors );
  //  SetDebugName( mDXObj, blendStateData.mName );
  //  BlendStateDX11* blendState;
  //  AddRendererResource( &blendState, blendStateData );
  //  blendState->mDXObj = mDXObj;
  //  *outputBlendState = blendState;
  //}


  //void RendererDirectX11::AddRasterizerState(
  //  RasterizerState** outputRasterizerState,
  //  const RasterizerStateData& rasterizerStateData,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  D3D11_RASTERIZER_DESC desc = {};
  //  desc.FillMode = GetFillMode( rasterizerStateData.fillMode );
  //  desc.CullMode = GetCullMode( rasterizerStateData.cullMode );
  //  desc.ScissorEnable = rasterizerStateData.scissor;
  //  desc.MultisampleEnable = rasterizerStateData.multisample;
  //  desc.DepthClipEnable = true;
  //  desc.FrontCounterClockwise = rasterizerStateData.frontCounterClockwise;
  //  ID3D11RasterizerState* rasterizerState;
  //  TAC_DX11_CALL( errors, mDevice->CreateRasterizerState, &desc, &rasterizerState );
    // TAC_HANDLE_ERROR( errors );
  //  SetDebugName( rasterizerState, rasterizerStateData.mName );
  //  RasterizerStateDX11* rasterizerStateDX11;
  //  AddRendererResource( &rasterizerStateDX11, rasterizerStateData );
  //  rasterizerStateDX11->mDXObj = rasterizerState;
  //  *outputRasterizerState = rasterizerStateDX11;
  //}

  //void RendererDirectX11::AddDepthState(
  //  DepthState** outputDepthState,
  //  const DepthStateData& depthStateData,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  D3D11_DEPTH_STENCIL_DESC desc = {};
  //  desc.DepthFunc = GetDepthFunc( depthStateData.depthFunc );
  //  desc.DepthEnable = depthStateData.depthTest;
  //  desc.DepthWriteMask
  //    = depthStateData.depthWrite
  //    ? D3D11_DEPTH_WRITE_MASK_ALL
  //    : D3D11_DEPTH_WRITE_MASK_ZERO;
  //  ID3D11DepthStencilState* depthStencilState;
  //  TAC_DX11_CALL( errors, mDevice->CreateDepthStencilState, &desc, &depthStencilState );
    //TAC_HANDLE_ERROR( errors );
  //  SetDebugName( depthStencilState, depthStateData.mName );
  //  DepthStateDX11* depthState;
  //  AddRendererResource( &depthState, depthStateData );
  //  depthState->mDXObj = depthStencilState;
  //  *outputDepthState = depthState;
  //}

  //void RendererDirectX11::AddVertexFormat(
  //  VertexFormat** outputVertexFormat,
  //  const VertexFormatData& vertexFormatDataa,
  //  Errors& errors )
  //{
  //  AssertRenderThread();
  //  Vector< D3D11_INPUT_ELEMENT_DESC > inputElementDescs;
  //  for( VertexDeclaration curFormat : vertexFormatDataa.vertexFormatDatas )
  //  {
  //    D3D11_INPUT_ELEMENT_DESC curDX11Input = {};
  //    curDX11Input.Format = GetDXGIFormat( curFormat.mTextureFormat );
  //    curDX11Input.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
  //    curDX11Input.InstanceDataStepRate = 0;
  //    curDX11Input.InputSlot = 0;
  //    curDX11Input.SemanticName = GetSemanticName( curFormat.mAttribute );
  //    // MSDN:
  //    // A semantic index modifies a semantic, with an integer index number.
  //    // A semantic index is only needed in a case where there is more than
  //    // one element with the same semantic.
  //    curDX11Input.SemanticIndex;
  //    curDX11Input.AlignedByteOffset = curFormat.mAlignedByteOffset;
  //    inputElementDescs.push_back( curDX11Input );
  //  }
  //  auto myShaderDX11 = ( ShaderDX11* )vertexFormatDataa.shader;
  //  ID3D11InputLayout* inputLayout;
  //  TAC_DX11_CALL(
  //    errors,
  //    mDevice->CreateInputLayout,
  //    inputElementDescs.data(),
  //    ( UINT )inputElementDescs.size(),
  //    myShaderDX11->mLoadData.mInputSig->GetBufferPointer(),
  //    myShaderDX11->mLoadData.mInputSig->GetBufferSize(),
  //    &inputLayout );
    //TAC_HANDLE_ERROR( errors );
  //  SetDebugName( inputLayout, vertexFormatDataa.mName );
  //  VertexFormatDX11* vertexFormatDX11;
  //  AddRendererResource( &vertexFormatDX11, vertexFormatDataa );
  //  vertexFormatDX11->mDXObj = inputLayout;
  //  *outputVertexFormat = vertexFormatDX11;
  //}


  //void RendererDirectX11::DebugBegin( const String& section )
  //{
  //  AssertRenderThread();
  //  if( !IsDebugMode() )
  //    return;
  //  std::wstring s( section.data(), section.c_str() + section.size() );
  //  mUserAnnotationDEBUG->BeginEvent( s.c_str() );
  //}

  //void RendererDirectX11::DebugMark( const String& remark )
  //{
  //  AssertRenderThread();
  //  if( !IsDebugMode() )
  //    return;
  //  std::wstring s( remark.c_str(), remark.c_str() + remark.size() );
  //  mUserAnnotationDEBUG->SetMarker( s.c_str() );
  //}

  //void RendererDirectX11::DebugEnd()
  //{
  //  AssertRenderThread();
  //  if( !IsDebugMode() )
  //    return;
  //  mUserAnnotationDEBUG->EndEvent();
  //}

  //// TODO: remove this function?
  //void RendererDirectX11::DrawNonIndexed( int vertCount )
  //{
  //  AssertRenderThread();
  //  TAC_INVALID_CODE_PATH;
  //  mDeviceContext->Draw( vertCount, 0 );
  //}

  //// TODO: remove this function?
  //void RendererDirectX11::DrawIndexed(
  //  int elementCount,
  //  int idxOffset,
  //  int vtxOffset )
  //{
  //  AssertRenderThread();
  //  TAC_INVALID_CODE_PATH;
  //  mDeviceContext->DrawIndexed( elementCount, idxOffset, vtxOffset );
  //}


  //void RendererDirectX11::Apply()
  //{
  //  AssertRenderThread();
  //  //Assert( mCurrentShader );

  //  if( !mCurrentSamplersDirty.empty() )
  //  {
  //    for( auto shaderType : mCurrentSamplersDirty )
  //    {
  //      Vector< ID3D11SamplerState* >ppSamplers;
  //      for( auto sampler : mCurrentSamplers.at( shaderType ) )
  //        ppSamplers.push_back( sampler->mDXObj );

  //      switch( shaderType )
  //      {
  //        case ShaderType::Vertex:
  //          mDeviceContext->VSSetSamplers( 0, ( UINT )ppSamplers.size(), ppSamplers.data() );
  //          break;
  //        case ShaderType::Fragment:
  //          mDeviceContext->PSSetSamplers( 0, ( UINT )ppSamplers.size(), ppSamplers.data() );
  //          break;
  //          TAC_INVALID_DEFAULT_CASE( shaderType );
  //      }
  //    }
  //    mCurrentSamplersDirty.clear();
  //  }

  //  if( !mCurrentTexturesDirty.empty() )
  //  {
  //    for( auto shaderType : mCurrentTexturesDirty )
  //    {
  //      const auto& shaderTypeCurrentTextures = mCurrentTextures.at( shaderType );
  //      Vector< ID3D11ShaderResourceView*> ppSRVs;
  //      for( auto texture : shaderTypeCurrentTextures )
  //        ppSRVs.push_back( texture->mSrv );
  //      switch( shaderType )
  //      {
  //        case ShaderType::Vertex:
  //          mDeviceContext->VSSetShaderResources( 0, ( UINT )ppSRVs.size(), ppSRVs.data() );
  //          break;
  //        case ShaderType::Fragment:
  //          mDeviceContext->PSSetShaderResources( 0, ( UINT )ppSRVs.size(), ppSRVs.data() );
  //          break;
  //          TAC_INVALID_DEFAULT_CASE( shaderType );
  //      }
  //    }
  //    mCurrentTexturesDirty.clear();
  //  }
  //}

  //void RendererDirectX11::SetViewport(
  //  float xRelBotLeftCorner,
  //  float yRelBotLeftCorner,
  //  float wIncreasingRight,
  //  float hIncreasingUp )
  //{
  //  auto curRenderTarget = mCurRenderTargets[ 0 ];
  //  D3D11_VIEWPORT vp;
  //  vp.Width = wIncreasingRight;
  //  vp.Height = hIncreasingUp;
  //  vp.MinDepth = 0.0f;
  //  vp.MaxDepth = 1.0f;
  //  vp.TopLeftX = xRelBotLeftCorner;
  //  vp.TopLeftY = curRenderTarget->myImage.mHeight - ( yRelBotLeftCorner + hIncreasingUp );
  //  mDeviceContext->RSSetViewports( 1, &vp );
  //}

  //void RendererDirectX11::SetPrimitiveTopology( Primitive primitive )
  //{
  //  AssertRenderThread();
  //  const D3D_PRIMITIVE_TOPOLOGY dxPrimitiveTopology = [ primitive ]()
  //  {
  //    switch( primitive )
  //    {
  //      case Primitive::TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  //      case Primitive::LineList: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
  //        TAC_INVALID_DEFAULT_CASE( primitive );
  //    }
  //    return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
  //  }( );
  //  mDeviceContext->IASetPrimitiveTopology( dxPrimitiveTopology );
  //}

  //void RendererDirectX11::SetScissorRect(
  //  float x1,
  //  float y1,
  //  float x2,
  //  float y2 )
  //{
  //  const D3D11_RECT r = {
  //    ( LONG )x1,
  //    ( LONG )y1,
  //    ( LONG )x2,
  //    ( LONG )y2 };
  //
  //  mDeviceContext->RSSetScissorRects( 1, &r );
  //}

  // private functions and data


  // TODO: wtf, hr is reassigned before being used
  String RendererDirectX11::AppendInfoQueueMessage( HRESULT hr )
  {
    AssertRenderThread();
    if( !IsDebugMode() )
      return "";
    // GetMessage() is called...
    // - 1st to get the message length,
    // - 2nd to get the message itself,
    SIZE_T messageLen = 0;
    hr = mInfoQueueDEBUG->GetMessageA( 0, nullptr, &messageLen );
    if( FAILED( hr ) )
      return "Failed to get info queue message";
    Vector< char > data( ( int )messageLen );
    auto pMessage = ( D3D11_MESSAGE* )data.data();
    hr = mInfoQueueDEBUG->GetMessageA( 0, pMessage, &messageLen );
    if( FAILED( hr ) )
      return "Failed to get message";
    // NOTE( N8 ): length may include the null terminator
    return String( pMessage->pDescription, ( int )pMessage->DescriptionByteLength );
  }

  // Q: Should this function just return the clip space dimensions instead of A, B?
  void RendererDirectX11::GetPerspectiveProjectionAB(
    float f,
    float n,
    float& a,
    float& b )
  {
    TAC_ASSERT( f > n );

    float invDenom = 1.0f / ( n - f );

    // ( A, B ) maps ( -n, -f ) to ( 0, 1 )
    // because clip space in directx is [ -1, 1 ][ -1, 1 ][ 0, 1 ]
    // note that clip space in opengl is [ -1, 1 ][ -1, 1 ][ -1, 1 ]
    // todo: double check this function
    a = f * invDenom;
    b = f * invDenom * n;
  }

  void RendererDirectX11::SetDebugName(
    ID3D11DeviceChild* directXObject,
    const String& name )
  {
    AssertRenderThread();
    TAC_ASSERT( name.size() );
    if( !IsDebugMode() )
      return;
    TAC_ASSERT( directXObject );
    const int buffersize = 256;
    char data[ buffersize ] = {};

    UINT pDataSize = buffersize;
    directXObject->GetPrivateData(
      WKPDID_D3DDebugObjectName,
      &pDataSize,
      &data );

    String newname;
    if( pDataSize )
    {
      newname += String( data, pDataSize );
      newname += " --and-- ";

      auto hide = MakeArray< D3D11_MESSAGE_ID >( D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS );
      D3D11_INFO_QUEUE_FILTER filter = {};
      filter.DenyList.NumIDs = hide.size();
      filter.DenyList.pIDList = hide.data();
      mInfoQueueDEBUG->PushStorageFilter( &filter );
    }
    newname += name;

    directXObject->SetPrivateData(
      WKPDID_D3DDebugObjectName,
      ( UINT )newname.size(),
      newname.c_str() );
    if( pDataSize )
    {
      mInfoQueueDEBUG->PopStorageFilter();
    }
  }



  // Make this static?
  //Sampler* ShaderDX11::Find( Vector< Sampler* >& samplers, const String& name )
  //{
  //  AssertRenderThread();
  //  for( auto sampler : samplers )
  //  {
  //    if( sampler->mName == name )
  //      return sampler;
  //  }
  //  return nullptr;
  //}

  void ShaderDX11LoadData::Release()
  {
    AssertRenderThread();
    TAC_RELEASE_IUNKNOWN( mVertexShader );
    TAC_RELEASE_IUNKNOWN( mPixelShader );
  }
  //ShaderDX11::~ShaderDX11()
  //{
  //  AssertRenderThread();
  //  mLoadData.Release();
  //  if( RendererDirectX11::Instance->mCurrentlyBoundShader == this )
  //    RendererDirectX11::Instance->mCurrentlyBoundShader = nullptr;
  //}

  //Sampler* ShaderDX11::FindTexture( const String& name )
  //{
  //  AssertRenderThread();
  //  return Find( mTextures, name );
  //}

  //Sampler* ShaderDX11::FindSampler( const String& name )
  //{
  //  AssertRenderThread();
  //  return Find( mSamplers, name );
  //}

  //bool debugTextureLifespan = false;
  //TextureDX11::TextureDX11()
  //{
  //  AssertRenderThread();
  //  if( debugTextureLifespan )
  //  {
  //    std::cout
  //      << "creating texture " << this
  //      << std::endl;
  //  }
  //}

  //TextureDX11::~TextureDX11()
  //{
  //  AssertRenderThread();
  //  if( debugTextureLifespan )
  //  {
  //    std::cout
  //      << "deleting texture " << this << "(" << mName << ")"
  //      << std::endl;
  //  }

  //  Clear();
  //}

  //void TextureDX11::Clear()
  //{
  //  AssertRenderThread();
  //  TAC_RELEASE_IUNKNOWN( mDXObj );
  //  TAC_RELEASE_IUNKNOWN( mSrv );
  //  TAC_RELEASE_IUNKNOWN( mRTV );
  //  for( int i = 0; i < RendererDirectX11::Instance->mCurrentlyBoundTextures.size(); ++i )
  //    if( RendererDirectX11::Instance->mCurrentlyBoundTextures[ i ] == this )
  //      RendererDirectX11::Instance->mCurrentlyBoundTextures[ i ] = nullptr;
  //}
  //void* TextureDX11::GetImguiTextureID()
  //{
  //  return this;
  //}

  //static void Overwrite( ID3D11Resource* resource, void* data, int byteCount, Errors& errors )
  //{
  //  AssertRenderThread();
  //  ID3D11DeviceContext* deviceContext = RendererDirectX11::Instance->mDeviceContext;
  //  D3D11_MAP d3d11mapType = GetD3D11_MAP( Map::WriteDiscard );
  //  D3D11_MAPPED_SUBRESOURCE mappedResource;
  //  TAC_DX11_CALL( errors, deviceContext->Map, resource, 0, d3d11mapType, 0, &mappedResource );
   // TAC_HANDLE_ERROR( errors );
  //  MemCpy( mappedResource.pData, data, byteCount );
  //  RendererDirectX11::Instance->mDeviceContext->Unmap( resource, 0 );
  //}

  //VertexBufferDX11::~VertexBufferDX11()
  //{

  //  TAC_RELEASE_IUNKNOWN( mDXObj );
  //  if( RendererDirectX11::Instance->mCurrentlyBoundVertexBuffer == this )
  //    RendererDirectX11::Instance->mCurrentlyBoundVertexBuffer = nullptr;
  //}

  //void VertexBufferDX11::Overwrite( void* data, int byteCount, Errors& errors )
  //{
  //  AssertRenderThread();
  //  Tac::Overwrite( mDXObj, data, byteCount, errors );
  //}

  //IndexBufferDX11::~IndexBufferDX11()
  //{
  //  TAC_RELEASE_IUNKNOWN( mDXObj );
  //  if( RendererDirectX11::Instance->mCurrentlyBoundIndexBuffer == this )
  //    RendererDirectX11::Instance->mCurrentlyBoundIndexBuffer = nullptr;
  //}

  //void IndexBufferDX11::Overwrite( void* data, int byteCount, Errors& errors )
  //{
  //  AssertRenderThread();
  //  Tac::Overwrite( mDXObj, data, byteCount, errors );
  //}

  //CBufferDX11::~CBufferDX11()
  //{
  //  TAC_RELEASE_IUNKNOWN( mDXObj );
  //}

  //void CBufferDX11::SendUniforms( void* bytes )
  //{
  //  ID3D11Resource* resource = mDXObj;
  //  RendererDirectX11::Instance->mDeviceContext->UpdateSubresource(
  //    resource,
  //    0,
  //    nullptr,
  //    bytes,
  //    0,
  //    0 );
  //}

  int registerDX11 = []()
  {
    static struct DirectX11RendererFactory : public RendererFactory
    {
      DirectX11RendererFactory()
      {
        mRendererName = RendererNameDirectX11;
      }
      void CreateRenderer() override
      {
        new RendererDirectX11;
      }
    } factory;
    RendererRegistry::Instance().mFactories.push_back( &factory );
    return 0;
  }( );

  //DepthBufferDX11::~DepthBufferDX11()
  //{
  //  Clear();
  //}

  //void DepthBufferDX11::Init( Errors& errors )
  //{
  //  AssertRenderThread();
  //  D3D11_TEXTURE2D_DESC texture2dDesc = {};
  //  texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  //  texture2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  //  texture2dDesc.Height = height;
  //  texture2dDesc.Width = width;
  //  texture2dDesc.SampleDesc.Count = 1;
  //  texture2dDesc.SampleDesc.Quality = 0;
  //  texture2dDesc.ArraySize = 1;
  //  texture2dDesc.MipLevels = 1;

  //  ID3D11Device* mDevice = RendererDirectX11::Instance->mDevice;

  //  ID3D11Texture2D* texture;
  //  TAC_DX11_CALL( errors, mDevice->CreateTexture2D, &texture2dDesc, nullptr, &texture );
  //  TAC_HANDLE_ERROR( errors );
  //  RendererDirectX11::Instance->SetDebugName( texture, mName + " tex2d" );

  //  ID3D11DepthStencilView* dsv;
  //  D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
  //  desc.Format = texture2dDesc.Format;
  //  desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  //  TAC_DX11_CALL( errors, mDevice->CreateDepthStencilView, texture, &desc, &dsv );
  //  TAC_HANDLE_ERROR( errors );
  //  RendererDirectX11::Instance->SetDebugName( dsv, mName + " dsv" );

  //  mDXObj = texture;
  //  mDSV = dsv;
  //}
  //void DepthBufferDX11::Clear()
  //{
  //  TAC_RELEASE_IUNKNOWN( mDXObj );
  //  TAC_RELEASE_IUNKNOWN( mDSV );
  //  // null the view if bound?
  //}

  //SamplerStateDX11::~SamplerStateDX11()
  //{

  //  TAC_RELEASE_IUNKNOWN( mDXObj );
  //  if( RendererDirectX11::Instance->mCurrentlyBoundSamplerState == this )
  //    RendererDirectX11::Instance->mCurrentlyBoundSamplerState = nullptr;
  //}

  //DepthStateDX11::~DepthStateDX11()
  //{
  //  TAC_RELEASE_IUNKNOWN( mDXObj );
  //  if( RendererDirectX11::Instance->mCurrentlyBoundDepthState == this )
  //    RendererDirectX11::Instance->mCurrentlyBoundDepthState = nullptr;
  //}

  //BlendStateDX11::~BlendStateDX11()
  //{
  //  TAC_RELEASE_IUNKNOWN( mDXObj );
  //  if( RendererDirectX11::Instance->mCurrentlyBoundBlendState == this )
  //    RendererDirectX11::Instance->mCurrentlyBoundBlendState = nullptr;
  //}

  //RasterizerStateDX11::~RasterizerStateDX11()
  //{
  //  TAC_RELEASE_IUNKNOWN( mDXObj );
  //  if( RendererDirectX11::Instance->mCurrentlyBoundRasterizerState == this )
  //    RendererDirectX11::Instance->mCurrentlyBoundRasterizerState = nullptr;
  //}

  //VertexFormatDX11::~VertexFormatDX11()
  //{
  //  TAC_RELEASE_IUNKNOWN( mDXObj );

  //  if( RendererDirectX11::Instance->mCurrentlyBoundVertexFormat == this )
  //    RendererDirectX11::Instance->mCurrentlyBoundVertexFormat = nullptr;
  //}


  void RendererDirectX11::AddVertexBuffer( Render::VertexBufferHandle index,
                                           Render::CommandDataCreateVertexBuffer* data,
                                           Errors& errors )
  {
    TAC_ASSERT( data->mStride );
    //TAC_ASSERT( data->mFormat.CalculateTotalByteCount() );
    AssertRenderThread();
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = data->mByteCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = GetUsage( data->mAccess );
    bd.CPUAccessFlags = data->mAccess == Access::Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data->mOptionalInitialBytes;
    ID3D11Buffer* buffer;
    TAC_DX11_CALL( errors,
                   mDevice->CreateBuffer,
                   &bd,
                   data->mOptionalInitialBytes ? &initData : nullptr,
                   &buffer );
    mVertexBuffers[ index.mResourceId ].mBuffer = buffer;
    mVertexBuffers[ index.mResourceId ].mStride = data->mStride;
  }

  void RendererDirectX11::AddVertexFormat( Render::VertexFormatHandle vertexFormatHandle,
                                           Render::CommandDataCreateVertexFormat* commandData,
                                           Errors& errors )
  {
    AssertRenderThread();
    Vector< D3D11_INPUT_ELEMENT_DESC > inputElementDescs;
    commandData->mShaderHandle;
    for( int iVertexFormatData = 0; iVertexFormatData < commandData->mVertexFormatDataCount; ++iVertexFormatData )
    {
      VertexDeclaration curFormat = commandData->mVertexFormatDatas[ iVertexFormatData ];

      D3D11_INPUT_ELEMENT_DESC curDX11Input = {};
      curDX11Input.Format = GetDXGIFormat( curFormat.mTextureFormat );
      curDX11Input.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      curDX11Input.InstanceDataStepRate = 0;
      curDX11Input.InputSlot = 0;
      curDX11Input.SemanticName = GetSemanticName( curFormat.mAttribute );
      // MSDN:
      // A semantic index modifies a semantic, with an integer index number.
      // A semantic index is only needed in a case where there is more than
      // one element with the same semantic.
      curDX11Input.SemanticIndex;
      curDX11Input.AlignedByteOffset = curFormat.mAlignedByteOffset;
      inputElementDescs.push_back( curDX11Input );
    }
    ID3DBlob* inputSig = mPrograms[ commandData->mShaderHandle.mResourceId ].mInputSig;
    ID3D11InputLayout* inputLayout;
    TAC_DX11_CALL(
      errors,
      mDevice->CreateInputLayout,
      inputElementDescs.data(),
      ( UINT )inputElementDescs.size(),
      inputSig->GetBufferPointer(),
      inputSig->GetBufferSize(),
      &inputLayout );
    TAC_HANDLE_ERROR( errors );
    mInputLayouts[ vertexFormatHandle.mResourceId ] = inputLayout;
  }


  void RendererDirectX11::AddIndexBuffer( Render::IndexBufferHandle index,
                                          Render::CommandDataCreateIndexBuffer* data,
                                          Errors& errors )
  {
    AssertRenderThread();
    TAC_ASSERT( data->mFormat.mPerElementDataType == GraphicsType::uint );
    TAC_ASSERT( data->mFormat.mElementCount == 1 );
    TAC_ASSERT( data->mFormat.mPerElementByteCount == 2 ||
                data->mFormat.mPerElementByteCount == 4 )
      D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = data->mByteCount;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.Usage = GetUsage( data->mAccess );
    bd.CPUAccessFlags = data->mAccess == Access::Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data->mOptionalInitialBytes;
    ID3D11Buffer* buffer = mIndexBuffers[ index.mResourceId ].mBuffer;
    TAC_DX11_CALL( errors,
                   mDevice->CreateBuffer,
                   &bd,
                   data->mOptionalInitialBytes ? &initData : nullptr,
                   &buffer );
    mIndexBuffers[ index.mResourceId ].mFormat = data->mFormat;
    mIndexBuffers[ index.mResourceId ].mBuffer = buffer;
  }

  void RendererDirectX11::AddRasterizerState( Render::RasterizerStateHandle rasterizerStateHandle,
                                              Render::CommandDataCreateRasterizerState* commandData,
                                              Errors& errors )
  {
    AssertRenderThread();
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = GetFillMode( commandData->fillMode );
    desc.CullMode = GetCullMode( commandData->cullMode );
    desc.ScissorEnable = commandData->scissor;
    desc.MultisampleEnable = commandData->multisample;
    desc.DepthClipEnable = true;
    desc.FrontCounterClockwise = commandData->frontCounterClockwise;
    ID3D11RasterizerState* rasterizerState;
    TAC_DX11_CALL( errors, mDevice->CreateRasterizerState, &desc, &rasterizerState );
    mRasterizerStates[ rasterizerStateHandle.mResourceId ] = rasterizerState;
  }

  void RendererDirectX11::AddSamplerState( Render::SamplerStateHandle samplerStateHandle,
                                           Render::CommandDataCreateSamplerState* commandData,
                                           Errors& errors )
  {
    Render::CommandDataCreateSamplerState &samplerStateData = *commandData;
    AssertRenderThread();
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = GetFilter( samplerStateData.filter );
    desc.AddressU = GetAddressMode( samplerStateData.u );
    desc.AddressV = GetAddressMode( samplerStateData.v );
    desc.AddressW = GetAddressMode( samplerStateData.w );
    desc.ComparisonFunc = GetCompare( samplerStateData.compare );
    desc.BorderColor[ 0 ] = 1;
    desc.BorderColor[ 1 ] = 0;
    desc.BorderColor[ 2 ] = 0;
    desc.BorderColor[ 3 ] = 1;
    ID3D11SamplerState* samplerState;
    TAC_DX11_CALL( errors, mDevice->CreateSamplerState, &desc, &samplerState );
    mSamplerStates[ samplerStateHandle.mResourceId ] = samplerState;
  }

  void RendererDirectX11::AddShader( Render::ShaderHandle index,
                                     Render::CommandDataCreateShader* commandData,
                                     Errors& errors )
  {
    AssertRenderThread();

    //   StringView mShaderPath;
    //   StringView mShaderStr;
    //   ConstantBufferHandle mConstantBuffers[10];
    //   int mConstantBufferCount = 0;

    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3DBlob* inputSignature = nullptr;

    for( ;; )
    {
      if( errors )
      {
        if( IsDebugMode() )
        {
          DebugBreak();
          errors.clear();
        }
        else
        {
          TAC_HANDLE_ERROR( errors );
        }
      }

      String shaderPart1 = FileToString( GetDirectX11ShaderPath( "common" ), errors );
      String shaderPart2 = commandData->mShaderPath.size()
        ? FileToString( GetDirectX11ShaderPath( commandData->mShaderPath ), errors )
        : commandData->mShaderStr;
      String fullShaderString = shaderPart1 + "\n" + shaderPart2;

      // vertex shader
      ID3DBlob* pVSBlob;

      CompileShaderFromString( &pVSBlob,
                               fullShaderString,
                               "VS",
                               "vs_4_0",
                               errors );
      if( errors )
        continue;
      TAC_ON_DESTRUCT( pVSBlob->Release() );

      TAC_DX11_CALL( errors,
                     mDevice->CreateVertexShader,
                     pVSBlob->GetBufferPointer(),
                     pVSBlob->GetBufferSize(),
                     nullptr,
                     &vertexShader );
      if( errors )
        continue;

      TAC_DX11_CALL( errors,
                     D3DGetBlobPart,
                     pVSBlob->GetBufferPointer(),
                     pVSBlob->GetBufferSize(),
                     D3D_BLOB_INPUT_SIGNATURE_BLOB,
                     0,
                     &inputSignature );
      if( errors )
        continue;

      ID3DBlob* pPSBlob;
      CompileShaderFromString( &pPSBlob,
                               fullShaderString,
                               "PS",
                               "ps_4_0",
                               errors );
      if( errors )
        continue;
      TAC_ON_DESTRUCT( pPSBlob->Release() );

      TAC_DX11_CALL( errors,
                     mDevice->CreatePixelShader,
                     pPSBlob->GetBufferPointer(),
                     pPSBlob->GetBufferSize(),
                     nullptr,
                     &pixelShader );
      if( errors )
        continue;

      break;
    }

    mPrograms[ index.mResourceId ].mInputSig = inputSignature;
    mPrograms[ index.mResourceId ].mVertexShader = vertexShader;
    mPrograms[ index.mResourceId ].mPixelShader = pixelShader;
  }

  void RendererDirectX11::AddTexture( Render::TextureHandle index,
                                      Render::CommandDataCreateTexture* data,
                                      Errors& errors )
  {
    AssertRenderThread();

    UINT MiscFlags = GetMiscFlags( data->mBinding );
    UINT BindFlags = GetBindFlags( data->mBinding );
    DXGI_FORMAT Format = GetDXGIFormat( data->mImage.mFormat );
    UINT MipLevels = 1;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = data->mImage.mWidth;
    texDesc.Height = data->mImage.mHeight;
    texDesc.MipLevels = MipLevels;
    texDesc.SampleDesc.Count = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = Format;
    texDesc.Usage = GetUsage( data->mAccess );
    texDesc.BindFlags = BindFlags;
    texDesc.CPUAccessFlags = GetCPUAccessFlags( data->mCpuAccess );
    texDesc.MiscFlags = MiscFlags;

    // D3D11_SUBRESOURCE_DATA structure
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476220(v=vs.85).aspx
    // You set SysMemPitch to the distance between any two adjacent pixels on different lines.
    // You set SysMemSlicePitch to the size of the entire 2D surface in bytes.
    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = data->mImageBytes;
    subResource.SysMemPitch = data->mPitch;
    subResource.SysMemSlicePitch = data->mPitch * data->mImage.mHeight; // <-- I guess

    ID3D11Texture2D* texture2D;
    TAC_DX11_CALL(
      errors,
      mDevice->CreateTexture2D,
      &texDesc,
      data->mImageBytes ? &subResource : nullptr,
      &texture2D );

    ID3D11RenderTargetView* rTV = nullptr;
    if( BindFlags & D3D11_BIND_RENDER_TARGET )
    {
      TAC_DX11_CALL( errors, mDevice->CreateRenderTargetView,
                     texture2D,
                     nullptr,
                     &rTV );
    }

    ID3D11ShaderResourceView* srv = nullptr;
    if( BindFlags & D3D11_BIND_SHADER_RESOURCE )
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
      srvDesc.Format = Format;
      srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MipLevels = MipLevels;

      TAC_DX11_CALL( errors, mDevice->CreateShaderResourceView, texture2D, &srvDesc, &srv );
    }

    if( BindFlags & D3D11_BIND_RENDER_TARGET &&
        BindFlags & D3D11_BIND_SHADER_RESOURCE )
      mDeviceContext->GenerateMips( srv );

    Texture* texture = mTextures + index.mResourceId;
    texture->mTexture2D = texture2D;
    texture->mTextureSRV = srv;
    texture->mTextureRTV = rTV;
  }

  void RendererDirectX11::AddBlendState( Render::BlendStateHandle blendStateHandle,
                                         Render::CommandDataCreateBlendState* commandData,
                                         Errors& errors )
  {
    Render::CommandDataCreateBlendState& blendStateData = *commandData;
    AssertRenderThread();
    D3D11_BLEND_DESC desc = {};
    D3D11_RENDER_TARGET_BLEND_DESC* d3d11rtbd = &desc.RenderTarget[ 0 ];
    d3d11rtbd->BlendEnable = true;
    d3d11rtbd->SrcBlend = GetBlend( blendStateData.srcRGB );
    d3d11rtbd->DestBlend = GetBlend( blendStateData.dstRGB );
    d3d11rtbd->BlendOp = GetBlendOp( blendStateData.blendRGB );
    d3d11rtbd->SrcBlendAlpha = GetBlend( blendStateData.srcA );
    d3d11rtbd->DestBlendAlpha = GetBlend( blendStateData.dstA );
    d3d11rtbd->BlendOpAlpha = GetBlendOp( blendStateData.blendA );
    d3d11rtbd->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11BlendState* blendState;
    TAC_DX11_CALL( errors, mDevice->CreateBlendState, &desc, &blendState );
    TAC_HANDLE_ERROR( errors );
    mBlendStates[ blendStateHandle.mResourceId ] = blendState;
  }

  void RendererDirectX11::AddConstantBuffer( Render::ConstantBufferHandle constantBufferhandle,
                                             Render::CommandDataCreateConstantBuffer* commandData,
                                             Errors& errors )
  {
    AssertRenderThread();
    ID3D11Buffer* cbufferhandle;
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = RoundUpToNearestMultiple( commandData->mByteCount, 16 );
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // i guess?
    bd.Usage = D3D11_USAGE_DYNAMIC; // i guess?
    TAC_DX11_CALL( errors, mDevice->CreateBuffer, &bd, nullptr, &cbufferhandle );
    mConstantBuffers[ constantBufferhandle.mResourceId ].mBuffer = cbufferhandle;
    mConstantBuffers[ constantBufferhandle.mResourceId ].mShaderRegister = commandData->mShaderRegister;
  }

  void RendererDirectX11::AddDepthState( Render::DepthStateHandle depthStateHandle,
                                         Render::CommandDataCreateDepthState* commandData,
                                         Errors& errors )
  {
    AssertRenderThread();
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthFunc = GetDepthFunc( commandData->depthFunc );
    desc.DepthEnable = commandData->depthTest;
    desc.DepthWriteMask
      = commandData->depthWrite
      ? D3D11_DEPTH_WRITE_MASK_ALL
      : D3D11_DEPTH_WRITE_MASK_ZERO;
    ID3D11DepthStencilState* depthStencilState;
    TAC_DX11_CALL( errors, mDevice->CreateDepthStencilState, &desc, &depthStencilState );
    TAC_HANDLE_ERROR( errors );
    mDepthStencilStates[ depthStateHandle.mResourceId ] = depthStencilState;
  }

  void RendererDirectX11::AddFramebuffer( Render::FramebufferHandle index,
                                          Render::CommandDataCreateFramebuffer* data,
                                          Errors& errors )
  {
    AssertRenderThread();

    auto hwnd = ( HWND )data->mNativeWindowHandle;
    IUnknown* pDevice = mDevice;
    IDXGISwapChain* swapChain;
    int bufferCount = 4;
    const UINT width = data->mWidth;
    const UINT height = data->mHeight;
    mDxgi.CreateSwapChain( hwnd,
                           pDevice,
                           bufferCount,
                           width,
                           height,
                           &swapChain,
                           errors );
    TAC_HANDLE_ERROR( errors );

    ID3D11Device* device = RendererDirectX11::Instance->mDevice;
    //DXGI_SWAP_CHAIN_DESC swapChainDesc;
    //swapChain->GetDesc( &swapChainDesc );

    ID3D11Texture2D* pBackBuffer;
    TAC_DXGI_CALL( errors, swapChain->GetBuffer, 0, IID_PPV_ARGS( &pBackBuffer ) );
    ID3D11RenderTargetView* rtv = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr;
    TAC_DX11_CALL( errors, device->CreateRenderTargetView,
                   pBackBuffer,
                   rtvDesc,
                   &rtv );
    pBackBuffer->Release();

    AssertRenderThread();
    D3D11_TEXTURE2D_DESC texture2dDesc = {};
    texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    texture2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture2dDesc.Height = height;
    texture2dDesc.Width = width;
    texture2dDesc.SampleDesc.Count = 1;
    texture2dDesc.SampleDesc.Quality = 0;
    texture2dDesc.ArraySize = 1;
    texture2dDesc.MipLevels = 1;

    ID3D11Texture2D* texture;
    TAC_DX11_CALL( errors, mDevice->CreateTexture2D, &texture2dDesc, nullptr, &texture );

    ID3D11DepthStencilView* dsv;
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = texture2dDesc.Format;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    TAC_DX11_CALL( errors, mDevice->CreateDepthStencilView, texture, &depthStencilViewDesc, &dsv );

    Framebuffer* framebuffer = &mFramebuffers[ index.mResourceId ];
    framebuffer->mSwapChain = swapChain;
    framebuffer->mDepthStencilView = dsv;
    framebuffer->mDepthTexture = texture;
    framebuffer->mHwnd = hwnd;
  }

  void RendererDirectX11::RemoveVertexBuffer( Render::VertexBufferHandle index, Errors& errors )
  {
    TAC_RELEASE_IUNKNOWN( mVertexBuffers[ index.mResourceId ].mBuffer );
    mVertexBuffers[ index.mResourceId ] = {};
    TAC_UNUSED_PARAMETER( errors );
  }

  void RendererDirectX11::RemoveVertexFormat( Render::VertexFormatHandle, Errors& )
  {

    TAC_UNIMPLEMENTED;
  }

  void RendererDirectX11::RemoveIndexBuffer( Render::IndexBufferHandle index, Errors& errors )
  {
    TAC_RELEASE_IUNKNOWN( mIndexBuffers[ index.mResourceId ].mBuffer );
    mIndexBuffers[ index.mResourceId ] = {};
    TAC_UNUSED_PARAMETER( errors );
  }

  void RendererDirectX11::RemoveRasterizerState( Render::RasterizerStateHandle, Errors& )
  {

    TAC_UNIMPLEMENTED;
  }

  void RendererDirectX11::RemoveSamplerState( Render::SamplerStateHandle, Errors& )
  {

    TAC_UNIMPLEMENTED;
  }

  void RendererDirectX11::RemoveShader( Render::ShaderHandle, Errors& )
  {

    TAC_UNIMPLEMENTED;
  }

  void RendererDirectX11::RemoveTexture( Render::TextureHandle index, Errors& errors )
  {
    TAC_RELEASE_IUNKNOWN( mTextures[ index.mResourceId ].mTexture2D );
    TAC_RELEASE_IUNKNOWN( mTextures[ index.mResourceId ].mTextureRTV );
    TAC_RELEASE_IUNKNOWN( mTextures[ index.mResourceId ].mTextureSRV );
    mTextures[ index.mResourceId ] = {};
    TAC_UNUSED_PARAMETER( errors );
  }

  void RendererDirectX11::RemoveFramebuffer( Render::FramebufferHandle index, Errors& errors )
  {
    TAC_UNUSED_PARAMETER( index );
    TAC_UNUSED_PARAMETER( errors );
    TAC_UNIMPLEMENTED;
  }

  void RendererDirectX11::RemoveBlendState( Render::BlendStateHandle, Errors& )
  {

    TAC_UNIMPLEMENTED;
  }

  void RendererDirectX11::RemoveConstantBuffer( Render::ConstantBufferHandle, Errors& )
  {

    TAC_UNIMPLEMENTED;
  }

  void RendererDirectX11::RemoveDepthState( Render::DepthStateHandle, Errors& )
  {

    TAC_UNIMPLEMENTED;
  }

  void RendererDirectX11::UpdateTextureRegion( Render::TextureHandle index,
                                               Render::CommandDataUpdateTextureRegion* data,
                                               Errors& errors )
  {
    AssertRenderThread();
    TAC_UNUSED_PARAMETER( errors );
    TAC_ASSERT( Render::IsSubmitAllocated( data->mSrcBytes ) );

    const UINT dstX = data->mDstX;
    const UINT dstY = data->mDstY;
    const UINT dstZ = 0;

    D3D11_BOX srcBox = {};
    srcBox.right = data->mSrc.mWidth;
    srcBox.bottom = data->mSrc.mHeight;
    srcBox.back = 1;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = data->mSrc.mWidth;
    texDesc.Height = data->mSrc.mHeight;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = GetDXGIFormat( data->mSrc.mFormat );
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = 0;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = data->mSrcBytes;
    subResource.SysMemPitch = data->mPitch;
    subResource.SysMemSlicePitch = data->mPitch * data->mSrc.mHeight;

    ID3D11Resource* dstTex = mTextures[ index.mResourceId ].mTexture2D;
    ID3D11Texture2D* srcTex;
    TAC_DX11_CALL( errors,
                   mDevice->CreateTexture2D,
                   &texDesc,
                   &subResource,
                   &srcTex );

    mDeviceContext->CopySubresourceRegion(
      dstTex,
      0, // dst subresource
      dstX,
      dstY,
      dstZ,
      srcTex,
      0, // src subresource,
      &srcBox );
    TAC_RELEASE_IUNKNOWN( srcTex );
  }

  //void RendererDirectX11::UpdateBuffer( ID3D11Buffer* buffer, Render::CommandDataUpdateBuffer* data, Errors& errors )
  //{
  //  UpdateBuffer( buffer, data->mBytes, data->mByteCount, errors );
  //}

  void RendererDirectX11::UpdateBuffer( ID3D11Buffer* buffer, const void* bytes, int byteCount, Errors& errors )
  {
    AssertRenderThread();
    TAC_ASSERT( Render::IsSubmitAllocated( bytes ) );
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    TAC_DX11_CALL( errors, mDeviceContext->Map, buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
    MemCpy( mappedResource.pData, bytes, byteCount );
    RendererDirectX11::Instance->mDeviceContext->Unmap( buffer, 0 );
  }

  //void RendererDirectX11::UpdateVertexBuffer( Render::VertexBufferHandle index,
  //                                            Render::CommandDataUpdateBuffer* data,
  //                                            Errors& errors )
  //{
  //  UpdateBuffer( mVertexBuffers[ index.mResourceId ].mBuffer, data, errors );
  //}

  //void RendererDirectX11::UpdateIndexBuffer( Render::IndexBufferHandle index,
  //                                           Render::CommandDataUpdateBuffer* data,
  //                                           Errors& errors )
  //{
  //  UpdateBuffer( mIndexBuffers[ index.mResourceId ].mBuffer, data, errors );
  //}

  //void RendererDirectX11::UpdateConstantBuffer( Render::ConstantBufferHandle constantBufferHandle,
  //                                              Render::CommandDataUpdateBuffer* data,
  //                                              Errors& errors )
  //{
  //  UpdateBuffer( mConstantBuffers[ constantBufferHandle.mResourceId ].mBuffer, data, errors );
  //}

}
