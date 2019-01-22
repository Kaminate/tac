#include "tacRendererDirectX11.h"
#include "common/tacDesktopWindow.h"
#include "common/tacShell.h"
#include "common/tacPreprocessor.h"
//#include "common/tacUtility.h"
#include "common/tacMemory.h"
#include "common/containers/tacArray.h"
#include "common/tacAlgorithm.h"
#include "common/math/tacMath.h"
#include "tacDXGI.h"

#include <initguid.h>
#include <dxgidebug.h>
//#include <algorithm>
#include <D3DCompiler.h> // D3DCOMPILE_...
#include <d3dcommon.h> // WKPDID_D3DDebugObjectName

#include <utility> // std::pair
#include <sstream> // std::stringstream

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "D3DCompiler.lib" )

static UINT TAC_SWAP_CHAIN_FLAGS = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;;

static TacString TacTryInferDX11ErrorStr( HRESULT res )
{
  switch( res )
  {
  case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND	The file was not found.";
  case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS	There are too many unique instances of a particular type of state object.";
  case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS	There are too many unique instances of a particular type of view object.";
  case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD: return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD	The first call to ID3D11DeviceContext::Map after either ID3D11Device::CreateDeferredContext or ID3D11DeviceContext::FinishCommandList per Resource was not D3D11_MAP_WRITE_DISCARD.";
  case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL The method call is invalid.For example, a method's parameter may not be a valid pointer.";
  case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING The previous blit operation that is transferring information to or from this surface is incomplete.";
  case E_FAIL: return "E_FAIL	Attempted to create a device with the debug layer enabled and the layer is not installed.";
  case E_INVALIDARG: return "E_INVALIDARG	An invalid parameter was passed to the returning function.";
  case E_OUTOFMEMORY: return "E_OUTOFMEMORY	Direct3D could not allocate sufficient memory to complete the call.";
  case E_NOTIMPL: return "E_NOTIMPL	The method call isn't implemented with the passed parameter combination.";
  case S_FALSE: return "S_FALSE	Alternate success value, indicating a successful but nonstandard completion( the precise meaning depends on context ).";
  case S_OK: return "S_OK	No error occurred.";
  default: return "idk lol";
  }
}

static void TacDX11CallAux( const char* fnCallWithArgs, HRESULT res, TacErrors& errors )
{
  std::stringstream ss;
  ss << fnCallWithArgs << " returned 0x" << std::hex << res;
  TacString inferredErrorMessage = TacTryInferDX11ErrorStr( res );
  if( !inferredErrorMessage.empty() )
  {
    ss << "(";
    ss << inferredErrorMessage;
    ss << ")";
  }
  errors.mMessage = ss.str().c_str();
}

#define TAC_DX11_CALL( errors, call, ... )\
{\
  HRESULT result = call( __VA_ARGS__ );\
  if( FAILED( result ) )\
  {\
    TacDX11CallAux( TacStringify( call ) "( " #__VA_ARGS__ " )", result, errors );\
    TAC_HANDLE_ERROR( errors );\
  }\
}


static D3D11_TEXTURE_ADDRESS_MODE GetAddressMode( TacAddressMode addressMode )
{
  switch( addressMode )
  {
  case TacAddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP;
  case TacAddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP;
  case TacAddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
    TacInvalidDefaultCase( addressMode );
  }
  return D3D11_TEXTURE_ADDRESS_WRAP;
}

static D3D11_COMPARISON_FUNC GetCompare( TacComparison compare )
{
  switch( compare )
  {
  case TacComparison::Always: return D3D11_COMPARISON_ALWAYS;
  case TacComparison::Never: return D3D11_COMPARISON_NEVER;
    TacInvalidDefaultCase( compare );
  }
  return D3D11_COMPARISON_ALWAYS;
};

static D3D11_FILTER GetFilter( TacFilter filter )
{
  switch( filter )
  {
  case TacFilter::Linear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  case TacFilter::Point: return D3D11_FILTER_MIN_MAG_MIP_POINT;
    TacInvalidDefaultCase( filter );
  }
  return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
};

static D3D11_COMPARISON_FUNC GetDepthFunc( TacDepthFunc depthFunc )
{
  switch( depthFunc )
  {
  case TacDepthFunc::Less: return D3D11_COMPARISON_LESS;
  case TacDepthFunc::LessOrEqual: return D3D11_COMPARISON_LESS_EQUAL;
    TacInvalidDefaultCase( depthFunc );
  }
  return D3D11_COMPARISON_LESS;
}

static D3D11_USAGE GetUsage( TacAccess access )
{
  switch( access )
  {
  case TacAccess::Default: return D3D11_USAGE_DEFAULT;
  case TacAccess::Dynamic: return D3D11_USAGE_DYNAMIC;
  case TacAccess::Static: return D3D11_USAGE_IMMUTABLE;
    TacInvalidDefaultCase( access );
  }
  return D3D11_USAGE_DEFAULT;
}

static UINT GetCPUAccessFlags( std::set< TacCPUAccess > access )
{
  std::map< TacCPUAccess, UINT > accessMap;
  accessMap[ TacCPUAccess::Read ] = D3D11_CPU_ACCESS_READ;
  accessMap[ TacCPUAccess::Write ] = D3D11_CPU_ACCESS_WRITE;
  UINT result = 0;
  for( auto accessType : access )
    result |= accessMap[ accessType ];
  return result;
}

static D3D11_MAP GetD3D11_MAP( TacMap mapType )
{
  switch( mapType )
  {
  case TacMap::Read: return D3D11_MAP_READ;
  case TacMap::Write: return D3D11_MAP_WRITE;
  case TacMap::ReadWrite: return D3D11_MAP_READ_WRITE;
  case TacMap::WriteDiscard: return  D3D11_MAP_WRITE_DISCARD;
    TacInvalidDefaultCase( mapType );
  }
  return D3D11_MAP_READ_WRITE;
}

static D3D11_BLEND GetBlend( TacBlendConstants c )
{
  switch( c )
  {
  case TacBlendConstants::OneMinusSrcA:
    return D3D11_BLEND_INV_SRC_ALPHA;
  case TacBlendConstants::SrcA:
    return D3D11_BLEND_SRC_ALPHA;
  case TacBlendConstants::SrcRGB:
    return D3D11_BLEND_SRC_COLOR;
  case TacBlendConstants::Zero:
    return D3D11_BLEND_ZERO;
  case TacBlendConstants::One:
    return D3D11_BLEND_ONE;
  default:
    TacAssert( false );
    return D3D11_BLEND_ZERO;
  }
};

static D3D11_BLEND_OP GetBlendOp( TacBlendMode mode )
{
  switch( mode )
  {
  case TacBlendMode::Add:
    return D3D11_BLEND_OP_ADD;
  default:
    TacAssert( false );
    return D3D11_BLEND_OP_ADD;
  }
};

static D3D11_FILL_MODE GetFillMode( TacFillMode fillMode )
{
  switch( fillMode )
  {
  case TacFillMode::Solid: return D3D11_FILL_SOLID;
  case TacFillMode::Wireframe: return D3D11_FILL_WIREFRAME;
    TacInvalidDefaultCase( fillMode );
  }
  return ( D3D11_FILL_MODE )0;
}

static D3D11_CULL_MODE GetCullMode( TacCullMode cullMode )
{
  switch( cullMode )
  {
  case TacCullMode::None: return D3D11_CULL_NONE;
  case TacCullMode::Back: return D3D11_CULL_BACK;
  case TacCullMode::Front: return D3D11_CULL_FRONT;
    TacInvalidDefaultCase( cullMode );
  }
  return ( D3D11_CULL_MODE )0;
}

// TODO Why isn't this used?
static const char* TacGetShaderModel( TacShaderType shaderType )
{
  switch( shaderType )
  {
  case TacShaderType::Vertex: return "vs_4_0";
  case TacShaderType::Fragment: return "ps_4_0";
    TacInvalidDefaultCase( shaderType );
  }
  return nullptr;
}

// TODO Why isn't this used?
static const char* TacGetShaderEntryPoint( TacShaderType shaderType )
{
  switch( shaderType )
  {
  case TacShaderType::Vertex: return "VS";
  case TacShaderType::Fragment: return "PS";
    TacInvalidDefaultCase( shaderType );
  }
  return nullptr;
}



TacDX11Window::~TacDX11Window()
{
  CleanupRenderTarget();
  if( mSwapChain )
  {
    mSwapChain->Release();
    mSwapChain = nullptr;
  }
}

void TacDX11Window::OnResize( TacErrors& errors )
{
  CleanupRenderTarget();

  // Set this number to zero to preserve the existing number of buffers in the swap chain
  UINT bufferCount = 0;

  // If this call ever fails, it probably means that one of the backbuffer related
  // resources needs to be Release()'d
  // ie: the rtv, srv, or the texture used to create it
  TAC_DXGI_CALL( errors, mSwapChain->ResizeBuffers,
    bufferCount,
    mDesktopWindow->mWidth,
    mDesktopWindow->mHeight,
    DXGI_FORMAT_UNKNOWN, // preserve existing format
    TAC_SWAP_CHAIN_FLAGS );
  CreateRenderTarget( errors );
  TAC_HANDLE_ERROR( errors );
}

void TacDX11Window::CleanupRenderTarget()
{
  if( mBackbufferColor )
  {
    mRenderer->RemoveRendererResource( mBackbufferColor );
    mBackbufferColor = nullptr;
  }

  if( mDepthBuffer )
  {
    mRenderer->RemoveRendererResource( mDepthBuffer );
    mDepthBuffer = nullptr;
  }
}

void TacDX11Window::SwapBuffers( TacErrors & errors )
{
  bool mVsyncEnabled = true;
  UINT syncInterval = mVsyncEnabled ? 1 : 0;
  mSwapChain->Present( syncInterval, 0 );
  //mBufferIndex = ( mBufferIndex + 1 ) % mBackbufferColors.size();
}

void TacDX11Window::CreateRenderTarget( TacErrors& errors )
{
  auto renderer = ( TacRendererDirectX11* )mRenderer;
  ID3D11Device* device = renderer->mDevice;
  DXGI_SWAP_CHAIN_DESC desc;
  mSwapChain->GetDesc( &desc );

  //TacAssert( mBackbufferColors.empty() );
  //for(
  UINT i = 0;
  //i < desc.BufferCount;
  //++i )
  {
    ID3D11Texture2D* pBackBuffer;
    TAC_DXGI_CALL( errors, mSwapChain->GetBuffer, i, IID_PPV_ARGS( &pBackBuffer ) );

    ID3D11RenderTargetView* rtv = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr;
    TAC_DX11_CALL( errors, device->CreateRenderTargetView,
      pBackBuffer,
      rtvDesc,
      &rtv );
    renderer->SetDebugName( rtv, "backbuffer color rtv" );

    TacTextureData textureData = {};
    textureData.myImage.mWidth = desc.BufferDesc.Width;
    textureData.myImage.mHeight = desc.BufferDesc.Height;
    textureData.mName = "tac backbuffer color";
    textureData.mStackFrame = TAC_STACK_FRAME;

    TacTextureDX11* backbuffer;
    this->mRenderer->AddRendererResource( &backbuffer, textureData );
    backbuffer->mRTV = rtv;
    pBackBuffer->Release();
    mBackbufferColor = backbuffer;
  }

  TacDepthBufferData depthBufferData;
  depthBufferData.width = desc.BufferDesc.Width;
  depthBufferData.height = desc.BufferDesc.Height;
  depthBufferData.mName = "backbuffer depth";
  depthBufferData.mStackFrame = TAC_STACK_FRAME;

  TacDepthBuffer* depthBuffer;
  renderer->AddDepthBuffer( &depthBuffer, depthBufferData, errors );
  TAC_HANDLE_ERROR( errors );
  mDepthBuffer = ( TacDepthBufferDX11* )depthBuffer;
}

void TacDX11Window::GetCurrentBackbufferTexture( TacTexture** texture )
{
  //*texture = mBackbufferColors[ mBufferIndex ];
  *texture = mBackbufferColor;
}

TacRendererDirectX11::TacRendererDirectX11()
= default;

TacRendererDirectX11::~TacRendererDirectX11()
{

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

void TacRendererDirectX11::Init( TacErrors& errors )
{
  UINT createDeviceFlags = 0;
  if( TacIsDebugMode() )
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

  D3D_FEATURE_LEVEL featureLevel;
  auto featureLevelArray = TacMakeArray< D3D_FEATURE_LEVEL >( D3D_FEATURE_LEVEL_11_0 );

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
  // If you're directx is crashing / throwing exception, don't forget to check
  // your output window, it likes to put error messages there
  if( TacIsDebugMode() )
  {
    TAC_DX11_CALL( errors, mDevice->QueryInterface, IID_PPV_ARGS( &mInfoQueueDEBUG ) );
    TAC_DX11_CALL( errors, mDeviceContext->QueryInterface, IID_PPV_ARGS( &mUserAnnotationDEBUG ) );
  }

  mDxgi.Init( errors );
  TAC_HANDLE_ERROR( errors );
}

void TacRendererDirectX11::Render( TacErrors& errors )
{
  // uhh so i still like want to clear the first one of each frame
  TacRenderView* mCurrentlyBoundView = nullptr;

  mDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

  for( TacDrawCall2& drawCall : mDrawCall2s )
  {
    if( currentlyBoundShader != drawCall.mShader )
    {
      auto shaderDX11 = ( TacShaderDX11 * )drawCall.mShader;
      ID3D11VertexShader* vertexShader = nullptr;
      ID3D11PixelShader* pixelShader = nullptr;
      if( drawCall.mShader )
      {
        vertexShader = shaderDX11->mVertexShader;
        pixelShader = shaderDX11->mPixelShader;
        for( TacCBuffer* cbufferr : drawCall.mShader->mCBuffers )
        {
          auto cbuffer = ( TacCBufferDX11* )cbufferr;
          ID3D11Buffer* buffer = cbuffer->mDXObj;
          auto buffers = TacMakeArray< ID3D11Buffer* >( buffer );
          mDeviceContext->VSSetConstantBuffers( cbuffer->shaderRegister, buffers.size(), buffers.data() );
          mDeviceContext->PSSetConstantBuffers( cbuffer->shaderRegister, buffers.size(), buffers.data() );
        }
      }
      mDeviceContext->VSSetShader( vertexShader, nullptr, 0 );
      mDeviceContext->PSSetShader( pixelShader, nullptr, 0 );
      currentlyBoundShader = shaderDX11;
    }

    if( currentlyBoundVertexBuffer != drawCall.mVertexBuffer )
    {
      int startSlot = 0;
      const int NUM_VBOS = 16;
      ID3D11Buffer* vertexBufferHandles[ NUM_VBOS ];
      UINT strides[ NUM_VBOS ];
      UINT offsets[ NUM_VBOS ] = {};
      auto vertexBufferDX11 = ( TacVertexBufferDX11* )drawCall.mVertexBuffer;
      TacVector< TacVertexBufferDX11* > vertexBuffers;
      if( vertexBufferDX11 )
      {
        vertexBuffers.push_back( vertexBufferDX11 );
      }
      auto vertexBufferCount = ( UINT )vertexBuffers.size();
      for( int i = 0; i < ( int )vertexBufferCount; ++i )
      {
        TacVertexBuffer* vertexBuffer = vertexBuffers[ i ];
        auto myVertexBufferDX11 = ( TacVertexBufferDX11* )vertexBuffer;
        ID3D11Buffer* vertexBufferHandle = myVertexBufferDX11->mDXObj;
        strides[ i ] = myVertexBufferDX11->mStrideBytesBetweenVertexes;
        vertexBufferHandles[ i ] = vertexBufferHandle;
      }

      mDeviceContext->IASetVertexBuffers(
        startSlot,
        vertexBufferCount,
        vertexBufferHandles,
        strides,
        offsets );
      currentlyBoundVertexBuffer = ( TacVertexBufferDX11* )drawCall.mVertexBuffer;
    }

    if( currentlyBoundIndexBuffer != drawCall.mIndexBuffer )
    {
      auto indexBufferDX11 = ( TacIndexBufferDX11* )drawCall.mIndexBuffer;
      ID3D11Buffer* buffer = nullptr;
      DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
      if( indexBufferDX11 )
      {
        buffer = indexBufferDX11->mDXObj;
        format = indexBufferDX11->mFormat;
      }
      mDeviceContext->IASetIndexBuffer( buffer, format, 0 );
      currentlyBoundIndexBuffer = indexBufferDX11;

      std::cout << "changing index buffer to "
        << ( void* )indexBufferDX11
        << " "
        << indexBufferDX11->indexCount << std::endl;
    }

    if( currentlyBoundBlendState != drawCall.mBlendState )
    {
      auto blendStateDX11 = ( TacBlendStateDX11* )drawCall.mBlendState;
      ID3D11BlendState* pBlendState = nullptr; // default blend state, overwrites dst with src pixels
      if( blendStateDX11 )
        pBlendState = blendStateDX11->mDXObj;
      v4 blendFactorRGBA = { 1.0f, 1.0f, 1.0f, 1.0f };
      uint32_t sampleMask = 0xffffffff;
      mDeviceContext->OMSetBlendState(
        pBlendState,
        blendFactorRGBA.data(),
        sampleMask );
      currentlyBoundBlendState = blendStateDX11;
    }

    if( mCurrentlyBoundRasterizerState != drawCall.mRasterizerState )
    {
      auto rasterizerStateDX11 = ( TacRasterizerStateDX11* )drawCall.mRasterizerState;
      ID3D11RasterizerState* pRasterizerState = nullptr;
      if( rasterizerStateDX11 )
        pRasterizerState = rasterizerStateDX11->mDXObj;
      mDeviceContext->RSSetState( pRasterizerState );
      mCurrentlyBoundRasterizerState = rasterizerStateDX11;
    }

    if( mCurrentlyBoundDepthState != drawCall.mDepthState )
    {
      auto depthStateDX11 = ( TacDepthStateDX11* )drawCall.mDepthState;
      uint32_t stencilRef = 0;
      ID3D11DepthStencilState *pDepthStencilState = nullptr;
      if( depthStateDX11 )
        pDepthStencilState = depthStateDX11->mDXObj;
      mDeviceContext->OMSetDepthStencilState(
        pDepthStencilState,
        stencilRef );
      mCurrentlyBoundDepthState = depthStateDX11;
    }

    if( mCurrentlyBoundVertexFormat != drawCall.mVertexFormat )
    {
      auto vertexFormatDX11 = ( TacVertexFormatDX11* )drawCall.mVertexFormat;
      ID3D11InputLayout* pInputLayout = nullptr;
      if( vertexFormatDX11 )
        pInputLayout = vertexFormatDX11->mDXObj;
      mDeviceContext->IASetInputLayout( pInputLayout );
      mCurrentlyBoundVertexFormat = vertexFormatDX11;
    }

    if( drawCall.mUniformDst )
    {
      drawCall.mUniformDst->SendUniforms( drawCall.mUniformSrcc.data() );
    }

    if( mCurrentlyBoundView != drawCall.mView )
    {
      // set & clear render target
      {
        auto textureDX11 = ( TacTextureDX11* )drawCall.mView->mFramebuffer;
        auto depthBufferDX11 = ( TacDepthBufferDX11* )drawCall.mView->mFramebufferDepth;
        auto renderTargetView = ( ID3D11RenderTargetView* )textureDX11->mRTV;
        auto renderTargetViews = TacMakeArray<ID3D11RenderTargetView*>( renderTargetView );
        ID3D11DepthStencilView *pDepthStencilView = depthBufferDX11->mDSV;
        mDeviceContext->OMSetRenderTargets(
          ( UINT )renderTargetViews.size(),
          renderTargetViews.data(),
          pDepthStencilView );

        mDeviceContext->ClearRenderTargetView(
          renderTargetView,
          drawCall.mView->mClearColorRGBA.data() );

        UINT clearFlags = D3D11_CLEAR_DEPTH; // | D3D11_CLEAR_STENCIL;
        FLOAT valueToClearDepthTo = 1.0f;
        mDeviceContext->ClearDepthStencilView( pDepthStencilView, clearFlags, valueToClearDepthTo, 0 );
      }

      // set scissor rect
      {
        TacScissorRect mScissorRect = drawCall.mView->mScissorRect;
        D3D11_RECT r;
        r.left = ( LONG )mScissorRect.mXMinRelUpperLeftCornerPixel;
        r.top = ( LONG )mScissorRect.mYMinRelUpperLeftCornerPixel;
        r.right = ( LONG )mScissorRect.mXMaxRelUpperLeftCornerPixel;
        r.bottom = ( LONG )mScissorRect.mYMaxRelUpperLeftCornerPixel;
        mDeviceContext->RSSetScissorRects( 1, &r );
      }

      // set viewport rect
      {
        TacViewport viewportRect = drawCall.mView->mViewportRect;
        auto textureDX11 = ( TacTextureDX11* )drawCall.mView->mFramebuffer;

        FLOAT TopLeftX = viewportRect.mViewportBottomLeftCornerRelFramebufferBottomLeftCornerX;
        FLOAT TopLeftY
          = textureDX11->myImage.mHeight
          - viewportRect.mViewportBottomLeftCornerRelFramebufferBottomLeftCornerY
          - viewportRect.mViewportPixelHeightIncreasingUp;

        D3D11_VIEWPORT vp;
        vp.Width = viewportRect.mViewportPixelWidthIncreasingRight;
        vp.Height = viewportRect.mViewportPixelHeightIncreasingUp;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = TopLeftX;
        vp.TopLeftY = TopLeftY;
        mDeviceContext->RSSetViewports( 1, &vp );
      }

      mCurrentlyBoundView = drawCall.mView;
    }

    if( drawCall.mTexture != mCurrentlyBoundTexture )
    {
      auto textureDX11 = ( TacTextureDX11* )drawCall.mTexture;
      ID3D11ShaderResourceView* srv = nullptr;
      if( textureDX11 )
      {
        srv = textureDX11->mSrv;
        if( !srv )
        {
          TacAssertMessage( "%s should be created with shader bind flags", textureDX11->mName.c_str() );
        }
      }
      auto srvs = TacMakeArray< ID3D11ShaderResourceView* >( srv );
      mDeviceContext->VSSetShaderResources( 0, srvs.size(), srvs.data() );
      mDeviceContext->PSSetShaderResources( 0, srvs.size(), srvs.data() );
      mCurrentlyBoundTexture = textureDX11;
    }

    if( drawCall.mSamplerState != mCurrentlyBoundSamplerState )
    {
      auto samplerStateDX11 = ( TacSamplerStateDX11* )drawCall.mSamplerState;
      auto samplerStates = TacMakeArray< ID3D11SamplerState* >(
        samplerStateDX11 ? samplerStateDX11->mDXObj : nullptr );
      mDeviceContext->VSSetSamplers( 0, samplerStates.size(), samplerStates.data() );
      mDeviceContext->PSSetSamplers( 0, samplerStates.size(), samplerStates.data() );
      mCurrentlyBoundSamplerState = samplerStateDX11;
    }

    if( drawCall.mIndexCount )
    {
      mDeviceContext->DrawIndexed( drawCall.mIndexCount, drawCall.mStartIndex, 0 );
    }
  }
  mDrawCall2s.clear();
  for( TacDX11Window* window : mWindows )
  {
    window->SwapBuffers( errors );
    TAC_HANDLE_ERROR( errors );
  }
}

void TacRendererDirectX11::CreateWindowContext( TacDesktopWindow* desktopWindow, TacErrors& errors )
{
  auto hwnd = ( HWND )desktopWindow->mOperatingSystemHandle;

  IUnknown* pDevice = mDevice;
  IDXGISwapChain* mSwapChain;
  int bufferCount = 4;
  mDxgi.CreateSwapChain( hwnd, pDevice, bufferCount, desktopWindow->mWidth, desktopWindow->mHeight, &mSwapChain, errors );
  TAC_HANDLE_ERROR( errors );

  auto dx12Window = new TacDX11Window();
  dx12Window->mSwapChain = mSwapChain;
  dx12Window->mRenderer = this;
  dx12Window->CreateRenderTarget( errors );
  dx12Window->mDesktopWindow = desktopWindow;
  TAC_HANDLE_ERROR( errors );

  mWindows.push_back( dx12Window );

  desktopWindow->mRendererData = dx12Window;
}

// Make this static?
void TacRendererDirectX11::ReportLiveObjects()
{
  if( !TacIsDebugMode() )
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

void TacRendererDirectX11::AddVertexBuffer( TacVertexBuffer** outputVertexBuffer, TacVertexBufferData vbData, TacErrors& errors )
{
  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = vbData.mNumVertexes * vbData.mStrideBytesBetweenVertexes;
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  bd.Usage = GetUsage( vbData.access );
  if( vbData.access == TacAccess::Dynamic )
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  D3D11_SUBRESOURCE_DATA initData = {};
  D3D11_SUBRESOURCE_DATA* pInitialData = nullptr;
  if( vbData.optionalData )
  {
    initData.pSysMem = vbData.optionalData;
    pInitialData = &initData;
  }
  ID3D11Buffer* buffer;
  TAC_DX11_CALL( errors, mDevice->CreateBuffer, &bd, pInitialData, &buffer );
  SetDebugName( buffer, vbData.mName );
  TacVertexBufferDX11* vertexBuffer;
  AddRendererResource( &vertexBuffer, vbData );
  vertexBuffer->mDXObj = buffer;
  *outputVertexBuffer = vertexBuffer;
}

void TacRendererDirectX11::AddIndexBuffer(
  TacIndexBuffer** outputIndexBuffer,
  TacIndexBufferData indexBufferData,
  TacErrors& errors )
{
  TacAssert( indexBufferData.indexCount > 0 );

  UINT totalBufferSize
    = indexBufferData.indexCount
    * indexBufferData.dataType.mPerElementByteCount
    * indexBufferData.dataType.mElementCount;

  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = totalBufferSize;
  bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  bd.Usage = GetUsage( indexBufferData.access );
  if( indexBufferData.access == TacAccess::Dynamic )
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  ID3D11Buffer* mDXObj = nullptr;
  D3D11_SUBRESOURCE_DATA initData = {};
  D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
  if( indexBufferData.data )
  {
    pInitData = &initData;
    initData.pSysMem = indexBufferData.data;
  }
  TAC_DX11_CALL( errors, mDevice->CreateBuffer, &bd, pInitData, &mDXObj );
  SetDebugName( mDXObj, indexBufferData.mName );

  TacIndexBufferDX11* indexBuffer;
  AddRendererResource( &indexBuffer, indexBufferData );
  indexBuffer->mFormat = GetDXGIFormat( indexBufferData.dataType );
  indexBuffer->mDXObj = mDXObj;
  *outputIndexBuffer = indexBuffer;
}

void TacRendererDirectX11::ClearColor(
  TacTexture* texture,
  v4 rgba )
{
  TacAssert( texture );
  auto* textureDX11 = ( TacTextureDX11* )texture;
  TacAssert( textureDX11->mRTV );
  mDeviceContext->ClearRenderTargetView(
    textureDX11->mRTV,
    &rgba[ 0 ] );
}

void TacRendererDirectX11::ClearDepthStencil(
  TacDepthBuffer* depthBuffer,
  bool shouldClearDepth,
  float depth,
  bool shouldClearStencil,
  uint8_t stencil )
{
  UINT clearFlags = 0;
  if( shouldClearDepth ) clearFlags |= D3D11_CLEAR_DEPTH;
  if( shouldClearStencil ) clearFlags |= D3D11_CLEAR_STENCIL;
  TacAssert( depthBuffer );
  auto depthBufferDX11 = ( TacDepthBufferDX11* )depthBuffer;
  ID3D11DepthStencilView *pDepthStencilView = depthBufferDX11->mDSV;
  TacAssert( pDepthStencilView );
  mDeviceContext->ClearDepthStencilView( pDepthStencilView, clearFlags, depth, stencil );
}

static void TacCompileShaderFromString(
  ID3DBlob** ppBlobOut,
  const TacString& shaderStr,
  const char* entryPoint,
  const char* shaderModel,
  TacErrors& errors )
{
  DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
  if( TacIsDebugMode() )
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

void TacRendererDirectX11::AddShader( TacShader** outputShader, TacShaderData shaderData, TacErrors& errors )
{
  TacShaderDX11* shader;
  AddRendererResource( &shader, shaderData );
  if( !shader->mShaderPath.empty() )
  {
    shader->mShaderPath += ".fx";
    for( ;; )
    {
      ReloadShader( shader, errors );
      if( TacIsDebugMode() && errors.size() )
      {
        DebugBreak();
        errors = "";
        continue;
      }
      break;
    }
  }
  else if( !shader->mShaderStr.empty() )
  {
    LoadShaderInternal( shader, shader->mShaderStr, errors );
    if( TacIsDebugMode() )
    {
      TacAssert( errors.empty() );
    }
  }
  else
  {
    TacInvalidCodePath;
  }
  mShaders.insert( shader );
  *outputShader = shader;
}

void TacRendererDirectX11::LoadShaderInternal(
  TacShaderDX11* shader,
  TacString str,
  TacErrors& errors )
{

  auto temporaryMemory = TacTemporaryMemory( "assets/common.fx", errors );
  TAC_HANDLE_ERROR( errors );

  TacString common( temporaryMemory.data(), ( int )temporaryMemory.size() );
  str = common + str;




  {
    ID3DBlob* pVSBlob;

    TacCompileShaderFromString(
      &pVSBlob,
      str,
      "VS",
      "vs_4_0",
      errors );
    TAC_HANDLE_ERROR( errors );
    OnDestruct( pVSBlob->Release() );

    TAC_DX11_CALL(
      errors,
      mDevice->CreateVertexShader,
      pVSBlob->GetBufferPointer(),
      pVSBlob->GetBufferSize(),
      nullptr,
      &shader->mVertexShader );
    SetDebugName( shader->mVertexShader, shader->mName );

    TAC_DX11_CALL(
      errors,
      D3DGetBlobPart,
      pVSBlob->GetBufferPointer(),
      pVSBlob->GetBufferSize(),
      D3D_BLOB_INPUT_SIGNATURE_BLOB,
      0,
      &shader->mInputSig );
    //hr = D3DGetInputSignatureBlob(
    //  pVSBlob->GetBufferPointer(),
    //  pVSBlob->GetBufferSize(),
    //  &shader->mInputSig );
    //TacAssert( SUCCEEDED( hr ) );
  }
  {
    ID3DBlob* pPSBlob;
    TacCompileShaderFromString(
      &pPSBlob,
      str,
      "PS",
      "ps_4_0",
      errors );
    TAC_HANDLE_ERROR( errors );
    OnDestruct( pPSBlob->Release() );

    TAC_DX11_CALL(
      errors,
      mDevice->CreatePixelShader,
      pPSBlob->GetBufferPointer(),
      pPSBlob->GetBufferSize(),
      nullptr,
      &shader->mPixelShader );
    SetDebugName( shader->mPixelShader, shader->mName );
  }
}

void TacRendererDirectX11::ReloadShader( TacShader* shader, TacErrors& errors )
{
  TacAssert( shader );
  auto* shaderDX11 = ( TacShaderDX11* )shader;
  if( shaderDX11->mShaderPath.empty() )
    return;

  if( shaderDX11->mVertexShader )
  {
    shaderDX11->mVertexShader->Release();
    shaderDX11->mVertexShader = nullptr;
  }

  if( shaderDX11->mPixelShader )
  {
    shaderDX11->mPixelShader->Release();
    shaderDX11->mPixelShader = nullptr;
  }

  auto temporaryMemory = TacTemporaryMemory( shaderDX11->mShaderPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacString shaderStr( temporaryMemory.data(), ( int )temporaryMemory.size() );
  LoadShaderInternal( shaderDX11, shaderStr, errors );
  TAC_HANDLE_ERROR( errors );
}

void TacRendererDirectX11::GetShaders( TacVector< TacShader* >&shaders )
{
  for( auto shader : mShaders )
    shaders.push_back( shader );
}

void TacRendererDirectX11::AddSamplerState(
  TacSamplerState** outputSamplerState,
  TacSamplerStateData samplerStateData,
  TacErrors& errors )
{
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
  SetDebugName( samplerState, samplerStateData.mName );

  TacSamplerStateDX11* samplerStateDX11;
  AddRendererResource( &samplerStateDX11, samplerStateData );
  samplerStateDX11->mDXObj = samplerState;
  *outputSamplerState = samplerStateDX11;
}

void TacRendererDirectX11::AddSampler(
  const TacString& name,
  TacShader* shader,
  TacShaderType shaderType,
  int samplerIndex )
{
  auto* shaderDX11 = ( TacShaderDX11* )shader;

  auto mySampler = new TacSampler();
  mySampler->mName = name;
  mySampler->mSamplerIndex = samplerIndex;
  mySampler->mShaderType = shaderType;
  shaderDX11->mSamplers.push_back( mySampler );
}

void TacRendererDirectX11::SetSamplerState(
  const TacString& samplerName,
  TacSamplerState* samplerState )
{
  //TacAssert( mCurrentShader );
  //auto sampler = mCurrentShader->FindSampler( samplerName );
  //TacAssert( sampler );
  //auto& samplers = mCurrentSamplers[ sampler->mShaderType ];
  //int requiredSize = sampler->mSamplerIndex + 1;
  //if( samplers.size() < requiredSize )
  //  samplers.resize( requiredSize );
  //samplers[ sampler->mSamplerIndex ] = ( TacSamplerStateDX11* )samplerState;
  //mCurrentSamplersDirty.insert( sampler->mShaderType );

}

void TacRendererDirectX11::AddTextureResource(
  TacTexture** outputTexture,
  TacTextureData textureData,
  TacErrors& errors )
{
  ID3D11Resource* dXObj;
  CreateTexture(
    textureData.myImage,
    &dXObj,
    textureData.access,
    textureData.cpuAccess,
    textureData.binding,
    textureData.mName,
    errors );
  TAC_HANDLE_ERROR( errors );
  SetDebugName( dXObj, textureData.mName );

  ID3D11RenderTargetView* rTV = nullptr;
  if( TacContains( textureData.binding, TacBinding::RenderTarget ) )
  {
    TAC_DX11_CALL( errors, mDevice->CreateRenderTargetView,
      dXObj,
      nullptr,
      &rTV );
    SetDebugName( rTV, textureData.mName + " rtv" );
  }

  ID3D11ShaderResourceView* srv = nullptr;
  if( TacContains( textureData.binding, TacBinding::ShaderResource ) )
  {
    CreateShaderResourceView(
      dXObj,
      &srv,
      textureData.mName,
      errors );
    TAC_HANDLE_ERROR( errors );
  }

  TacTextureDX11* textureDX11;
  AddRendererResource( &textureDX11, textureData );
  textureDX11->mRTV = rTV;
  textureDX11->mSrv = srv;
  textureDX11->mDXObj = dXObj;
  mTextures.insert( textureDX11 );
  *outputTexture = textureDX11;
}

void TacRendererDirectX11::RemoveTextureResoure( TacTexture* texture )
{
  auto tex = ( TacTextureDX11* )texture;
  mTextures.erase( tex );
  delete texture;
}

void TacRendererDirectX11::AddTexture(
  const TacString& name,
  TacShader* shaderID,
  TacShaderType shaderType,
  int samplerIndex )
{
  TacAssert( shaderID );
  auto* shader = ( TacShaderDX11* )shaderID;

  if( TacIsDebugMode() )
  {
    for( auto sampler : shader->mTextures )
    {
      // make sure we don't overwrite a texture
      if( sampler->mShaderType == shaderType )
      {
        TacAssert( sampler->mSamplerIndex != samplerIndex );
      }
    }
  }

  auto mySampler = new TacSampler();
  mySampler->mName = name;
  mySampler->mSamplerIndex = samplerIndex;
  mySampler->mShaderType = shaderType;
  shader->mTextures.push_back( mySampler );
}

void TacRendererDirectX11::SetTexture(
  const TacString& name,
  TacTexture* texture )
{
  //TacAssert( mCurrentShader );
  //auto sampler = mCurrentShader->FindTexture( name );
  //TacAssert( sampler );
  //// todo: make this not shit
  //auto& textures = mCurrentTextures[ sampler->mShaderType ];
  //int requiredSize = sampler->mSamplerIndex + 1;
  //if( textures.size() < requiredSize )
  //  textures.resize( requiredSize );
  //textures[ sampler->mSamplerIndex ] = ( TacTextureDX11* )texture;
  //mCurrentTexturesDirty.insert( sampler->mShaderType );
}

void TacRendererDirectX11::CopyTextureRegion(
  TacTexture* dst,
  TacImage src,
  int x,
  int y,
  TacErrors& errors )
{
  int z = 0;
  D3D11_BOX srcBox = {};
  srcBox.right = src.mWidth;
  srcBox.bottom = src.mHeight;
  srcBox.back = 1;

  TacTexture* srcTexture;
  TacTextureData textureData;
  textureData.access = TacAccess::Default;
  textureData.binding = {};
  textureData.cpuAccess = {};
  textureData.mName = "temp copy texture";
  textureData.mStackFrame = TAC_STACK_FRAME;
  textureData.myImage = src;
  AddTextureResource( &srcTexture, textureData, errors );
  TAC_HANDLE_ERROR( errors );
  auto srcTextureDX11 = ( TacTextureDX11* )srcTexture;
  auto dstTextureDX11 = ( TacTextureDX11* )dst;

  ID3D11Resource* dstTex = dstTextureDX11->mDXObj;
  ID3D11Resource* srcTex = srcTextureDX11->mDXObj;
  mDeviceContext->CopySubresourceRegion(
    dstTex,
    0, // dst subresource
    x,
    y,
    z,
    srcTex,
    0, // src subresource,
    &srcBox );
  RemoveTextureResoure( srcTexture );
}


void TacRendererDirectX11::GetTextures( TacVector< TacTexture* >& textures )
{
  for( auto texture : mTextures )
    textures.push_back( texture );
}

// non-virtual ---

// why does this function exist?
void TacRendererDirectX11::CreateTexture(
  const TacImage& myImage,
  ID3D11Resource** resource,
  TacAccess access,
  std::set< TacCPUAccess > cpuAccess,
  std::set< TacBinding > binding,
  const TacString& debugName,
  TacErrors& errors )
{
  DXGI_FORMAT format = GetDXGIFormat( myImage.mFormat );
  D3D11_USAGE Usage = GetUsage( access );
  UINT cpuAccessFlags = GetCPUAccessFlags( cpuAccess );
  UINT BindFlags = 0;
  if( TacContains( binding, TacBinding::RenderTarget ) )
    BindFlags |= D3D11_BIND_RENDER_TARGET;
  if( TacContains( binding, TacBinding::ShaderResource ) )
    BindFlags |= D3D11_BIND_SHADER_RESOURCE;


  D3D11_TEXTURE2D_DESC texDesc = {};
  texDesc.Width = myImage.mWidth;
  texDesc.Height = myImage.mHeight;
  texDesc.MipLevels = 1;
  texDesc.SampleDesc.Count = 1;
  texDesc.ArraySize = 1;
  texDesc.Format = format;
  texDesc.Usage = Usage;
  texDesc.BindFlags = BindFlags;
  texDesc.CPUAccessFlags = cpuAccessFlags;

  // D3D11_SUBRESOURCE_DATA structure
  // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476220(v=vs.85).aspx
  // You set SysMemPitch to the distance between any two adjacent pixels on different lines.
  // You set SysMemSlicePitch to the size of the entire 2D surface in bytes.
  D3D11_SUBRESOURCE_DATA subResource = {};
  subResource.pSysMem = myImage.mData;
  subResource.SysMemPitch = myImage.mPitch;
  subResource.SysMemSlicePitch = myImage.mPitch * myImage.mHeight; // <-- I guess

  D3D11_SUBRESOURCE_DATA* pSubResource = myImage.mData ? &subResource : nullptr;
  TAC_DX11_CALL( errors, mDevice->CreateTexture2D, &texDesc, pSubResource, ( ID3D11Texture2D** )resource );
}

void TacRendererDirectX11::CreateShaderResourceView(
  ID3D11Resource* resource,
  ID3D11ShaderResourceView** srv,
  const TacString& debugName,
  TacErrors& errors )
{
  D3D11_RESOURCE_DIMENSION type;
  resource->GetType( &type );

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  switch( type )
  {
  case D3D11_RESOURCE_DIMENSION_UNKNOWN:
  {
    TacAssert( false );
  } break;
  case D3D11_RESOURCE_DIMENSION_BUFFER:
  {
    TacAssert( false );
  } break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
  {
    TacAssert( false );
  } break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
  {
    D3D11_TEXTURE2D_DESC desc2d;
    ( ( ID3D11Texture2D* )resource )->GetDesc( &desc2d );
    srvDesc.Format = desc2d.Format;
    srvDesc.ViewDimension = desc2d.SampleDesc.Count > 1
      ? D3D11_SRV_DIMENSION_TEXTURE2DMS
      : D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc2d.MipLevels;
  } break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
  {
    TacAssert( false );
  } break;
  default:
  {
    TacAssert( false );
  } break;
  }

  TAC_DX11_CALL( errors, mDevice->CreateShaderResourceView, resource, &srvDesc, srv );
  SetDebugName( *srv, debugName );
}

void TacRendererDirectX11::AddDepthBuffer(
  TacDepthBuffer** outputDepthBuffer,
  TacDepthBufferData depthBufferData,
  TacErrors& errors )
{
  D3D11_TEXTURE2D_DESC texture2dDesc = {};
  texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  texture2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  texture2dDesc.Height = depthBufferData.height;
  texture2dDesc.Width = depthBufferData.width;
  texture2dDesc.SampleDesc.Count = 1;
  texture2dDesc.SampleDesc.Quality = 0;
  texture2dDesc.ArraySize = 1;
  texture2dDesc.MipLevels = 1;

  ID3D11Texture2D* texture;
  TAC_DX11_CALL( errors, mDevice->CreateTexture2D, &texture2dDesc, nullptr, &texture );
  SetDebugName( texture, depthBufferData.mName );

  ID3D11DepthStencilView* dsv;
  D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
  desc.Format = texture2dDesc.Format;
  desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  TAC_DX11_CALL( errors, mDevice->CreateDepthStencilView, texture, &desc, &dsv );
  SetDebugName( dsv, depthBufferData.mName );

  TacDepthBufferDX11* depthBufferDX11;
  AddRendererResource( &depthBufferDX11, depthBufferData );
  depthBufferDX11->mDXObj = texture;
  depthBufferDX11->mDSV = dsv;
  *outputDepthBuffer = depthBufferDX11;
}

void TacRendererDirectX11::AddConstantBuffer( TacCBuffer** outputCbuffer, TacCBufferData cBufferData, TacErrors& errors )
{
  ID3D11Buffer* cbufferhandle;
  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = TacRoundUpToNearestMultiple( cBufferData.byteCount, 16 );
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  TAC_DX11_CALL( errors, mDevice->CreateBuffer, &bd, nullptr, &cbufferhandle );
  SetDebugName( cbufferhandle, cBufferData.mName );
  TacCBufferDX11* cBufferDX11;
  AddRendererResource( &cBufferDX11, cBufferData );
  cBufferDX11->mDXObj = cbufferhandle;
  *outputCbuffer = cBufferDX11;
}

void TacRendererDirectX11::AddBlendState( TacBlendState** outputBlendState, TacBlendStateData blendStateData, TacErrors& errors )
{
  D3D11_RENDER_TARGET_BLEND_DESC d3d11rtbd = {};
  d3d11rtbd.BlendEnable = true;
  d3d11rtbd.SrcBlend = GetBlend( blendStateData.srcRGB );
  d3d11rtbd.DestBlend = GetBlend( blendStateData.dstRGB );
  d3d11rtbd.BlendOp = GetBlendOp( blendStateData.blendRGB );
  d3d11rtbd.SrcBlendAlpha = GetBlend( blendStateData.srcA );
  d3d11rtbd.DestBlendAlpha = GetBlend( blendStateData.dstA );
  d3d11rtbd.BlendOpAlpha = GetBlendOp( blendStateData.blendA );
  d3d11rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  D3D11_BLEND_DESC desc = {};
  desc.RenderTarget[ 0 ] = d3d11rtbd;
  ID3D11BlendState* mDXObj;
  TAC_DX11_CALL( errors, mDevice->CreateBlendState, &desc, &mDXObj );
  SetDebugName( mDXObj, blendStateData.mName );
  TacBlendStateDX11* blendState;
  AddRendererResource( &blendState, blendStateData );
  blendState->mDXObj = mDXObj;
  *outputBlendState = blendState;
}


void TacRendererDirectX11::AddRasterizerState(
  TacRasterizerState** outputRasterizerState,
  TacRasterizerStateData rasterizerStateData,
  TacErrors& errors )
{
  D3D11_RASTERIZER_DESC desc = {};
  desc.FillMode = GetFillMode( rasterizerStateData.fillMode );
  desc.CullMode = GetCullMode( rasterizerStateData.cullMode );
  desc.ScissorEnable = rasterizerStateData.scissor;
  desc.MultisampleEnable = rasterizerStateData.multisample;
  desc.DepthClipEnable = true;
  desc.FrontCounterClockwise = rasterizerStateData.frontCounterClockwise;
  ID3D11RasterizerState* rasterizerState;
  TAC_DX11_CALL( errors, mDevice->CreateRasterizerState, &desc, &rasterizerState );
  SetDebugName( rasterizerState, rasterizerStateData.mName );
  TacRasterizerStateDX11* rasterizerStateDX11;
  AddRendererResource( &rasterizerStateDX11, rasterizerStateData );
  rasterizerStateDX11->mDXObj = rasterizerState;
  *outputRasterizerState = rasterizerStateDX11;
}

void TacRendererDirectX11::AddDepthState(
  TacDepthState** outputDepthState,
  TacDepthStateData depthStateData,
  TacErrors& errors )
{
  D3D11_DEPTH_STENCIL_DESC desc = {};
  desc.DepthFunc = GetDepthFunc( depthStateData.depthFunc );
  desc.DepthEnable = depthStateData.depthTest;
  desc.DepthWriteMask
    = depthStateData.depthWrite
    ? D3D11_DEPTH_WRITE_MASK_ALL
    : D3D11_DEPTH_WRITE_MASK_ZERO;
  ID3D11DepthStencilState* depthStencilState;
  TAC_DX11_CALL( errors, mDevice->CreateDepthStencilState, &desc, &depthStencilState );
  SetDebugName( depthStencilState, depthStateData.mName );
  TacDepthStateDX11* depthState;
  AddRendererResource( &depthState, depthStateData );
  depthState->mDXObj = depthStencilState;
  *outputDepthState = depthState;
}

void TacRendererDirectX11::AddVertexFormat(
  TacVertexFormat** outputVertexFormat,
  TacVertexFormatData vertexFormatDataa,
  TacErrors& errors )
{
  TacVector< D3D11_INPUT_ELEMENT_DESC > inputElementDescs;
  for( TacVertexDeclaration curTacFormat : vertexFormatDataa.vertexFormatDatas )
  {
    D3D11_INPUT_ELEMENT_DESC curDX11Input = {};
    curDX11Input.Format = GetDXGIFormat( curTacFormat.mTextureFormat );
    curDX11Input.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    curDX11Input.InstanceDataStepRate = 0;
    curDX11Input.SemanticName = TacGetSemanticName( curTacFormat.mAttribute );
    curDX11Input.AlignedByteOffset = curTacFormat.mAlignedByteOffset;
    inputElementDescs.push_back( curDX11Input );
  }

  auto myShaderDX11 = ( TacShaderDX11* )vertexFormatDataa.shader;
  ID3D11InputLayout* inputLayout;
  TAC_DX11_CALL(
    errors,
    mDevice->CreateInputLayout,
    inputElementDescs.data(),
    ( UINT )inputElementDescs.size(),
    myShaderDX11->mInputSig->GetBufferPointer(),
    myShaderDX11->mInputSig->GetBufferSize(),
    &inputLayout );
  SetDebugName( inputLayout, vertexFormatDataa.mName );
  TacVertexFormatDX11* vertexFormatDX11;
  AddRendererResource( &vertexFormatDX11, vertexFormatDataa );
  vertexFormatDX11->mDXObj = inputLayout;
  *outputVertexFormat = vertexFormatDX11;
}


void TacRendererDirectX11::DebugBegin( const TacString& section )
{
  if( !TacIsDebugMode() )
    return;
  std::wstring s( section.data(), section.c_str() + section.size() );
  mUserAnnotationDEBUG->BeginEvent( s.c_str() );
}

void TacRendererDirectX11::DebugMark( const TacString& remark )
{
  if( !TacIsDebugMode() )
    return;
  std::wstring s( remark.c_str(), remark.c_str() + remark.size() );
  mUserAnnotationDEBUG->SetMarker( s.c_str() );
}

void TacRendererDirectX11::DebugEnd()
{
  if( !TacIsDebugMode() )
    return;
  mUserAnnotationDEBUG->EndEvent();
}

void TacRendererDirectX11::DrawNonIndexed( int vertCount )
{
  mDeviceContext->Draw( vertCount, 0 );
}

void TacRendererDirectX11::DrawIndexed(
  int elementCount,
  int idxOffset,
  int vtxOffset )
{
  mDeviceContext->DrawIndexed( elementCount, idxOffset, vtxOffset );
}


void TacRendererDirectX11::Apply()
{
  //TacAssert( mCurrentShader );

  if( !mCurrentSamplersDirty.empty() )
  {
    for( auto shaderType : mCurrentSamplersDirty )
    {
      TacVector< ID3D11SamplerState* >ppSamplers;
      for( auto sampler : mCurrentSamplers.at( shaderType ) )
        ppSamplers.push_back( sampler->mDXObj );

      switch( shaderType )
      {
      case TacShaderType::Vertex:
        mDeviceContext->VSSetSamplers( 0, ( UINT )ppSamplers.size(), ppSamplers.data() );
        break;
      case TacShaderType::Fragment:
        mDeviceContext->PSSetSamplers( 0, ( UINT )ppSamplers.size(), ppSamplers.data() );
        break;
        TacInvalidDefaultCase( shaderType );
      }
    }
    mCurrentSamplersDirty.clear();
  }

  if( !mCurrentTexturesDirty.empty() )
  {
    for( auto shaderType : mCurrentTexturesDirty )
    {
      const auto& shaderTypeCurrentTextures = mCurrentTextures.at( shaderType );
      TacVector< ID3D11ShaderResourceView*> ppSRVs;
      for( auto texture : shaderTypeCurrentTextures )
        ppSRVs.push_back( texture->mSrv );
      switch( shaderType )
      {
      case TacShaderType::Vertex:
        mDeviceContext->VSSetShaderResources( 0, ( UINT )ppSRVs.size(), ppSRVs.data() );
        break;
      case TacShaderType::Fragment:
        mDeviceContext->PSSetShaderResources( 0, ( UINT )ppSRVs.size(), ppSRVs.data() );
        break;
        TacInvalidDefaultCase( shaderType );
      }
    }
    mCurrentTexturesDirty.clear();
  }
}

void TacRendererDirectX11::SetViewport(
  float xRelBotLeftCorner,
  float yRelBotLeftCorner,
  float wIncreasingRight,
  float hIncreasingUp )
{
  auto curRenderTarget = mCurRenderTargets[ 0 ];
  D3D11_VIEWPORT vp;
  vp.Width = wIncreasingRight;
  vp.Height = hIncreasingUp;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = xRelBotLeftCorner;
  vp.TopLeftY = curRenderTarget->myImage.mHeight - ( yRelBotLeftCorner + hIncreasingUp );
  mDeviceContext->RSSetViewports( 1, &vp );
}

void TacRendererDirectX11::SetPrimitiveTopology( TacPrimitive primitive )
{
  const D3D_PRIMITIVE_TOPOLOGY dxPrimitiveTopology = [ primitive ]()
  {
    switch( primitive )
    {
    case TacPrimitive::TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case TacPrimitive::LineList: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
      TacInvalidDefaultCase( primitive );
    }
    return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
  }( );
  mDeviceContext->IASetPrimitiveTopology( dxPrimitiveTopology );
}

void TacRendererDirectX11::SetScissorRect(
  float x1,
  float y1,
  float x2,
  float y2 )
{
  const D3D11_RECT r = {
    ( LONG )x1,
    ( LONG )y1,
    ( LONG )x2,
    ( LONG )y2 };

  mDeviceContext->RSSetScissorRects( 1, &r );
}

// private functions and data


// TODO: wtf, hr is reassigned before being used
TacString TacRendererDirectX11::AppendInfoQueueMessage( HRESULT hr )
{
  if( !TacIsDebugMode() )
    return "";
  // GetMessage() is called...
  // - 1st to get the message length,
  // - 2nd to get the message itself,
  SIZE_T messageLen = 0;
  hr = mInfoQueueDEBUG->GetMessageA( 0, nullptr, &messageLen );
  if( FAILED( hr ) )
    return "Failed to get info queue message";
  TacVector< char > data( ( int )messageLen );
  auto pMessage = ( D3D11_MESSAGE* )data.data();
  hr = mInfoQueueDEBUG->GetMessageA( 0, pMessage, &messageLen );
  if( FAILED( hr ) )
    return "Failed to get message";
  // NOTE( N8 ): length may include the null terminator
  return TacString( pMessage->pDescription, ( int )pMessage->DescriptionByteLength );
}

void TacRendererDirectX11::GetPerspectiveProjectionAB(
  float f,
  float n,
  float& a,
  float& b )
{
  TacAssert( f > n );

  float invDenom = 1.0f / ( n - f );

  // ( A, B ) maps ( -n, -f ) to ( 0, 1 )
  // because clip space in directx is [ -1, 1 ][ -1, 1 ][ 0, 1 ]
  // note that clip space in opengl is [ -1, 1 ][ -1, 1 ][ -1, 1 ]
  // todo: double check this function
  a = f * invDenom;
  b = f * invDenom * n;
}

void TacRendererDirectX11::SetDebugName(
  ID3D11DeviceChild* directXObject,
  const TacString& name )
{
  TacAssert( name.size() );
  if( !TacIsDebugMode() )
    return;
  TacAssert( directXObject );
  const int buffersize = 256;
  char data[ buffersize ] = {};

  UINT pDataSize = buffersize;
  directXObject->GetPrivateData(
    WKPDID_D3DDebugObjectName,
    &pDataSize,
    &data );

  TacString newname;
  if( pDataSize )
  {
    newname += TacString( data, pDataSize );
    newname += " --and-- ";

    auto hide = TacMakeArray< D3D11_MESSAGE_ID >( D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS );
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
TacSampler* TacShaderDX11::Find( TacVector< TacSampler* >& samplers, const TacString& name )
{
  for( auto sampler : samplers )
  {
    if( sampler->mName == name )
      return sampler;
  }
  return nullptr;
}

TacShaderDX11::~TacShaderDX11()
{
  TAC_RELEASE_IUNKNOWN( mVertexShader );
  TAC_RELEASE_IUNKNOWN( mPixelShader );
  auto rendererDX11 = ( TacRendererDirectX11* )mRenderer;
  if( rendererDX11->currentlyBoundShader == this )
    rendererDX11->currentlyBoundShader = nullptr;
}

TacSampler* TacShaderDX11::FindTexture( const TacString& name )
{
  return Find( mTextures, name );
}

TacSampler* TacShaderDX11::FindSampler( const TacString& name )
{
  return Find( mSamplers, name );
}


TacTextureDX11::~TacTextureDX11()
{
  TAC_RELEASE_IUNKNOWN( mDXObj );
  TAC_RELEASE_IUNKNOWN( mSrv );
  TAC_RELEASE_IUNKNOWN( mRTV );
  auto rendererDX11 = ( TacRendererDirectX11* )mRenderer;
  if( rendererDX11->mCurrentlyBoundTexture == this )
    rendererDX11->mCurrentlyBoundTexture = nullptr;
  // if the rtv is the currently bound view, null the view?
}

void* TacTextureDX11::GetImguiTextureID()
{
  return this;
}

static void TacOverwrite( TacRenderer* rendererr, ID3D11Resource* resource, void* data, int byteCount, TacErrors& errors )
{
  auto renderer = ( TacRendererDirectX11* )rendererr;
  ID3D11DeviceContext* deviceContext = renderer->mDeviceContext;
  D3D11_MAP d3d11mapType = GetD3D11_MAP( TacMap::WriteDiscard );
  D3D11_MAPPED_SUBRESOURCE mappedResource;
  TAC_DX11_CALL( errors, deviceContext->Map, resource, 0, d3d11mapType, 0, &mappedResource );
  TacMemCpy( mappedResource.pData, data, byteCount );
  renderer->mDeviceContext->Unmap( resource, 0 );
}

TacVertexBufferDX11::~TacVertexBufferDX11()
{

  TAC_RELEASE_IUNKNOWN( mDXObj );
  auto rendererDX11 = ( TacRendererDirectX11* )mRenderer;
  if( rendererDX11->currentlyBoundVertexBuffer == this )
    rendererDX11->currentlyBoundVertexBuffer = nullptr;
}

void TacVertexBufferDX11::Overwrite( void* data, int byteCount, TacErrors& errors )
{
  TacOverwrite( mRenderer, mDXObj, data, byteCount, errors );
}

TacIndexBufferDX11::~TacIndexBufferDX11()
{
  TAC_RELEASE_IUNKNOWN( mDXObj );
  auto rendererDX11 = ( TacRendererDirectX11* )mRenderer;
  if( rendererDX11->currentlyBoundIndexBuffer == this )
    rendererDX11->currentlyBoundIndexBuffer = nullptr;
}

void TacIndexBufferDX11::Overwrite( void* data, int byteCount, TacErrors& errors )
{
  TacOverwrite( mRenderer, mDXObj, data, byteCount, errors );
}

TacCBufferDX11::~TacCBufferDX11()
{

  TAC_RELEASE_IUNKNOWN( mDXObj );
}

void TacCBufferDX11::SendUniforms( void* bytes )
{
  auto renderer = ( TacRendererDirectX11* )mRenderer;
  ID3D11Resource* resource = mDXObj;
  renderer->mDeviceContext->UpdateSubresource(
    resource,
    0,
    nullptr,
    bytes,
    0,
    0 );
}

int registerDX11 = []()
{
  static struct TacDirectX11RendererFactory : public TacRendererFactory
  {
    TacDirectX11RendererFactory()
    {
      mRendererName = RendererNameDirectX11;
    }
    void CreateRenderer( TacRenderer** renderer ) override
    {
      *renderer = new TacRendererDirectX11();
    }
  } factory;
  TacRendererFactory::GetRegistry().push_back( &factory );
  return 0;
}( );

TacDepthBufferDX11::~TacDepthBufferDX11()
{
  TAC_RELEASE_IUNKNOWN( mDXObj );
  TAC_RELEASE_IUNKNOWN( mDSV );
  // null the view if bound?
}

TacSamplerStateDX11::~TacSamplerStateDX11()
{

  TAC_RELEASE_IUNKNOWN( mDXObj );
  auto rendererDX11 = ( TacRendererDirectX11* )mRenderer;
  if( rendererDX11->mCurrentlyBoundSamplerState == this )
    rendererDX11->mCurrentlyBoundSamplerState = nullptr;
}

TacDepthStateDX11::~TacDepthStateDX11()
{
  TAC_RELEASE_IUNKNOWN( mDXObj );
  auto rendererDX11 = ( TacRendererDirectX11* )mRenderer;
  if( rendererDX11->mCurrentlyBoundDepthState == this )
    rendererDX11->mCurrentlyBoundDepthState = nullptr;
}

TacBlendStateDX11::~TacBlendStateDX11()
{
  TAC_RELEASE_IUNKNOWN( mDXObj );
  auto rendererDX11 = ( TacRendererDirectX11* )mRenderer;
  if( rendererDX11->currentlyBoundBlendState == this )
    rendererDX11->currentlyBoundBlendState = nullptr;
}

TacRasterizerStateDX11::~TacRasterizerStateDX11()
{
  TAC_RELEASE_IUNKNOWN( mDXObj );
  auto rendererDX11 = ( TacRendererDirectX11* )mRenderer;
  if( rendererDX11->mCurrentlyBoundRasterizerState == this )
    rendererDX11->mCurrentlyBoundRasterizerState = nullptr;
}

TacVertexFormatDX11::~TacVertexFormatDX11()
{
  TAC_RELEASE_IUNKNOWN( mDXObj );

  auto rendererDX11 = ( TacRendererDirectX11* )mRenderer;
  if( rendererDX11->mCurrentlyBoundVertexFormat == this )
    rendererDX11->mCurrentlyBoundVertexFormat = nullptr;
}
