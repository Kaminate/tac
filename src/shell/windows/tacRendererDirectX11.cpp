#include "src/common/containers/tacArray.h"
#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/math/tacMath.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacMemory.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacShell.h"
#include "src/common/tacOS.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/tacFrameMemory.h"
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
  static bool gVerbose;

  static int registerDX11 = []()
  {
    static RendererFactory factory;
    factory.mCreateRenderer = []() { TAC_NEW RendererDirectX11; };
    factory.mRendererName = RendererNameDirectX11;
    RendererFactoriesRegister( &factory );
    return 0;
  }( );

#define TAC_DX11_CALL( errors, call, ... )                                                       \
{                                                                                                \
  const HRESULT result = call( __VA_ARGS__ );                                                    \
  if( FAILED( result ) )                                                                         \
  {                                                                                              \
    const String errorMsg = DX11CallAux( TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )", result ); \
    TAC_RAISE_ERROR( errorMsg, errors );                                                         \
  }                                                                                              \
}

  static String GetDirectX11ShaderPath( StringView shaderName )
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

  static String DX11CallAux( const char* fnCallWithArgs, HRESULT res )
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
    return  ss.str().c_str();
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
      default: TAC_ASSERT_INVALID_CASE( addressMode ); return D3D11_TEXTURE_ADDRESS_WRAP;
    }
  }

  static D3D11_COMPARISON_FUNC GetCompare( Comparison compare )
  {
    switch( compare )
    {
      case Comparison::Always: return D3D11_COMPARISON_ALWAYS;
      case Comparison::Never: return D3D11_COMPARISON_NEVER;
      default: TAC_ASSERT_INVALID_CASE( compare ); return D3D11_COMPARISON_ALWAYS;
    }
  };

  static D3D11_FILTER GetFilter( Filter filter )
  {
    switch( filter )
    {
      case Filter::Linear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      case Filter::Point: return D3D11_FILTER_MIN_MAG_MIP_POINT;
      case Filter::Aniso: return D3D11_FILTER_ANISOTROPIC;
      default: TAC_ASSERT_INVALID_CASE( filter ); return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    }
  };

  static D3D11_COMPARISON_FUNC GetDepthFunc( DepthFunc depthFunc )
  {
    switch( depthFunc )
    {
      case DepthFunc::Less: return D3D11_COMPARISON_LESS;
      case DepthFunc::LessOrEqual: return D3D11_COMPARISON_LESS_EQUAL;
      default: TAC_ASSERT_INVALID_CASE( depthFunc ); return D3D11_COMPARISON_LESS;
    }
  }

  static D3D11_USAGE GetUsage( Access access )
  {
    switch( access )
    {
      case Access::Default: return D3D11_USAGE_DEFAULT;
      case Access::Dynamic: return D3D11_USAGE_DYNAMIC;
      case Access::Static: return D3D11_USAGE_IMMUTABLE;
      default: TAC_ASSERT_INVALID_CASE( access ); return D3D11_USAGE_DEFAULT;
    }
  }

  static UINT GetCPUAccessFlags( CPUAccess access )
  {
    UINT result = 0;
    if( ( int )access & ( int )CPUAccess::Read )
      result |= D3D11_CPU_ACCESS_READ;
    if( ( int )access & ( int )CPUAccess::Write )
      result |= D3D11_CPU_ACCESS_WRITE;
    return result;
  }

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
      default: TAC_ASSERT_INVALID_CASE( fillMode ); return ( D3D11_FILL_MODE )0;
    }
  }

  static D3D11_CULL_MODE GetCullMode( CullMode cullMode )
  {
    switch( cullMode )
    {
      case CullMode::None: return D3D11_CULL_NONE;
      case CullMode::Back: return D3D11_CULL_BACK;
      case CullMode::Front: return D3D11_CULL_FRONT;
      default: TAC_ASSERT_INVALID_CASE( cullMode ); return ( D3D11_CULL_MODE )0;
    }
  }

  static WCHAR* ToTransientWchar( StringView str )
  {
    WCHAR* result = ( WCHAR* )FrameMemory::Allocate( ( sizeof( WCHAR ) + 1 ) * str.size() );
    WCHAR* resultIter = result;
    for( char c : str )
      *resultIter++ = ( WCHAR )c;
    *resultIter++ = 0;
    return result;
  }

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
    mName = RendererNameDirectX11;
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

  void RendererDirectX11::RenderBegin( const Render::Frame*, Errors& )
  {
    if( gVerbose )
      std::cout << "Render2::Begin\n";

    for( auto& b : constantBuffers )
      b = {};
    constantBufferCount = 0;

    for( int iWindow = 0; iWindow < mWindowCount; ++iWindow )
    {
      const Render::FramebufferHandle framebufferHandle = mWindows[ iWindow ];
      TAC_ASSERT( framebufferHandle.IsValid() );

      Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];
      ID3D11RenderTargetView* renderTargetView = framebuffer->mRenderTargetView;
      ID3D11DepthStencilView* depthStencilView = framebuffer->mDepthStencilView;

      const UINT ClearFlags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
      const FLOAT ClearDepth = 1.0f;
      const UINT8 ClearStencil = 0;
      mDeviceContext->ClearDepthStencilView( depthStencilView, ClearFlags, ClearDepth, ClearStencil );

      const FLOAT ClearGrey = 0.5f;
      const FLOAT ClearColorRGBA[] = { ClearGrey, ClearGrey, ClearGrey,  1.0f };
      mDeviceContext->ClearRenderTargetView( renderTargetView, ClearColorRGBA );
    }
    blendState = nullptr;
    depthStencilState = nullptr;
    viewHandle = Render::ViewHandle();
    indexBuffer = nullptr;

  }
  void RendererDirectX11::RenderEnd( const Render::Frame*, Errors& )
  {
    if( gVerbose )
      std::cout << "Render2::End\n";
  }
  void RendererDirectX11::RenderDrawCall( const Render::Frame* frame,
                                          const Render::DrawCall3* drawCall,
                                          Errors& errors )
  {






    if( drawCall->mShaderHandle.IsValid() )
    {
      Program* program = &mPrograms[ ( int )drawCall->mShaderHandle ];
      TAC_ASSERT( program->mVertexShader );
      TAC_ASSERT( program->mPixelShader );
      mDeviceContext->VSSetShader( program->mVertexShader, NULL, 0 );
      mDeviceContext->PSSetShader( program->mPixelShader, NULL, 0 );
    }

    if( drawCall->mBlendStateHandle.IsValid() && blendState != mBlendStates[ ( int )drawCall->mBlendStateHandle ] )
    {
      blendState = mBlendStates[ ( int )drawCall->mBlendStateHandle ];
      TAC_ASSERT( blendState );
      const FLOAT blendFactorRGBA[] = { 1.0f, 1.0f, 1.0f, 1.0f };
      const UINT sampleMask = 0xffffffff;
      mDeviceContext->OMSetBlendState( blendState, blendFactorRGBA, sampleMask );
    }

    if( drawCall->mDepthStateHandle.IsValid() && depthStencilState != mDepthStencilStates[ ( int )drawCall->mDepthStateHandle ] )
    {
      depthStencilState = mDepthStencilStates[ ( int )drawCall->mDepthStateHandle ];
      TAC_ASSERT( depthStencilState );
      const UINT stencilRef = 0;
      mDeviceContext->OMSetDepthStencilState( depthStencilState, stencilRef );
    }

    if( drawCall->mIndexBufferHandle.IsValid() )
    {
      indexBuffer = &mIndexBuffers[ ( int )drawCall->mIndexBufferHandle ];
      if( !indexBuffer->mBuffer )
        OS::DebugBreak();
      TAC_ASSERT( indexBuffer->mBuffer );
      const DXGI_FORMAT dxgiFormat = GetDXGIFormat( indexBuffer->mFormat );
      const UINT byteOffset = 0; //  drawCall->mStartIndex * indexBuffer->mFormat.mPerElementByteCount;
      mDeviceContext->IASetIndexBuffer( indexBuffer->mBuffer,
                                        dxgiFormat,
                                        byteOffset );
    }

    if( drawCall->mVertexBufferHandle.IsValid() )
    {
      const VertexBuffer* vertexBuffer = &mVertexBuffers[ ( int )drawCall->mVertexBufferHandle ];
      TAC_ASSERT( vertexBuffer->mBuffer );
      const UINT startSlot = 0;
      const UINT NumBuffers = 1;
      const UINT Strides[ NumBuffers ] = { ( UINT )vertexBuffer->mStride };
      const UINT ByteOffsets[ NumBuffers ] =
      {
        0
        // ( UINT )( drawCall->mStartVertex * vertexBuffer->mStride )
      };
      ID3D11Buffer* buffers[ NumBuffers ] = { vertexBuffer->mBuffer };
      mDeviceContext->IASetVertexBuffers( startSlot,
                                          NumBuffers,
                                          buffers,
                                          Strides,
                                          ByteOffsets );
    }

    if( drawCall->mRasterizerStateHandle.IsValid() )
    {
      ID3D11RasterizerState* rasterizerState = mRasterizerStates[ ( int )drawCall->mRasterizerStateHandle ];
      TAC_ASSERT( rasterizerState );
      mDeviceContext->RSSetState( rasterizerState );
    }

    if( drawCall->mSamplerStateHandle.IsValid() )
    {
      const UINT StartSlot = 0;
      const UINT NumSamplers = 1;
      ID3D11SamplerState* samplerState = mSamplerStates[ ( int )drawCall->mSamplerStateHandle ];
      TAC_ASSERT( samplerState );
      ID3D11SamplerState* Samplers[] = { samplerState };
      mDeviceContext->VSSetSamplers( StartSlot, NumSamplers, Samplers );
      mDeviceContext->PSSetSamplers( StartSlot, NumSamplers, Samplers );
    }

    if( drawCall->mVertexFormatHandle.IsValid() )
    {
      ID3D11InputLayout* inputLayout = mInputLayouts[ ( int )drawCall->mVertexFormatHandle ];
      TAC_ASSERT( inputLayout );
      mDeviceContext->IASetInputLayout( inputLayout );
    }

    if( drawCall->mViewHandle.IsValid() &&
        drawCall->mViewHandle != viewHandle )
    {
      viewHandle = drawCall->mViewHandle;
      const Render::View* view = &frame->mViews[ ( int )viewHandle ];
      const Render::FramebufferHandle framebufferHandle = view->mFrameBufferHandle;

      // Did you forget to call Render::SetViewFramebuffer?
      TAC_ASSERT( framebufferHandle.IsValid() );

      Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];
      ID3D11RenderTargetView* renderTargetView = framebuffer->mRenderTargetView;
      ID3D11DepthStencilView* depthStencilView = framebuffer->mDepthStencilView;
      TAC_ASSERT( renderTargetView );
      TAC_ASSERT( depthStencilView );
      UINT NumViews = 1;
      ID3D11RenderTargetView* RenderTargetViews[] = { renderTargetView };
      mDeviceContext->OMSetRenderTargets( NumViews, RenderTargetViews, depthStencilView );

      TAC_ASSERT( view->mViewportSet );
      D3D11_VIEWPORT viewport;
      viewport.Height = view->mViewport.mHeight;
      viewport.Width = view->mViewport.mWidth;
      viewport.TopLeftX = view->mViewport.mBottomLeftX;
      viewport.TopLeftY = -view->mViewport.mBottomLeftY; // convert opengl to directx
      viewport.MinDepth = view->mViewport.mMinDepth;
      viewport.MaxDepth = view->mViewport.mMaxDepth;
      mDeviceContext->RSSetViewports( 1, &viewport );

      // used if the rasterizer state ScissorEnable is TRUE
      TAC_ASSERT( view->mScissorSet );
      D3D11_RECT scissor;
      scissor.bottom = ( LONG )view->mScissorRect.mYMaxRelUpperLeftCornerPixel;
      scissor.left = ( LONG )view->mScissorRect.mXMinRelUpperLeftCornerPixel;
      scissor.right = ( LONG )view->mScissorRect.mXMaxRelUpperLeftCornerPixel;
      scissor.top = ( LONG )view->mScissorRect.mYMinRelUpperLeftCornerPixel;
      mDeviceContext->RSSetScissorRects( 1, &scissor );
    }

    if( drawCall->mTextureHandle.mTextureCount )
    {
      const UINT StartSlot = 0;
      const UINT NumViews = drawCall->mTextureHandle.mTextureCount;
      ID3D11ShaderResourceView* ShaderResourceViews[ D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT ] = {};
      for( int iSlot = 0; iSlot < drawCall->mTextureHandle.mTextureCount; ++iSlot )
      {
        const Render::TextureHandle textureHandle = drawCall->mTextureHandle[ iSlot ];
        if( !textureHandle.IsValid() )
          continue;
        const Texture* texture = &mTextures[ ( int )textureHandle ];
        TAC_ASSERT( texture->mTextureSRV ); // Did you set the Tac::Render::TexSpec::mBinding?
        ShaderResourceViews[ iSlot ] = texture->mTextureSRV;
      }

      mDeviceContext->VSSetShaderResources( StartSlot, NumViews, ShaderResourceViews );
      mDeviceContext->PSSetShaderResources( StartSlot, NumViews, ShaderResourceViews );
    }

    for( const Render::UpdateConstantBufferData& stuff : drawCall->mUpdateConstantBuffers )
    {
      const ConstantBuffer* constantBuffer = &mConstantBuffers[ ( int )stuff.mConstantBufferHandle ];
      TAC_ASSERT( constantBuffer->mBuffer );
      UpdateBuffer( constantBuffer->mBuffer,
                    stuff.mBytes,
                    stuff.mByteCount,
                    errors );

      constantBuffers[ constantBuffer->mShaderRegister ] = constantBuffer->mBuffer;
      constantBufferCount = Max( constantBufferCount, constantBuffer->mShaderRegister + 1 );

      const UINT StartSlot = 0;
      mDeviceContext->PSSetConstantBuffers( StartSlot, constantBufferCount, constantBuffers );
      mDeviceContext->VSSetConstantBuffers( StartSlot, constantBufferCount, constantBuffers );
    }

    mDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // not quite convinced this assert should be here.
    // on one hand, it prevents u from forgetting to set index count. ( good )
    // on the other, it prevents u from setting start index/index count
    //   seperately from setting the index buffer ( bad? )
    TAC_ASSERT( !drawCall->mIndexBufferHandle.IsValid() || drawCall->mIndexCount );

    if( drawCall->mIndexCount )
    {
      const UINT IndexCount = drawCall->mIndexCount;
      const UINT StartIndexLocation = drawCall->mStartIndex; //  *indexBuffer->mFormat.CalculateTotalByteCount();
      // should this be multiplied by the sizeof vertex?
      const INT BaseVertexLocation = 0; // drawCall->mStartVertex;
      mDeviceContext->DrawIndexed( IndexCount, StartIndexLocation, BaseVertexLocation );
    }
    else if( drawCall->mVertexCount )
    {
      const UINT VertexCount = drawCall->mVertexCount;
      const UINT StartVertexLocation = drawCall->mStartVertex;
      mDeviceContext->Draw( VertexCount, StartVertexLocation );
    }
  }

  void RendererDirectX11::SwapBuffers()
  {
    if( gVerbose )
      std::cout << "SwapBuffers::Begin\n";
    for( int iWindow = 0; iWindow < mWindowCount; ++iWindow )
      //for( int iFramebuffer = 0; iFramebuffer < Render::kMaxFramebuffers; ++iFramebuffer )
    {
      Render::FramebufferHandle framebufferHandle = mWindows[ iWindow ];
      Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];
      if( !framebuffer->mSwapChain )
        continue;
      const UINT SyncInterval = 0;
      const UINT Flags = 0;
      framebuffer->mSwapChain->Present( SyncInterval, Flags );
    }
    if( gVerbose )
      std::cout << "SwapBuffers::End\n";
  }

  static void CompileShaderFromString( ID3DBlob** ppBlobOut,
                                       const StringView shaderStr,
                                       const char* entryPoint,
                                       const char* shaderModel,
                                       Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    if( IsDebugMode() )
    {
      dwShaderFlags |= D3DCOMPILE_DEBUG;
      dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    }

    ID3DBlob* pErrorBlob;
    const HRESULT hr = D3DCompile( shaderStr.data(),
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
      const char* errMsg = ( const char* )pErrorBlob->GetBufferPointer();
      TAC_RAISE_ERROR( errMsg, errors );
    }
  }

  void RendererDirectX11::LoadShaderInternal( ShaderDX11LoadData* loadData,
                                              String name,
                                              String str,
                                              Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
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

      TAC_DX11_CALL( errors,
                     mDevice->CreateVertexShader,
                     pVSBlob->GetBufferPointer(),
                     pVSBlob->GetBufferSize(),
                     nullptr,
                     &loadData->mVertexShader );
      SetDebugName( loadData->mVertexShader, name + " vtx shader" );

      TAC_DX11_CALL( errors,
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

      TAC_DX11_CALL( errors,
                     mDevice->CreatePixelShader,
                     pPSBlob->GetBufferPointer(),
                     pPSBlob->GetBufferSize(),
                     nullptr,
                     &loadData->mPixelShader );
      SetDebugName( loadData->mPixelShader, name + " px shader" );
    }
  }

  // Q: Should this function just return the clip space dimensions instead of A, B?
  void RendererDirectX11::GetPerspectiveProjectionAB( float f,
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



  void RendererDirectX11::SetDebugName( ID3D11DeviceChild* directXObject,
                                        StringView name )
  {
    TAC_ASSERT( IsMainThread() );
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



  void RendererDirectX11::AddVertexBuffer( Render::CommandDataCreateVertexBuffer* data,
                                           Errors& errors )
  {
    TAC_ASSERT( data->mStride );
    TAC_ASSERT( IsMainThread() );
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = data->mByteCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = GetUsage( data->mAccess );
    bd.CPUAccessFlags = data->mAccess == Access::Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    TAC_ASSERT( !data->mOptionalInitialBytes || Render::IsSubmitAllocated( data->mOptionalInitialBytes ) );
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data->mOptionalInitialBytes;
    D3D11_SUBRESOURCE_DATA *pInitData = data->mOptionalInitialBytes ? &initData : nullptr;
    ID3D11Buffer* buffer;
    TAC_DX11_CALL( errors,
                   mDevice->CreateBuffer,
                   &bd,
                   pInitData,
                   &buffer );
    VertexBuffer* vertexBuffer = &mVertexBuffers[ ( int )data->mVertexBufferHandle ];
    vertexBuffer->mBuffer = buffer;
    vertexBuffer->mStride = data->mStride;
  }

  void RendererDirectX11::AddVertexFormat( Render::CommandDataCreateVertexFormat* commandData,
                                           Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    Render::VertexFormatHandle vertexFormatHandle = commandData->mVertexFormatHandle;
    Vector< D3D11_INPUT_ELEMENT_DESC > inputElementDescs;
    commandData->mShaderHandle;
    for( int iVertexFormatData = 0;
         iVertexFormatData < commandData->mVertexDeclarations.mVertexFormatDataCount;
         ++iVertexFormatData )
    {
      VertexDeclaration curFormat = commandData->mVertexDeclarations.mVertexFormatDatas[ iVertexFormatData ];

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
    ID3DBlob* inputSig = mPrograms[ ( int )commandData->mShaderHandle ].mInputSig;
    ID3D11InputLayout* inputLayout;
    TAC_DX11_CALL( errors,
                   mDevice->CreateInputLayout,
                   inputElementDescs.data(),
                   ( UINT )inputElementDescs.size(),
                   inputSig->GetBufferPointer(),
                   inputSig->GetBufferSize(),
                   &inputLayout );
    TAC_HANDLE_ERROR( errors );
    mInputLayouts[ ( int )vertexFormatHandle ] = inputLayout;
  }



  void RendererDirectX11::AddIndexBuffer( Render::CommandDataCreateIndexBuffer* data,
                                          Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    Render::IndexBufferHandle index = data->mIndexBufferHandle;
    TAC_ASSERT( data->mFormat.mPerElementDataType == GraphicsType::uint );
    TAC_ASSERT( data->mFormat.mElementCount == 1 );
    TAC_ASSERT( data->mFormat.mPerElementByteCount == 2 ||
                data->mFormat.mPerElementByteCount == 4 );
    TAC_ASSERT( !data->mOptionalInitialBytes || Render::IsSubmitAllocated( data->mOptionalInitialBytes ) );
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = data->mByteCount;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
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
    mIndexBuffers[ ( int )index ].mFormat = data->mFormat;
    mIndexBuffers[ ( int )index ].mBuffer = buffer;
  }

  void RendererDirectX11::AddRasterizerState( Render::CommandDataCreateRasterizerState* commandData,
                                              Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = GetFillMode( commandData->mRasterizerState.mFillMode );
    desc.CullMode = GetCullMode( commandData->mRasterizerState.mCullMode );
    desc.ScissorEnable = commandData->mRasterizerState.mScissor;
    desc.MultisampleEnable = commandData->mRasterizerState.mMultisample;
    desc.DepthClipEnable = true;
    desc.FrontCounterClockwise = commandData->mRasterizerState.mFrontCounterClockwise;
    ID3D11RasterizerState* rasterizerState;
    TAC_DX11_CALL( errors, mDevice->CreateRasterizerState, &desc, &rasterizerState );
    mRasterizerStates[ ( int )commandData->mRasterizerStateHandle ] = rasterizerState;
  }

  void RendererDirectX11::AddSamplerState( Render::CommandDataCreateSamplerState* commandData,
                                           Errors& errors )
  {
    Render::CommandDataCreateSamplerState &samplerStateData = *commandData;
    TAC_ASSERT( IsMainThread() );
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = GetFilter( commandData->mSamplerState.mFilter );
    desc.AddressU = GetAddressMode( commandData->mSamplerState.mU );
    desc.AddressV = GetAddressMode( commandData->mSamplerState.mV );
    desc.AddressW = GetAddressMode( commandData->mSamplerState.mW );
    desc.ComparisonFunc = GetCompare( commandData->mSamplerState.mCompare );
    desc.BorderColor[ 0 ] = 1;
    desc.BorderColor[ 1 ] = 0;
    desc.BorderColor[ 2 ] = 0;
    desc.BorderColor[ 3 ] = 1;
    ID3D11SamplerState* samplerStateDX11;
    TAC_DX11_CALL( errors, mDevice->CreateSamplerState, &desc, &samplerStateDX11 );
    mSamplerStates[ ( int )commandData->mSamplerStateHandle ] = samplerStateDX11;
  }

  void RendererDirectX11::AddShader( Render::CommandDataCreateShader* commandData,
                                     Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );

    //   StringView mShaderPath;
    //   StringView mShaderStr;
    //   ConstantBufferHandle mConstantBuffers[10];
    //   int mConstantBufferCount = 0;

    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3DBlob* inputSignature = nullptr;
    Render::ShaderHandle index = commandData->mShaderHandle;

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

      String shaderStringFull;
      StringView shaderFileNoPaths[] = { "common", commandData->mShaderSource.mShaderPath };
      for( StringView shaderFileNoPath : shaderFileNoPaths )
      {
        if( shaderFileNoPath.empty() )
          continue;
        String shaderFilePath = GetDirectX11ShaderPath( shaderFileNoPath );
        String shaderFileContents = FileToString( shaderFilePath, errors );
        shaderStringFull += shaderFileContents;
        shaderStringFull += "\n";
      }

      shaderStringFull += commandData->mShaderSource.mShaderStr;

      // vertex shader
      ID3DBlob* pVSBlob;

      CompileShaderFromString( &pVSBlob,
                               shaderStringFull,
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
                               shaderStringFull,
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

    mPrograms[ ( int )index ].mInputSig = inputSignature;
    mPrograms[ ( int )index ].mVertexShader = vertexShader;
    mPrograms[ ( int )index ].mPixelShader = pixelShader;
  }

  void RendererDirectX11::AddTexture( Render::CommandDataCreateTexture* data,
                                      Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    if( data->mTexSpec.mImageBytes && !data->mTexSpec.mPitch )
    {
      data->mTexSpec.mPitch =
        data->mTexSpec.mImage.mWidth *
        data->mTexSpec.mImage.mFormat.CalculateTotalByteCount();
    }

    // D3D11_SUBRESOURCE_DATA structure
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476220(v=vs.85).aspx
    // You set SysMemPitch to the distance between any two adjacent pixels on different lines.
    // You set SysMemSlicePitch to the size of the entire 2D surface in bytes.
    D3D11_SUBRESOURCE_DATA subResources[ 6 ] = {};
    int iSubresource = 0;
    if( data->mTexSpec.mImageBytes )
    {
      D3D11_SUBRESOURCE_DATA* subResource = &subResources[ iSubresource++ ];
      subResource->pSysMem = data->mTexSpec.mImageBytes;
      subResource->SysMemPitch = data->mTexSpec.mPitch;
      subResource->SysMemSlicePitch = data->mTexSpec.mPitch * data->mTexSpec.mImage.mHeight;
    }
    for( const void* imageBytesCubemap : data->mTexSpec.mImageBytesCubemap )
    {
      if( !imageBytesCubemap )
        continue;
      D3D11_SUBRESOURCE_DATA* subResource = &subResources[ iSubresource++ ];
      subResource->pSysMem = imageBytesCubemap;
      subResource->SysMemPitch = data->mTexSpec.mPitch;
      subResource->SysMemSlicePitch = data->mTexSpec.mPitch * data->mTexSpec.mImage.mHeight;
    }
    const bool isCubemap = iSubresource == 6;
    D3D11_SUBRESOURCE_DATA *pInitialData = iSubresource ? subResources : nullptr;

    const UINT MiscFlags = GetMiscFlags( data->mTexSpec.mBinding ) |
      ( isCubemap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0 );
    const UINT BindFlags = GetBindFlags( data->mTexSpec.mBinding );
    const DXGI_FORMAT Format = GetDXGIFormat( data->mTexSpec.mImage.mFormat );
    const UINT MipLevels = 1;
    const UINT ArraySize = iSubresource;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = data->mTexSpec.mImage.mWidth;
    texDesc.Height = data->mTexSpec.mImage.mHeight;
    texDesc.MipLevels = MipLevels;
    texDesc.SampleDesc.Count = 1;
    texDesc.ArraySize = ArraySize;
    texDesc.Format = Format;
    texDesc.Usage = GetUsage( data->mTexSpec.mAccess );
    texDesc.BindFlags = BindFlags;
    texDesc.CPUAccessFlags = GetCPUAccessFlags( data->mTexSpec.mCpuAccess );
    texDesc.MiscFlags = MiscFlags;


    ID3D11Texture2D* texture2D;
    TAC_DX11_CALL( errors,
                   mDevice->CreateTexture2D,
                   &texDesc,
                   pInitialData,
                   &texture2D );
    TAC_HANDLE_ERROR( errors );

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
      srvDesc.ViewDimension = isCubemap ? D3D11_SRV_DIMENSION_TEXTURECUBE : D3D11_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MipLevels = MipLevels;

      TAC_DX11_CALL( errors, mDevice->CreateShaderResourceView, texture2D, &srvDesc, &srv );
      TAC_HANDLE_ERROR( errors );
    }

    if( BindFlags & D3D11_BIND_RENDER_TARGET &&
        BindFlags & D3D11_BIND_SHADER_RESOURCE )
      mDeviceContext->GenerateMips( srv );

    Texture* texture = &mTextures[ ( int )data->mTextureHandle ];
    texture->mTexture2D = texture2D;
    texture->mTextureSRV = srv;
    texture->mTextureRTV = rTV;
  }

  void RendererDirectX11::AddBlendState( Render::CommandDataCreateBlendState* commandData,
                                         Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    Render::BlendStateHandle blendStateHandle = commandData->mBlendStateHandle;
    Render::BlendState* blendState = &commandData->mBlendState;
    D3D11_BLEND_DESC desc = {};
    D3D11_RENDER_TARGET_BLEND_DESC* d3d11rtbd = &desc.RenderTarget[ 0 ];
    d3d11rtbd->BlendEnable = true;
    d3d11rtbd->SrcBlend = GetBlend( blendState->srcRGB );
    d3d11rtbd->DestBlend = GetBlend( blendState->dstRGB );
    d3d11rtbd->BlendOp = GetBlendOp( blendState->blendRGB );
    d3d11rtbd->SrcBlendAlpha = GetBlend( blendState->srcA );
    d3d11rtbd->DestBlendAlpha = GetBlend( blendState->dstA );
    d3d11rtbd->BlendOpAlpha = GetBlendOp( blendState->blendA );
    d3d11rtbd->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11BlendState* blendStateDX11;
    TAC_DX11_CALL( errors, mDevice->CreateBlendState, &desc, &blendStateDX11 );
    TAC_HANDLE_ERROR( errors );
    mBlendStates[ ( int )blendStateHandle ] = blendStateDX11;
  }

  void RendererDirectX11::AddConstantBuffer( Render::CommandDataCreateConstantBuffer* commandData,
                                             Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    Render::ConstantBufferHandle constantBufferhandle = commandData->mConstantBufferHandle;
    ID3D11Buffer* cbufferhandle;
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = RoundUpToNearestMultiple( commandData->mByteCount, 16 );
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // i guess?
    bd.Usage = D3D11_USAGE_DYNAMIC; // i guess?
    TAC_DX11_CALL( errors, mDevice->CreateBuffer, &bd, nullptr, &cbufferhandle );
    mConstantBuffers[ ( int )constantBufferhandle ].mBuffer = cbufferhandle;
    mConstantBuffers[ ( int )constantBufferhandle ].mShaderRegister = commandData->mShaderRegister;
  }

  void RendererDirectX11::AddDepthState( Render::CommandDataCreateDepthState* commandData,
                                         Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthFunc = GetDepthFunc( commandData->mDepthState.mDepthFunc );
    desc.DepthEnable = commandData->mDepthState.mDepthTest;
    desc.DepthWriteMask
      = commandData->mDepthState.mDepthWrite
      ? D3D11_DEPTH_WRITE_MASK_ALL
      : D3D11_DEPTH_WRITE_MASK_ZERO;
    ID3D11DepthStencilState* depthStencilState;
    TAC_DX11_CALL( errors, mDevice->CreateDepthStencilState, &desc, &depthStencilState );
    TAC_HANDLE_ERROR( errors );
    mDepthStencilStates[ ( int )commandData->mDepthStateHandle ] = depthStencilState;
  }

  void RendererDirectX11::AddFramebuffer( Render::CommandDataCreateFramebuffer* data,
                                          Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );

    auto hwnd = ( HWND )data->mNativeWindowHandle; // window->GetOperatingSystemHandle();
    IDXGISwapChain* swapChain;
    const int bufferCount = 4;
    const UINT width = data->mWidth;
    const UINT height = data->mHeight;
    mDxgi.CreateSwapChain( hwnd,
                           mDevice,
                           bufferCount,
                           width,
                           height,
                           &swapChain,
                           errors );
    TAC_HANDLE_ERROR( errors );

    ID3D11Device* device = RendererDirectX11::Instance->mDevice;
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChain->GetDesc( &swapChainDesc );

    ID3D11Texture2D* pBackBuffer;
    TAC_DXGI_CALL( errors, swapChain->GetBuffer, 0, IID_PPV_ARGS( &pBackBuffer ) );
    ID3D11RenderTargetView* rtv = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr;
    TAC_DX11_CALL( errors, device->CreateRenderTargetView,
                   pBackBuffer,
                   rtvDesc,
                   &rtv );
    pBackBuffer->Release();

    D3D11_RENDER_TARGET_VIEW_DESC createdDesc = {};
    rtv->GetDesc( &createdDesc );


    TAC_ASSERT( IsMainThread() );
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

    Framebuffer* framebuffer = &mFramebuffers[ ( int )data->mFramebufferHandle ];
    framebuffer->mSwapChain = swapChain;
    framebuffer->mDepthStencilView = dsv;
    framebuffer->mDepthTexture = texture;
    framebuffer->mHwnd = hwnd;
    framebuffer->mRenderTargetView = rtv;
    framebuffer->mBufferCount = bufferCount;

    mWindows[ mWindowCount++ ] = data->mFramebufferHandle;
  }

  void RendererDirectX11::RemoveVertexBuffer( Render::VertexBufferHandle vertexBufferHandle, Errors& errors )
  {
    VertexBuffer *vertexBuffer = &mVertexBuffers[ ( int )vertexBufferHandle ];
    TAC_RELEASE_IUNKNOWN( vertexBuffer->mBuffer );
    *vertexBuffer = VertexBuffer();
  }

  void RendererDirectX11::RemoveVertexFormat( Render::VertexFormatHandle vertexFormatHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mInputLayouts[ ( int )vertexFormatHandle ] );
  }

  void RendererDirectX11::RemoveIndexBuffer( Render::IndexBufferHandle indexBufferHandle, Errors& )
  {
    IndexBuffer *indexBuffer = &mIndexBuffers[ ( int )indexBufferHandle ];
    TAC_RELEASE_IUNKNOWN( indexBuffer->mBuffer );
    *indexBuffer = IndexBuffer();
  }

  void RendererDirectX11::RemoveRasterizerState( Render::RasterizerStateHandle rasterizerStateHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mRasterizerStates[ ( int )rasterizerStateHandle ] );
  }

  void RendererDirectX11::RemoveSamplerState( Render::SamplerStateHandle samplerStateHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mSamplerStates[ ( int )samplerStateHandle ] );
  }

  void RendererDirectX11::RemoveShader( const Render::ShaderHandle shaderHandle, Errors& )
  {
    Program* program = &mPrograms[ ( int )shaderHandle ];
    TAC_RELEASE_IUNKNOWN( program->mInputSig );
    TAC_RELEASE_IUNKNOWN( program->mVertexShader );
    TAC_RELEASE_IUNKNOWN( program->mPixelShader );
  }

  void RendererDirectX11::RemoveTexture( Render::TextureHandle textureHandle, Errors& )
  {
    Texture* texture = &mTextures[ ( int )textureHandle ];
    TAC_RELEASE_IUNKNOWN( texture->mTexture2D );
    TAC_RELEASE_IUNKNOWN( texture->mTextureRTV );
    TAC_RELEASE_IUNKNOWN( texture->mTextureSRV );
  }

  void RendererDirectX11::RemoveFramebuffer( Render::FramebufferHandle framebufferHandle, Errors& )
  {
    for( int i = 0; i < mWindowCount; ++i )
      if( mWindows[ i ] == framebufferHandle )
        mWindows[ i ] = mWindows[ --mWindowCount ];
    Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];
    TAC_RELEASE_IUNKNOWN( framebuffer->mDepthStencilView );
    TAC_RELEASE_IUNKNOWN( framebuffer->mDepthTexture );
    TAC_RELEASE_IUNKNOWN( framebuffer->mRenderTargetView );
    TAC_RELEASE_IUNKNOWN( framebuffer->mSwapChain );
    *framebuffer = Framebuffer();
  }

  void RendererDirectX11::RemoveBlendState( Render::BlendStateHandle blendStateHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mBlendStates[ ( int )blendStateHandle ] );
  }

  void RendererDirectX11::RemoveConstantBuffer( Render::ConstantBufferHandle constantBufferHandle, Errors& )
  {
    ConstantBuffer* constantBuffer = &mConstantBuffers[ ( int )constantBufferHandle ];
    TAC_RELEASE_IUNKNOWN( constantBuffer->mBuffer );
    *constantBuffer = ConstantBuffer();
  }

  void RendererDirectX11::RemoveDepthState( Render::DepthStateHandle depthStateHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mDepthStencilStates[ ( int )depthStateHandle ] );
  }

  void RendererDirectX11::UpdateTextureRegion( Render::CommandDataUpdateTextureRegion* commandData,
                                               Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    TAC_UNUSED_PARAMETER( errors );
    Render::TexUpdate* data = &commandData->mTexUpdate;
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

    ID3D11Resource* dstTex = mTextures[ ( int )commandData->mTextureHandle ].mTexture2D;
    ID3D11Texture2D* srcTex;
    TAC_DX11_CALL( errors,
                   mDevice->CreateTexture2D,
                   &texDesc,
                   &subResource,
                   &srcTex );

    mDeviceContext->CopySubresourceRegion( dstTex,
                                           0, // dst subresource
                                           dstX,
                                           dstY,
                                           dstZ,
                                           srcTex,
                                           0, // src subresource,
                                           &srcBox );
    TAC_RELEASE_IUNKNOWN( srcTex );
  }

  void RendererDirectX11::UpdateVertexBuffer( Render::CommandDataUpdateVertexBuffer* commandData,
                                              Errors& errors )
  {
    ID3D11Buffer* buffer = mVertexBuffers[ ( int )commandData->mVertexBufferHandle ].mBuffer;
    UpdateBuffer( buffer, commandData->mBytes, commandData->mByteCount, errors );
  }

  void RendererDirectX11::UpdateConstantBuffer( Render::CommandDataUpdateConstantBuffer* commandData,
                                                Errors& errors )
  {
    const ConstantBuffer* constantBuffer = &mConstantBuffers[ ( int )commandData->mConstantBufferHandle ];
    UpdateBuffer( constantBuffer->mBuffer,
                  commandData->mBytes,
                  commandData->mByteCount,
                  errors );

    constantBuffers[ constantBuffer->mShaderRegister ] = constantBuffer->mBuffer;
    constantBufferCount = Max( constantBufferCount, constantBuffer->mShaderRegister + 1 );

    const UINT StartSlot = 0;
    mDeviceContext->PSSetConstantBuffers( StartSlot, constantBufferCount, constantBuffers );
    mDeviceContext->VSSetConstantBuffers( StartSlot, constantBufferCount, constantBuffers );
  }

  void RendererDirectX11::UpdateIndexBuffer( Render::CommandDataUpdateIndexBuffer* commandData,
                                             Errors& errors )
  {
    ID3D11Buffer* buffer = mIndexBuffers[ ( int )commandData->mIndexBufferHandle ].mBuffer;
    UpdateBuffer( buffer, commandData->mBytes, commandData->mByteCount, errors );
  }

  void RendererDirectX11::ResizeFramebuffer( Render::CommandDataResizeFramebuffer* data,
                                             Errors& errors )
  {
    Render::FramebufferHandle framebufferHandle = data->mFramebufferHandle;

    Framebuffer* framebuffer = &mFramebuffers[ ( int )data->mFramebufferHandle ];
    IDXGISwapChain* swapChain = framebuffer->mSwapChain;

    D3D11_TEXTURE2D_DESC depthTextureDesc;
    framebuffer->mDepthTexture->GetDesc( &depthTextureDesc );
    depthTextureDesc.Width = data->mWidth;
    depthTextureDesc.Height = data->mHeight;

    // Release outstanding back buffer references prior to calling IDXGISwapChain::ResizeBuffers
    framebuffer->mDepthStencilView->Release();
    framebuffer->mDepthStencilView = nullptr;
    framebuffer->mDepthTexture->Release();
    framebuffer->mDepthTexture = nullptr;
    framebuffer->mRenderTargetView->Release();
    framebuffer->mRenderTargetView = nullptr;

    DXGI_SWAP_CHAIN_DESC desc;
    TAC_HANDLE_ERROR_IF( FAILED( framebuffer->mSwapChain->GetDesc( &desc ) ),
                         "Failed to get swap chain desc",
                         errors );
    framebuffer->mSwapChain->ResizeBuffers( framebuffer->mBufferCount,
                                            data->mWidth,
                                            data->mHeight,
                                            DXGI_FORMAT_UNKNOWN,
                                            desc.Flags );
    ID3D11Texture2D* pBackBuffer;
    TAC_DXGI_CALL( errors, swapChain->GetBuffer, 0, IID_PPV_ARGS( &pBackBuffer ) );
    ID3D11RenderTargetView* rtv = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr;
    TAC_DX11_CALL( errors, mDevice->CreateRenderTargetView,
                   pBackBuffer,
                   rtvDesc,
                   &rtv );
    pBackBuffer->Release();

    ID3D11Texture2D* depthTexture;
    TAC_DX11_CALL( errors, mDevice->CreateTexture2D, &depthTextureDesc, nullptr, &depthTexture );

    ID3D11DepthStencilView* dsv;
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = depthTextureDesc.Format;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    TAC_DX11_CALL( errors, mDevice->CreateDepthStencilView, depthTexture, &depthStencilViewDesc, &dsv );

    framebuffer->mDepthStencilView = dsv;
    framebuffer->mDepthTexture = depthTexture;
    framebuffer->mRenderTargetView = rtv;
  }

  void RendererDirectX11::UpdateBuffer( ID3D11Buffer* buffer,
                                        const void* bytes,
                                        int byteCount,
                                        Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );
    TAC_ASSERT( Render::IsSubmitAllocated( bytes ) );
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    TAC_DX11_CALL( errors, mDeviceContext->Map, buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
    MemCpy( mappedResource.pData, bytes, byteCount );
    RendererDirectX11::Instance->mDeviceContext->Unmap( buffer, 0 );
  }


  void RendererDirectX11::DebugGroupBegin( StringView desc )
  {
    if( !mUserAnnotationDEBUG )
      return;
    TAC_ASSERT( !desc.empty() );
    LPCWSTR descWchar = ToTransientWchar( desc );
    mUserAnnotationDEBUG->BeginEvent( descWchar );

  }
  void RendererDirectX11::DebugMarker( StringView desc )
  {
    if( !mUserAnnotationDEBUG )
      return;
    LPCWSTR descWchar = ToTransientWchar( desc );
    mUserAnnotationDEBUG->SetMarker( descWchar );

  }
  void RendererDirectX11::DebugGroupEnd()
  {

    if( mUserAnnotationDEBUG )
      mUserAnnotationDEBUG->EndEvent();

  }


}
