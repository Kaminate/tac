/*


// ok heres the fuckin deal:
// you want to set event markers and event groups for convenient graphcis debugging
// so you see these nice functions, like ID3D12GraphicsCommandList::SetMarker .
//
// you go to the page
// https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12graphicscommandlist-setmarker
// and it says 
//   This is a support method used internally by the PIX event runtime.
//   It is not intended to be called directly.
//   To insert instrumentation markers at the current location within a D3D12 command list,
//   use the PIXSetMarker function.  This is provided by the WinPixEventRuntime NuGet package.
//
// But you can't use the nuget package because
// 1. you would rather kill yourself than have nuget fuck you raw in the ass
// 2. you generate your project with cmake
//
// So you decide to live without debug markers


#include "src/common/Utility.h"
#include "src/common/Memory.h"
#include "src/common/Preprocessor.h"
#include "src/common/containers/Array.h"
#include "src/common/math/Math.h"
#include "src/common/OS.h"
#include "src/common/DesktopWindow.h"
#include "src/shell/windows/Windows.h"
#include "src/shell/windows/RendererDirectX12.h"

#include <sstream>
#include <d3d11_1.h>


// TODO: precompiled shaders
#include <d3dcompiler.h>
#pragma comment( lib, "d3dcompiler.lib" )

#pragma comment( lib, "d3d12.lib" )

namespace Tac
{


bool isGraphicsDebugging = true;

static D3D12_COMPARISON_FUNC GetDepthFuncDX12( DepthFunc depthFunc )
{
  switch( depthFunc )
  {
  case DepthFunc::Less: return D3D12_COMPARISON_FUNC_LESS;
  case DepthFunc::LessOrEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    TAC_ASSERT_INVALID_DEFAULT_CASE( depthFunc );
  }
  return D3D12_COMPARISON_FUNC_LESS;
}

static D3D12_FILL_MODE GetFillModeDX12( FillMode fillMode )
{
  switch( fillMode )
  {
  case FillMode::Solid: return D3D12_FILL_MODE_SOLID;
  case FillMode::Wireframe:return D3D12_FILL_MODE_WIREFRAME;
    TAC_ASSERT_INVALID_DEFAULT_CASE( fillMode );
  }
  return D3D12_FILL_MODE_SOLID;
}

static D3D12_CULL_MODE GetCullModeDX12( CullMode cullMode )
{
  switch( cullMode )
  {
  case CullMode::None: return D3D12_CULL_MODE_NONE;
  case CullMode::Back: return D3D12_CULL_MODE_BACK;
  case CullMode::Front: return D3D12_CULL_MODE_FRONT;
    TAC_ASSERT_INVALID_DEFAULT_CASE( cullMode );
  }
  return D3D12_CULL_MODE_NONE;
}

static D3D12_BLEND GetBlendDX12( BlendConstants blendConstants )
{
  switch( blendConstants )
  {
  case BlendConstants::One: return D3D12_BLEND_ONE;
  case BlendConstants::Zero: return D3D12_BLEND_ZERO;
  case BlendConstants::SrcRGB: return D3D12_BLEND_SRC_COLOR;
  case BlendConstants::SrcA: return D3D12_BLEND_SRC_ALPHA;
  case BlendConstants::OneMinusSrcA: return D3D12_BLEND_INV_SRC_ALPHA;
    TAC_ASSERT_INVALID_DEFAULT_CASE( blendConstants );
  }
  return D3D12_BLEND_ONE;
}

static D3D12_BLEND_OP GetBlendOpDX12( BlendMode blendMode )
{
  switch( blendMode )
  {
  case BlendMode::Add: return D3D12_BLEND_OP_ADD;
    TAC_ASSERT_INVALID_DEFAULT_CASE( blendMode );
  }
  return D3D12_BLEND_OP_ADD;
}

static Vector< WCHAR > ToWChar( const String& str )
{
  Vector< WCHAR > result;
  for( char c : str )
    result.push_back( c );
  result.push_back( ( WCHAR )'\0' );
  return result;
}

static void NameDirectX12Object( ID3D12Object* obj, const String& name )
{
  Vector< WCHAR > nameWchar = ToWChar( name );
  HRESULT hr = obj->SetName( nameWchar.data() );
  Assert( hr == S_OK );
}

void DX12Window::Submit( Errors& errors )
{
  // Present swap chain
  {
    // syncInterval is An integer that specifies how to synchronize presentation of a frame with the vertical blank.
    // see also: https://docs.microsoft.com/en-us/windows/desktop/api/dxgi/nf-dxgi-idxgiswapchain-present
    UINT syncInterval = 1;

    // 0 denotes that we are presenting a frame from each buffer (starting with the current buffer) to the output.
    UINT syncFlags = 0;
    HRESULT hr = mSwapChain->Present( syncInterval, syncFlags );
    if( hr != S_OK )
    {
      switch( hr )
      {
      case DXGI_ERROR_DEVICE_RESET: errors = "DXGI_ERROR_DEVICE_RESET"; break;
      case DXGI_ERROR_DEVICE_REMOVED: errors = "DXGI_ERROR_DEVICE_REMOVED"; break;
      case DXGI_STATUS_OCCLUDED: errors = "DXGI_STATUS_OCCLUDED"; break;
          TAC_ASSERT_INVALID_DEFAULT_CASE( hr );
      }
      TAC_HANDLE_ERROR( errors );
    }
  }
  mBackbufferIndex = ( mBackbufferIndex + 1 ) % mBackbufferColors.size();
}

void DX12Window::DebugDoubleCheckBackbufferIndex()
{
  if( !IsDebugMode() )
    return;
  ComPtr<IDXGISwapChain3> swapChain3;
  HRESULT hr = mSwapChain.As( &swapChain3 );
  if( hr == S_OK )
    return;
  UINT curBBIndex = swapChain3->GetCurrentBackBufferIndex();
  Assert( curBBIndex == mBackbufferIndex );
}

void DX12Window::GetCurrentBackbufferTexture( Texture** texture )
{
  DebugDoubleCheckBackbufferIndex();
  *texture = mBackbufferColors[ mBackbufferIndex ];
}

static String TryInferDX12ErrorStr( HRESULT res )
{
  switch( res )
  {
  case DXGI_ERROR_INVALID_CALL:
    return "DXGI_ERROR_INVALID_CALL: The method call is invalid.For example, a method's parameter may not be a valid pointer.";
  case DXGI_ERROR_WAS_STILL_DRAWING:
    return "DXGI_ERROR_WAS_STILL_DRAWING: The previous blit operation that is transferring information to or from this surface is incomplete.";
  case E_FAIL:
    return "E_FAIL: Attempted to create a device with the debug layer enabled and the layer is not installed.";
  case E_INVALIDARG:
    return "E_INVALIDARG: An invalid parameter was passed to the returning function.";
  case E_OUTOFMEMORY:
    return "E_OUTOFMEMORY: Direct3D could not allocate sufficient memory to complete the call.";
  case E_NOTIMPL:
    return "E_NOTIMPL: The method call isn't implemented with the passed parameter combination.";
  case S_FALSE:
    return "S_FALSE: Alternate success value, indicating a successful but nonstandard completion( the precise meaning depends on context ).";
  case S_OK:
    return "S_OK: No error occurred.";
  default:
    return "";
  }
}

void DX12CallAux( const char* fnCallWithArgs, HRESULT res, Errors& errors )
{
  std::stringstream ss;
  ss << fnCallWithArgs << " returned 0x" << std::hex << res;
  String inferredErrorMessage = TryInferDX12ErrorStr( res );
  if(!inferredErrorMessage.empty())
  {
    ss << "(";
    ss << inferredErrorMessage;
    ss << ")";
  }
  errors.mMessage = ss.str().c_str();
}

#define TAC_DX12_CALL( errors, call, ... )\
{\
  HRESULT result = call( __VA_ARGS__ );\
  if( FAILED( result ) )\
  {\
    DX12CallAux( Stringify( call ) "( " #__VA_ARGS__ " )", result, errors );\
    TAC_HANDLE_ERROR( errors );\
  }\
}

struct RendererBufferDX12
{
  void CrateMainBuffer( RendererDX12* renderer, const String& name, Errors& errors, D3D12_RESOURCE_STATES resourceStates, int byteCount )
  {
    ID3D12Device* device = RendererDX12::Instance->mDevice.Get();
    ID3D12Resource* resource;
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = byteCount;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES properties = {};
    properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    TAC_DX12_CALL(
      errors,
      device->CreateCommittedResource,
      &properties,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      resourceStates,
      nullptr, // clear value
      IID_PPV_ARGS( &resource ) );
    TAC_HANDLE_ERROR( errors );
    NameDirectX12Object( resource, name );
    mResourceBuffer = resource;
    mState = resourceStates;
  }
  void CreateUploadBuffer( RendererDX12* renderer, const String& name, Errors& errors, int byteCount )
  {
    ID3D12Device* device = RendererDX12::Instance->mDevice.Get();
    ID3D12Resource* uploadResource;

    // https://docs.microsoft.com/en-us/windows/desktop/direct3d12/uploading-resources#code-example-d3d12
    // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_resource_desc
    D3D12_RESOURCE_DESC uploadResourceDesc = {};
    uploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadResourceDesc.Width = byteCount;
    uploadResourceDesc.Height = 1;
    uploadResourceDesc.DepthOrArraySize = 1;
    uploadResourceDesc.MipLevels = 1;
    uploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    uploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    uploadResourceDesc.SampleDesc.Count = 1;
    uploadResourceDesc.SampleDesc.Quality = 0;
    uploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // mostly for texture stuff
    D3D12_HEAP_PROPERTIES uploadProps = {};
    uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    TAC_DX12_CALL(
      errors,
      device->CreateCommittedResource,
      &uploadProps,
      D3D12_HEAP_FLAG_NONE,
      &uploadResourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, // gpu will read from this buffer and copy its contents somewhere
      nullptr, // optimized clear value
      IID_PPV_ARGS( &uploadResource ) );
    TAC_HANDLE_ERROR( errors );
    NameDirectX12Object( uploadResource, name + "\"upload object\"" );
    mResourceStaging = uploadResource;
  }
  void UpdateBuffer( RendererDX12* renderer, void* bytes, int byteCount, Errors& errors )
  {
    //D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_COPY_DEST;
    ID3D12GraphicsCommandList* commandList = RendererDX12::Instance->mCommandList.Get();
    ID3D12Resource* dstResource = mResourceBuffer.Get();
    ID3D12Resource* srcResource = mResourceStaging.Get();

    RendererDX12::Instance->ResourceBarrier( dstResource, mState, D3D12_RESOURCE_STATE_COPY_DEST );

    D3D12_RANGE range;
    range.Begin = 0;
    range.End = ( SIZE_T )byteCount;
    void* mappedData;
    UINT subresource = 0;
    TAC_DX12_CALL( errors, mResourceStaging->Map, subresource, &range, &mappedData );
    MemCpy( mappedData, bytes, byteCount );
    mResourceStaging->Unmap( subresource, &range );
    UINT64 dstOffset = 0;
    UINT64 srcOffset = 0;
    commandList->CopyBufferRegion( dstResource, dstOffset, srcResource, srcOffset, ( UINT64 )byteCount );

    RendererDX12::Instance->ResourceBarrier( dstResource, mState, D3D12_RESOURCE_STATE_COMMON );
  }

  D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;
  ComPtr< ID3D12Resource > mResourceStaging;
  ComPtr< ID3D12Resource > mResourceBuffer;
};

struct VertexBufferDX12 : public VertexBuffer
{
  RendererBufferDX12 mBuffer;
  void Overwrite( void* data, int byteCount, Errors& errors ) override
  {
    auto renderer = ( RendererDX12* )Renderer::Instance;
    mBuffer.UpdateBuffer( renderer, data, byteCount, errors );
  };
};

struct IndexBufferDX12 : public IndexBuffer
{
  RendererBufferDX12 mBuffer;
  void Overwrite( void* data, int byteCount, Errors& errors )  override
  {
    auto renderer = ( RendererDX12* )Renderer::Instance;
    mBuffer.UpdateBuffer( renderer, data, byteCount, errors );
  }
};

struct ShaderDX12 : public Shader
{
  ComPtr< ID3DBlob > mBlobVertexShader;
  ComPtr< ID3DBlob > mBlobFragmentShader;
  ComPtr< ID3D12RootSignature > mRootSignature;
};


void ConstantBufferDX12::SendUniforms( void* bytes )
{
  // Do I need a resource barrier here?
  MemCpy( mMappedData, bytes, byteCount );
}

RendererDX12* RendererDX12::Instance = nullptr;
RendererDX12::RendererDX12()
{
  RendererDX12::Instance = this;
}

void RendererDX12::Init( Errors& errors )
{
  mDXGI.Init( errors );
  TAC_HANDLE_ERROR( errors );

  bool shouldCreateDebugLayer = IsDebugMode() &&
    // https://github.com/Microsoft/DirectX-Graphics-Samples/issues/158
    !isGraphicsDebugging;

  // Enable the D3D12 debug layer before creating the device, or else the device will be removed
  if( shouldCreateDebugLayer )
  {
    ID3D12Debug* debugController;
    TAC_DX12_CALL( errors, D3D12GetDebugInterface, IID_PPV_ARGS( &debugController ) );
    debugController->EnableDebugLayer();
    mDebugController = debugController;
  }

  ID3D12Device* device;
  TAC_DX12_CALL( errors, D3D12CreateDevice, mDXGI.mDxgiAdapter4, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &device ) );
  NameDirectX12Object( device, "tac dx12 device" );
  mDevice = device;



  if( IsDebugMode() && mDebugController )
  {
    TAC_DX12_CALL( errors, mDevice.As, &mInfoQueue );
    mInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE );
    mInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
    mInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE );
  }

  // rtv desc heap
  {
    ID3D12DescriptorHeap* rtvDescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeap = {};
    rtvDescHeap.NumDescriptors = 1024; // ?
    rtvDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescHeap.NodeMask = 1;
    TAC_DX12_CALL( errors, mDevice->CreateDescriptorHeap, &rtvDescHeap, IID_PPV_ARGS( &rtvDescriptorHeap ) );
    NameDirectX12Object( rtvDescriptorHeap, "tac rtv desc heap" );
    mRTVDescriptorHeap = rtvDescriptorHeap;
  }

  // dsv desc heap
  {
    ID3D12DescriptorHeap* dsvDescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC dsvDescHeap = {};
    dsvDescHeap.NumDescriptors = 128; // ?
    dsvDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDescHeap.NodeMask = 1;
    TAC_DX12_CALL( errors, mDevice->CreateDescriptorHeap, &dsvDescHeap, IID_PPV_ARGS( &dsvDescriptorHeap ) );
    NameDirectX12Object( dsvDescriptorHeap, "tac dsv desc heap" );
    mDSVDescriptorHeap = dsvDescriptorHeap;
  }

  // scratch desc heap
  {
    // ok so like, mScratchDescriptorHeap will contains all of our descriptors that we allocate the constant buffer views from
    // and then like, we'll copy the ones that the shader is using to mScratchDescriptorHeapCopyDest, cuz they need to be contiguous
    // or some shit.

    // this heap will store the descriptors to our constant buffer
    // ^ which means?
    ID3D12DescriptorHeap* scratchDescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC scratchDescHeap = {};
    scratchDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    scratchDescHeap.NumDescriptors = 1024; // idfk
    scratchDescHeap.NodeMask = 1;
    scratchDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // cpu read + write
    TAC_DX12_CALL( errors, mDevice->CreateDescriptorHeap, &scratchDescHeap, IID_PPV_ARGS( &scratchDescriptorHeap ) );
    NameDirectX12Object( scratchDescriptorHeap, "tac scratch desc heap src" );
    mScratchDescriptorHeap = scratchDescriptorHeap;

    scratchDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // cpu write only
    TAC_DX12_CALL( errors, mDevice->CreateDescriptorHeap, &scratchDescHeap, IID_PPV_ARGS( &scratchDescriptorHeap ) );
    NameDirectX12Object( scratchDescriptorHeap, "tac scratch desc heap dst" );
    mScratchDescriptorHeapCopyDest = scratchDescriptorHeap;
  }

  // cmd queue
  {
    ID3D12CommandQueue* commandQueue;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 1;
    TAC_DX12_CALL( errors, mDevice->CreateCommandQueue, &queueDesc, IID_PPV_ARGS( &commandQueue ) );
    NameDirectX12Object( commandQueue, "my command queue idk" );
    mCommandQueue = commandQueue;
  }

  // bgfx uses 256 of these cmd thingies in a ring buffer
  {
    // a command allocator is...
    ID3D12CommandAllocator* commandAllocator;
    TAC_DX12_CALL( errors, mDevice->CreateCommandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &commandAllocator ) );
    NameDirectX12Object( commandAllocator, "tac command allocator" );
    mCommandAllocator = commandAllocator;

    ID3D12GraphicsCommandList* commandList;
    TAC_DX12_CALL( errors, mDevice->CreateCommandList, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, NULL, IID_PPV_ARGS( &commandList ) );
    TAC_DX12_CALL( errors, commandList->Close );
    TAC_DX12_CALL( errors, commandList->Reset, commandAllocator, NULL );
    NameDirectX12Object( commandList, "tac command list" );
    mCommandList = commandList;
  }

  // fence
  {
    mFenceValue = 0;
    ID3D12Fence* fence;
    TAC_DX12_CALL( errors, mDevice->CreateFence, mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) );
    NameDirectX12Object( fence, "my fence idk" );
    mFence = fence;
  }
}

void RendererDX12::CreateWindowContext( DesktopWindow* desktopWindow, Errors& errors )
{
  UINT bufferCount = 4; // ?
  //IDXGISwapChain* swapChain;
  IDXGISwapChain1* swapChain;
  {
    auto hwnd = ( HWND )desktopWindow->mOperatingSystemHandle;
    IUnknown* pDevice = mCommandQueue.Get(); // not an ID3D12Device

    DXGI_SWAP_CHAIN_DESC1 scd1 = {};
    {
      scd1.Width = ( UINT )desktopWindow->mWidth;
      scd1.Height = ( UINT )desktopWindow->mHeight;
      scd1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      scd1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      scd1.SampleDesc.Count = 1;
      scd1.BufferCount = bufferCount;
      scd1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      scd1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC scfsd = {};
    {
      scfsd.RefreshRate.Numerator = 1;
      scfsd.RefreshRate.Denominator = 60;
      scfsd.Windowed = TRUE;
      scfsd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
      scfsd.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    }

    // This call deprecates IDXGIFactory::CreateSwapChain
    TAC_DX12_CALL( errors, mDXGI.mFactory->CreateSwapChainForHwnd, pDevice, hwnd, &scd1, &scfsd, NULL, &swapChain );

    NameDXGIObject( swapChain, desktopWindow->mName + " swap chain" );
  }

  // TODO: how the fuck does any of this work?
  uint32_t rtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
  D3D12_CPU_DESCRIPTOR_HANDLE handle = mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  Vector< TextureDX12* > backbufferColors;
  for( UINT i = 0; i < bufferCount; ++i )
  {
    ID3D12Resource* backbufferColor;
    TAC_DX12_CALL( errors, swapChain->GetBuffer, i, IID_PPV_ARGS( &backbufferColor ) );
    String debugName = desktopWindow->mName + " backbufer " + ToString( ( int )i );
    //Vector< WCHAR > debugNameWchar = ToWChar( debugName );
    //backbufferColor->SetName( debugNameWchar.data() );
    NameDirectX12Object( backbufferColor, debugName );

    DXGI_FORMAT backbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
    renderTargetViewDesc.Format = backbufferFormat;
    renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    //mDevice->CreateRenderTargetView( backbufferColor, NULL, handle );
    mDevice->CreateRenderTargetView( backbufferColor, &renderTargetViewDesc, handle );

    auto texture = new TextureDX12();
    texture->mCpuDescriptorHandle = handle;
    texture->mResource = backbufferColor;
    texture->myImage.mWidth = desktopWindow->mWidth;
    texture->myImage.mHeight = desktopWindow->mHeight;
    texture->mDxgiFormat = backbufferFormat;
    //texture->myImage.mFormat = GetFormat( backbufferFormat );

    handle.ptr += rtvDescriptorSize;
    backbufferColors.push_back( texture );

  }


  DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  ID3D12Resource* backbufferDepthStencil;
  D3D12_CPU_DESCRIPTOR_HANDLE backbufferDepthCpuDescriptorHandle;
  {
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width = desktopWindow->mWidth;
    resourceDesc.Height = desktopWindow->mHeight;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = depthBufferFormat;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES properties = {};
    properties.Type = D3D12_HEAP_TYPE_CUSTOM;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = depthBufferFormat;
    clearValue.DepthStencil.Depth = 1;
    clearValue.DepthStencil.Stencil = 0;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    TAC_DX12_CALL(
      errors,
      mDevice->CreateCommittedResource,
      &properties,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      state,
      &clearValue,
      IID_PPV_ARGS( &backbufferDepthStencil ) );
    backbufferDepthCpuDescriptorHandle = mDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    mDevice->CreateDepthStencilView( backbufferDepthStencil, nullptr, backbufferDepthCpuDescriptorHandle );

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = backbufferDepthStencil;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    mCommandList->ResourceBarrier( 1, &barrier );
  }

  auto depthBufferDX12 = new DepthBufferDX12();
  depthBufferDX12->mBackbufferDepthStencil = backbufferDepthStencil;
  depthBufferDX12->mBackbufferDepthCpuDescriptorHandle = backbufferDepthCpuDescriptorHandle;
  depthBufferDX12->mDxgiFormat = depthBufferFormat;

  auto dx12Window = new DX12Window();
  dx12Window->mSwapChain = swapChain;
  dx12Window->mBackbufferColors = backbufferColors;
  dx12Window->mDepthBuffer = depthBufferDX12;

  desktopWindow->mRendererData = dx12Window;
  mWindows.push_back( dx12Window );
}

void RendererDX12::AddVertexBuffer( VertexBuffer** vertexBuffer, const VertexBufferData& vertexBufferData, Errors& errors )
{
  int byteCount = vertexBufferData.mNumVertexes * vertexBufferData.mStrideBytesBetweenVertexes;
  D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  VertexBufferDX12* vertexBufferDX12;
  AddRendererResource( &vertexBufferDX12, vertexBufferData );
  vertexBufferDX12->mBuffer.CrateMainBuffer( this, vertexBufferData.mName, errors, resourceStates, byteCount );
  vertexBufferDX12->mBuffer.CreateUploadBuffer( this, vertexBufferData.mName, errors, byteCount );
  *vertexBuffer = vertexBufferDX12;
}

void RendererDX12::AddIndexBuffer( IndexBuffer** indexBuffer, const IndexBufferData& indexBufferData, Errors& errors )
{
  Assert( indexBufferData.mFormat.mElementCount == 1 );
  int byteCount = indexBufferData.mIndexCount * indexBufferData.mFormat.mPerElementByteCount;
  D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_INDEX_BUFFER;
  IndexBufferDX12* indexBufferDX12;
  AddRendererResource( &indexBufferDX12, indexBufferData );
  indexBufferDX12->mBuffer.CrateMainBuffer( this, indexBufferData.mName, errors, resourceStates, byteCount );
  indexBufferDX12->mBuffer.CreateUploadBuffer( this, indexBufferData.mName, errors, byteCount );
  *indexBuffer = indexBufferDX12;
}

void RendererDX12::AddConstantBuffer( CBuffer** outputCbuffer, const CBufferData& cBufferData, Errors& errors )
{
  Assert( cBufferData.byteCount );

  D3D12_HEAP_PROPERTIES heapProperties = {};
  heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_CLEAR_VALUE* clearValue = nullptr;
  D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_GENERIC_READ;

  D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAG_NONE;

  D3D12_RESOURCE_DESC resourceDesc = {};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = ( UINT64 )RoundUpToNearestMultiple( cBufferData.byteCount, 256 );
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  ID3D12Resource* resource = nullptr;
  TAC_DX12_CALL( errors, mDevice->CreateCommittedResource, &heapProperties, HeapFlags, &resourceDesc, resourceStates, clearValue, IID_PPV_ARGS( &resource ) );
  NameDirectX12Object( resource, cBufferData.mName + "upload heap" );
  mCBufferUpload = resource;


  D3D12_RANGE cpuReadRange = {};
  // this shouldnt matter since the cpu isn't reading the data
  cpuReadRange.End = ( SIZE_T )cBufferData.byteCount;
  void* mappedData;
  UINT subresouce = 0;
  TAC_DX12_CALL( errors, resource->Map, subresouce, &cpuReadRange, &mappedData );
  // memcpy shader uniforms here
  //m4 world = {};
  //world = m4::Identity();
  //MemCpy( mappedData, &world, sizeof( m4 ) );
  // stay permanently mapped to avoid map/unmap overhead
  //resource->Unmap( 0, &cpuReadRange );


  // Create a constant buffer view, which
  // - describes the constant buffer, and 
  // - contains a pointer to the memory where the constant buffer resides
  D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
  constantBufferViewDesc.BufferLocation = mCBufferUpload->GetGPUVirtualAddress();
  constantBufferViewDesc.SizeInBytes = ( UINT )cBufferData.byteCount;

  D3D12_CPU_DESCRIPTOR_HANDLE destDescriptorCPU = mScratchDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  destDescriptorCPU.ptr += scratchIncrement;

  D3D12_GPU_DESCRIPTOR_HANDLE destDescriptorGPU = mScratchDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
  destDescriptorGPU.ptr += scratchIncrement;

  scratchIncrement += mDevice->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

  mDevice->CreateConstantBufferView( &constantBufferViewDesc, destDescriptorCPU );








  ConstantBufferDX12* constantBufferDX12;
  AddRendererResource( &constantBufferDX12, cBufferData );
  constantBufferDX12->mCBufferDestDescriptorCPU = destDescriptorCPU;
  constantBufferDX12->mCBufferDestDescriptorGPU = destDescriptorGPU;
  constantBufferDX12->mMappedData = mappedData;

  *outputCbuffer = constantBufferDX12;
}

void RendererDX12::AddShader( Shader** shader, const ShaderData& shaderData, Errors& errors )
{
  struct ShaderPart
  {
    String mEntryPoint;
    String mShaderVersion;
    ID3DBlob* blobShader = nullptr;
  };

  ShaderPart shaderPartVertex;
  shaderPartVertex.mEntryPoint = "VS";
  shaderPartVertex.mShaderVersion = "vs_5_0";

  ShaderPart shaderPartFragment;
  shaderPartFragment.mEntryPoint = "PS";
  shaderPartFragment.mShaderVersion = "ps_5_0";
  for( ;; )
  {
    Vector< char > String;
    if(!shaderData.mShaderPath.empty())
    {
      String shaderPath = shaderData.mShaderPath + ".fx";
      String = FileToString( shaderPath, errors );
      TAC_HANDLE_ERROR( errors );
    }

    Vector< char > common = FileToString( "assets/common.fx", errors );
    TAC_HANDLE_ERROR( errors );

    // Using a string instead of a vector because it's null terminated,
    // which means it will debug visualizes better
    String shaderMemory;
    for( char c : common )
      shaderMemory.push_back( c );
    for( char c : shaderData.mShaderStr )
      shaderMemory.push_back( c );
    for( char c : String )
      shaderMemory.push_back( c );


    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    if( IsDebugMode() )
    {
      flags |= D3DCOMPILE_DEBUG;
    }


    auto CompileShaderPart = [ & ]( ShaderPart* shaderPart )
    {
      // https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/d3dcompile
      ID3DBlob* blobShader;
      ID3DBlob* blobErrors;

      // You can use this parameter for strings that specify error messages. (???)
      LPCSTR pSourceName = nullptr;
      D3D_SHADER_MACRO* pDefines = nullptr;
      ID3DInclude* pInclude = nullptr;

      // https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/d3dcompile-effect-constants 
      // Used only when compiling effects ( unused for shaders )
      UINT Flags2 = 0;


      HRESULT hr = D3DCompile(
        shaderMemory.data(),
        shaderMemory.size(),
        pSourceName,
        pDefines,
        pInclude,
        shaderPart->mEntryPoint.c_str(),
        shaderPart->mShaderVersion.c_str(),
        flags,
        Flags2,
        &blobShader,
        &blobErrors );
      if( FAILED( hr ) )
      {
        errors = "failed to compile shader";
        if(!shaderData.mShaderPath.empty())
          errors += " from file " + shaderData.mShaderPath;
        errors += "\n";

        String blobErrorsString = String(
          ( char* )blobErrors->GetBufferPointer() );

        // this like includes the null which i guess makes sense, but i don't want
        // that as part of my string...
        //( int )blobErrors->GetBufferSize() );

        errors += blobErrorsString;
        return;
      }
      shaderPart->blobShader = blobShader;
    };

    ShaderPart* failedShaderPart = nullptr;
    //bool shouldRetry = false;
    for( ShaderPart* shaderPart : { &shaderPartVertex, &shaderPartFragment } )
    {
      CompileShaderPart( shaderPart );
      if( errors )
      {
        failedShaderPart = shaderPart;
        break;
      }
    }

    if( !failedShaderPart )
      break;
    if( !IsDebugMode() )
      return;


    String compositeShaderPath = Shell::Instance.mPrefPath + "/FailedShader.txt";
    Errors compositeShaderErrors;
    WriteToFile( compositeShaderPath, shaderMemory.data(), shaderMemory.size(), compositeShaderErrors );
    String compositeShaderErrorString =
      compositeShaderErrors ?
      compositeShaderErrors.ToString() :
      "See composite shader " + compositeShaderPath;

    Vector< String > popupMessages;
    popupMessages.push_back( errors.ToString() );
    popupMessages.push_back( compositeShaderErrorString );
    popupMessages.push_back( "Specified entry point: " + failedShaderPart->mEntryPoint );
    popupMessages.push_back( "Specified shader ver: " + failedShaderPart->mShaderVersion );
    popupMessages.push_back( errors.ToString() );
    popupMessages.push_back( "Fix errors, and press OK to try again" );
    String popupMessage = Join( "\n", popupMessages.data(), popupMessages.size() );
    OSDebugPopupBox( popupMessage );
    errors.clear();
  }

  ID3D12RootSignature *rootSignature;
  {
    // ok so like a root signature has its own small heap for dirty things and global
    // state changes. so like it can have its own descriptors/descriptor table.
    //
    // You do want to have descriptors for your shaders
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dn899109(v=vs.85).aspx

    // Declare containers in outer scope, careful with those pointers, man...
    Vector< D3D12_ROOT_PARAMETER > rootParameters;
    Vector< D3D12_DESCRIPTOR_RANGE > descriptorRanges;


    for( CBuffer* cbuffer : shaderData.mCBuffers )
    {
      // The base shader register in the range.
      // For example, for shader - resource views( SRVs ), 3 maps to ": register(t3);" in HLSL.
      //UINT baseShaderRegister = 0;
        auto baseShaderRegister = ( UINT )cbuffer->shaderRegister;

      // number of descriptors in the range
      UINT descriptorCount = 1;

      // Can typically be 0, but allows multiple descriptor arrays of unknown size to not appear to overlap
      UINT registerSpace = 0;

      UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

      D3D12_DESCRIPTOR_RANGE descriptorRangeCBufferPerFrame = {};
      descriptorRangeCBufferPerFrame.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
      descriptorRangeCBufferPerFrame.NumDescriptors = descriptorCount;
      descriptorRangeCBufferPerFrame.BaseShaderRegister = baseShaderRegister;
      descriptorRangeCBufferPerFrame.RegisterSpace = registerSpace;
      descriptorRangeCBufferPerFrame.OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
      descriptorRanges.push_back( descriptorRangeCBufferPerFrame );
    }

    if( descriptorRanges.empty() )
    {
      TAC_ASSERT_MESSAGE( "No descriptor ranges, will get bullshit errors" );
    }

    D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable = {};
    {
      descriptorTable.NumDescriptorRanges = ( UINT )descriptorRanges.size();
      descriptorTable.pDescriptorRanges = descriptorRanges.data();
    }

    D3D12_ROOT_PARAMETER rootParameter = {};
    {
      rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      rootParameter.DescriptorTable = descriptorTable;
    }
    rootParameters.push_back( rootParameter );

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.NumParameters = ( UINT )rootParameters.size();
    rootSignatureDesc.pParameters = rootParameters.data();
    rootSignatureDesc.NumStaticSamplers;
    rootSignatureDesc.pStaticSamplers;
    ComPtr< ID3DBlob > signatureBlob;
    ComPtr< ID3DBlob > signatureBlobErrors;
    TAC_DX12_CALL( errors, D3D12SerializeRootSignature, &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &signatureBlobErrors );

    // For single GPU operation, set this to zero.
    // If there are multiple GPU nodes, set bits to identify the nodes( the device's physical adapters) to which the root signature is to apply.
    // Each bit in the mask corresponds to a single node.
    UINT nodeMask = 0;
    TAC_DX12_CALL(
      errors,
      mDevice->CreateRootSignature,
      nodeMask,
      signatureBlob->GetBufferPointer(),
      signatureBlob->GetBufferSize(),
      IID_PPV_ARGS( &rootSignature ) );

    NameDirectX12Object( rootSignature, shaderData.mName + " root sig" );

  }


  ShaderDX12* shaderDX12;
  AddRendererResource( &shaderDX12, shaderData );
  shaderDX12->mBlobVertexShader = shaderPartVertex.blobShader;
  shaderDX12->mBlobFragmentShader = shaderPartFragment.blobShader;
  shaderDX12->mRootSignature = rootSignature;
  *shader = shaderDX12;
}

void RendererDX12::GetPSOStuff( const DrawCall2& drawCall2, ID3D12PipelineState **pppipelineState, Errors& errors )
{
  for( PSOStuff& curPSOStuff : mPSOStuff )
  {
    if( drawCall2.mShader == curPSOStuff.mShader &&
      drawCall2.mBlendState == curPSOStuff.mBlendState &&
      drawCall2.mRasterizerState == curPSOStuff.mRasterizerState &&
      drawCall2.mDepthState == curPSOStuff.mDepthState &&
      drawCall2.mVertexFormat == curPSOStuff.mVertexFormat &&
      drawCall2.mRenderView->mFramebuffer == curPSOStuff.mFramebufferTexture &&
      drawCall2.mRenderView->mFramebufferDepth == curPSOStuff.mFramebufferDepth )
    {
      *pppipelineState = curPSOStuff.mPipelineState.Get();
      return;
    }
  }


  RenderView* renderView = drawCall2.mRenderView;
  auto backbufferColor = ( TextureDX12* )renderView->mFramebuffer;
  auto backbufferDepth = ( DepthBufferDX12* )renderView->mFramebufferDepth;


  // common variables
  auto shader = ( ShaderDX12* )drawCall2.mShader;
  ID3DBlob* vertexShader = shader->mBlobVertexShader.Get();
  ID3DBlob* fragmentShader = shader->mBlobFragmentShader.Get();
  Vector< D3D12_INPUT_ELEMENT_DESC > inputElementDescs;

  // pipeline state variables

  ID3D12RootSignature *pRootSignature = shader->mRootSignature.Get();

  D3D12_SHADER_BYTECODE VS = {};
    VS.BytecodeLength = vertexShader->GetBufferSize();
    VS.pShaderBytecode = vertexShader->GetBufferPointer();

  D3D12_SHADER_BYTECODE PS = {};
    PS.BytecodeLength = fragmentShader->GetBufferSize();
    PS.pShaderBytecode = fragmentShader->GetBufferPointer();

  D3D12_SHADER_BYTECODE DS = {};

  D3D12_SHADER_BYTECODE HS = {};

  D3D12_SHADER_BYTECODE GS = {};

  // Describes a streaming output buffer
  // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_stream_output_desc
  // bgfx leaves it all blank
  D3D12_STREAM_OUTPUT_DESC StreamOutput = {};

  D3D12_BLEND_DESC BlendState = {};
  {
    D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
    renderTargetBlendDesc.BlendEnable = TRUE;
    renderTargetBlendDesc.SrcBlend = GetBlendDX12( drawCall2.mBlendState->srcRGB );
    renderTargetBlendDesc.DestBlend = GetBlendDX12( drawCall2.mBlendState->dstRGB );
    renderTargetBlendDesc.BlendOp = GetBlendOpDX12( drawCall2.mBlendState->blendRGB );
    renderTargetBlendDesc.SrcBlendAlpha = GetBlendDX12( drawCall2.mBlendState->srcA );
    renderTargetBlendDesc.DestBlendAlpha = GetBlendDX12( drawCall2.mBlendState->dstA );
    renderTargetBlendDesc.BlendOpAlpha = GetBlendOpDX12( drawCall2.mBlendState->blendA );
    renderTargetBlendDesc.LogicOpEnable = FALSE;
    renderTargetBlendDesc.RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_RED |
      D3D12_COLOR_WRITE_ENABLE_GREEN |
      D3D12_COLOR_WRITE_ENABLE_BLUE |
      D3D12_COLOR_WRITE_ENABLE_ALPHA;

    // Specifies whether to use alpha-to-coverage as a multisampling technique when setting a pixel to a render target.
    // see also https://msdn.microsoft.com/en-us/library/Bb205072(v=VS.85).aspx
    BlendState.AlphaToCoverageEnable = FALSE;

    // Specifies whether to enable independent blending in simultaneous render targets.
    // Set to TRUE to enable independent blending.
    // If set to FALSE, only the RenderTarget[ 0 ] members are used; RenderTarget[ 1..7 ] are ignored.
    BlendState.IndependentBlendEnable = FALSE;

    // An array of D3D12_RENDER_TARGET_BLEND_DESC struct Ures that describe the blend states for render targets
    // these correspond to the eight render targets that can be bound to the output-merger stage at one time.
    BlendState.RenderTarget[ 0 ] = renderTargetBlendDesc;
  }

  // ?
  UINT SampleMask = UINT32_MAX;

  D3D12_RASTERIZER_DESC RasterizerState = {};
  {
    RasterizerState.AntialiasedLineEnable = TRUE;
    RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    RasterizerState.CullMode = GetCullModeDX12( drawCall2.mRasterizerState->cullMode );
    RasterizerState.DepthBias = 0;
    RasterizerState.DepthBiasClamp = 0;
    RasterizerState.DepthClipEnable = TRUE; // ?
    RasterizerState.FillMode = GetFillModeDX12( drawCall2.mRasterizerState->fillMode );
    RasterizerState.ForcedSampleCount = 0;
    RasterizerState.FrontCounterClockwise = ( BOOL )drawCall2.mRasterizerState->frontCounterClockwise;
    RasterizerState.MultisampleEnable = false;
    RasterizerState.SlopeScaledDepthBias = 0;
  }

  D3D12_DEPTH_STENCIL_DESC DepthStencilState = {};
  {
    // ?
    DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    // ?
    DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

    DepthStencilState.DepthEnable = drawCall2.mDepthState->depthTest;
    DepthStencilState.DepthFunc = GetDepthFuncDX12( drawCall2.mDepthState->depthFunc );
    DepthStencilState.DepthWriteMask = drawCall2.mDepthState->depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    DepthStencilState.StencilEnable = FALSE;
  }


  D3D12_INPUT_LAYOUT_DESC InputLayout = {};
  {
    for( VertexDeclaration& vertexDeclaration : drawCall2.mVertexFormat->vertexFormatDatas )
    {
      // A semantic index is only needed in a case where there is more than one element with the same semantic 
      UINT semanticIndex = 0;

      // https://docs.microsoft.com/en-us/windows/desktop/direct3d11/d3d10-graphics-programming-guide-input-assembler-stage-getting-started
      // Data enters the IA stage through inputs called input slots.
      // The IA stage has n input slots, which are designed to accommodate up to n vertex buffers that provide input data.
      // Each vertex buffer must be assigned to a different slot
      // this information is stored in the input-layout declaration when the input-layout object is created.
      UINT inputSlot = 0;

      // The number of instances to draw using the same per-instance data before advancing in the buffer by one element.
      // This value must be 0 for an element that contains per-vertex data
      UINT instanceDataStepRate = 0;

      // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_input_element_desc
      D3D12_INPUT_ELEMENT_DESC inputElementDesc = {};
      inputElementDesc.SemanticName = GetSemanticName( vertexDeclaration.mAttribute );
      inputElementDesc.SemanticIndex = semanticIndex;
      inputElementDesc.Format = GetDXGIFormat( vertexDeclaration.mTextureFormat );
      inputElementDesc.InputSlot = inputSlot;
      inputElementDesc.AlignedByteOffset = vertexDeclaration.mAlignedByteOffset;
      inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
      inputElementDesc.InstanceDataStepRate = instanceDataStepRate;
      inputElementDescs.push_back( inputElementDesc );
    }

    InputLayout.NumElements = ( UINT )inputElementDescs.size();
    InputLayout.pInputElementDescs = inputElementDescs.data();
  }

  D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue = {};
  {
    // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_index_buffer_strip_cut_value
    // When using triangle strip primitive topology, vertex positions are interpreted as vertices of a continuous triangle “strip”.
    // There is a special index value that represents the desire to have a discontinuity in the strip, the cut index value.
    // This enum lists the supported cut values.
    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED	Indicates that there is no cut value.
    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF	Indicates that 0xFFFF should be used as the cut value.
    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF	Indicates that 0xFFFFFFFF should be used as the cut value.
    IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
  }

  D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  UINT NumRenderTargets = 1;

  DXGI_FORMAT RTVFormats[ 8 ] = {};
  {
    //RTVFormats[ 0 ] = GetDXGIFormat( backbufferColor->myImage.mFormat );
    RTVFormats[ 0 ] = backbufferColor->mDxgiFormat;
  }

  DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN;
  {
    //DepthBuffer* depthBuffer = renderView->mFramebufferDepth;
    //if( depthBuffer->mDepthBitCount == 24 &&
    //  depthBuffer->mDepthGraphicsType == GraphicsType::unorm &&
    //  depthBuffer->mStencilBitCount == 8 &&
    //  depthBuffer->mStencilType == GraphicsType::uint )
    //{
    //  DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    //}
    //else
    //{
    //  InvalidCodePath;
    //}
    DSVFormat = backbufferDepth->mDxgiFormat;
  }

  DXGI_SAMPLE_DESC SampleDesc = {};
  {
    SampleDesc.Count = 1;
  }

  UINT NodeMask = 1;

  D3D12_CACHED_PIPELINE_STATE CachedPSO = {};

  D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

  // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_graphics_pipeline_state_desc
  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc = {};
  graphicsPipelineStateDesc.pRootSignature = pRootSignature;
  graphicsPipelineStateDesc.VS = VS;
  graphicsPipelineStateDesc.PS = PS;
  graphicsPipelineStateDesc.DS = DS;
  graphicsPipelineStateDesc.HS = HS;
  graphicsPipelineStateDesc.GS = GS;
  graphicsPipelineStateDesc.StreamOutput = StreamOutput;
  graphicsPipelineStateDesc.BlendState = BlendState;
  graphicsPipelineStateDesc.SampleMask = SampleMask;
  graphicsPipelineStateDesc.RasterizerState = RasterizerState;
  graphicsPipelineStateDesc.DepthStencilState = DepthStencilState;
  graphicsPipelineStateDesc.InputLayout = InputLayout;
  graphicsPipelineStateDesc.IBStripCutValue = IBStripCutValue;
  graphicsPipelineStateDesc.PrimitiveTopologyType = PrimitiveTopologyType;
  graphicsPipelineStateDesc.NumRenderTargets = NumRenderTargets;
  for( int i = 0; i < 8; ++i )
    graphicsPipelineStateDesc.RTVFormats[ i ] = RTVFormats[ i ];
  graphicsPipelineStateDesc.DSVFormat = DSVFormat;
  graphicsPipelineStateDesc.SampleDesc = SampleDesc;
  graphicsPipelineStateDesc.NodeMask = NodeMask;
  graphicsPipelineStateDesc.CachedPSO = CachedPSO;
  graphicsPipelineStateDesc.Flags = Flags;

  ID3D12PipelineState* pipelineState;
  TAC_DX12_CALL( errors, mDevice->CreateGraphicsPipelineState, &graphicsPipelineStateDesc, IID_PPV_ARGS( &pipelineState ) );
  NameDirectX12Object( pipelineState, "tac pipeline state " + ToString( mPSOStuff.size() ) );

  PSOStuff psoStuff = {};
  psoStuff.mBlendState = drawCall2.mBlendState;
  psoStuff.mDepthState = drawCall2.mDepthState;
  psoStuff.mFramebufferDepth = backbufferDepth;
  psoStuff.mFramebufferTexture = backbufferColor;
  psoStuff.mPipelineState = pipelineState;
  psoStuff.mRasterizerState = drawCall2.mRasterizerState;
  psoStuff.mShader = drawCall2.mShader;
  psoStuff.mVertexFormat = drawCall2.mVertexFormat;
  mPSOStuff.push_back( psoStuff );
  *pppipelineState = pipelineState;
}

void RendererDX12::Render( Errors& errors )
{
  bool shouldAddDrawCalls = true;
  if( shouldAddDrawCalls )
  {
    RenderView* lastRenderView = nullptr;
    // Add all draw calls to the commnd list
    for( DrawCall2& drawCall2 : mDrawCall2s )
    {
      auto shader = ( ShaderDX12* )drawCall2.mShader;
      RenderView* renderView = drawCall2.mRenderView;

      if( drawCall2.mUniformDst )
        drawCall2.mUniformDst->SendUniforms( drawCall2.mUniformSrcc.data() );

      if( renderView )
      {
        TextureDX12* backbufferColor = nullptr;
        DepthBufferDX12* backbufferDepth = nullptr;
        backbufferColor = ( TextureDX12* )renderView->mFramebuffer;
        backbufferDepth = ( DepthBufferDX12* )renderView->mFramebufferDepth;
        // set render target
        if( renderView != lastRenderView )
        {
          renderView = lastRenderView;

          // Indicate that the back buffer will be used as a render target.
          ResourceBarrier( backbufferColor->mResource.Get(), backbufferColor->mState, D3D12_RESOURCE_STATE_RENDER_TARGET );


          Vector< D3D12_CPU_DESCRIPTOR_HANDLE > renderTargetDescriptors;
          D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = backbufferColor->mCpuDescriptorHandle;
          renderTargetDescriptors.push_back( cpuDescriptorHandle );

          //Vector< D3D12_RECT > rects;
          //D3D12_RECT rect = {};
          //rect.bottom = ( LONG )backbufferColor->mHeight();
          //rect.right = ( LONG )backbufferColor->mWidth();
          //rects.push_back( rect );

          // If rects is NULL, ClearRenderTargetView clears the entire resource view.
          //mCommandList->ClearRenderTargetView( cpuDescriptorHandle, colorRGBA, ( UINT )rects.size(), rects.data() );
          mCommandList->ClearRenderTargetView( cpuDescriptorHandle, renderView->mClearColorRGBA.data(), 0, NULL );

          // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12graphicscommandlist-omsetrendertargets
          // TRUE
          //   The handle passed in is the pointer to a contiguous range of NumRenderTargetDescriptors descriptors.
          //   This case is useful if the set of descriptors to bind already happens to be contiguous in memory
          //   ( so all that’s needed is a handle to the first one ).
          //   In this case the driver dereferences the handle and then increments the memory being pointed to.
          // FALSE
          //   The handle is the first of an array of NumRenderTargetDescriptors handles.
          //   The false case allows an application to bind a set of descriptors from different locations at once.
          BOOL RTsSingleHandleToDescriptorRange = FALSE;
          D3D12_CPU_DESCRIPTOR_HANDLE pDepthStencilDescriptor = backbufferDepth->mBackbufferDepthCpuDescriptorHandle;
          mCommandList->OMSetRenderTargets(
            ( UINT )renderTargetDescriptors.size(),
            renderTargetDescriptors.data(),
            RTsSingleHandleToDescriptorRange,
            &pDepthStencilDescriptor );



        }

        // set viewport
        {
          Vector< D3D12_VIEWPORT > viewports;
          D3D12_VIEWPORT viewport = {};
          viewport.Height = ( FLOAT )backbufferColor->myImage.mHeight;
          viewport.Width = ( FLOAT )backbufferColor->myImage.mWidth;
          viewports.push_back( viewport );
          mCommandList->RSSetViewports( ( UINT )viewports.size(), viewports.data() );
        }

        // set scissor rect
        {
          Vector< D3D12_RECT > rects;
          D3D12_RECT  rect = {};
          rect.right = ( LONG )backbufferColor->myImage.mWidth;
          rect.bottom = ( LONG )backbufferColor->myImage.mHeight;
          rects.push_back( rect );
          mCommandList->RSSetScissorRects( ( UINT )rects.size(), rects.data() );
        }


        mCommandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

        // what even is this?
        //UINT RootParameterIndex = 0;
        //D3D12_GPU_VIRTUAL_ADDRESS BufferLocation = {};
        //mCommandList->SetGraphicsRootShaderResourceView( RootParameterIndex, BufferLocation );

        // set pipeline state
        {
          ID3D12PipelineState *pipelineState = nullptr;
          GetPSOStuff( drawCall2, &pipelineState, errors );
          TAC_HANDLE_ERROR( errors );
          mCommandList->SetPipelineState( pipelineState );
        }

        // D3D12_GRAPHICS_PIPELINE_STATE_DESC::pRootSignature vs ID3D12GraphicsCommandList::SetGraphicsRootSignature
        // https://www.gamedev.net/forums/topic/677663-d3d12-root-signature-difference-between-pso-and-cmdlist/

        ID3D12RootSignature* rootSignature = shader->mRootSignature.Get();
        mCommandList->SetGraphicsRootSignature( rootSignature );

        {
          // set the descriptor heap prior to setting the root descriptor table,
          // since thats apparently a thing i need to do
          D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
          D3D12_CPU_DESCRIPTOR_HANDLE dst = mScratchDescriptorHeapCopyDest->GetCPUDescriptorHandleForHeapStart();
          UINT inc = mDevice->GetDescriptorHandleIncrementSize( type );

          for( CBuffer* cBuffer : shader->mCBuffers )
          {
            auto cbuf = ( ConstantBufferDX12* )cBuffer;
            //Vector<ID3D12DescriptorHeap *> descriptorHeaps =
            //{
            //  mScratchDescriptorHeap.Get()
            //};
            //mCommandList->SetDescriptorHeaps( ( UINT )descriptorHeaps.size(), descriptorHeaps.data() );

            D3D12_CPU_DESCRIPTOR_HANDLE src = cbuf->mCBufferDestDescriptorCPU;
            mDevice->CopyDescriptorsSimple( 1, dst, src, type );
            dst.ptr += inc;

            // test
          }
          Vector<ID3D12DescriptorHeap *> descriptorHeaps =
          {
            mScratchDescriptorHeapCopyDest.Get()
          };
          mCommandList->SetDescriptorHeaps( ( UINT )descriptorHeaps.size(), descriptorHeaps.data() );
        }

        // set root descriptor table ( which includes our cbuffers )
        {
          UINT rootParameterIndex = 0;
          //D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor = mCBufferPerFrameDestDescriptorGPU;
          // test
          D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor = mScratchDescriptorHeapCopyDest->GetGPUDescriptorHandleForHeapStart();
          mCommandList->SetGraphicsRootDescriptorTable( rootParameterIndex, baseDescriptor );
        }

        // set index buffer
        {
          auto indexBuffer = ( IndexBufferDX12* )drawCall2.mIndexBuffer;
          D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
          indexBufferView.BufferLocation = indexBuffer->mBuffer.mResourceBuffer->GetGPUVirtualAddress();
          indexBufferView.Format = GetDXGIFormat( indexBuffer->mFormat );
          indexBufferView.SizeInBytes = ( UINT )( indexBuffer->mIndexCount * indexBuffer->mFormat.mPerElementByteCount );
          mCommandList->IASetIndexBuffer( &indexBufferView );
        }

        // set vertex buffer
        {
          auto vertexBuffer = ( VertexBufferDX12* )drawCall2.mVertexBuffer;
          D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
          vertexBufferView.BufferLocation = vertexBuffer->mBuffer.mResourceBuffer->GetGPUVirtualAddress();
          vertexBufferView.StrideInBytes = ( UINT )vertexBuffer->mStrideBytesBetweenVertexes;
          vertexBufferView.SizeInBytes = ( UINT )( vertexBuffer->mNumVertexes * vertexBuffer->mStrideBytesBetweenVertexes );
          mCommandList->IASetVertexBuffers( 0, 1, &vertexBufferView );
        }

        // Draw
        {
          // Number of indices read from the index buffer for each instance.
          auto IndexCountPerInstance = ( UINT )drawCall2.mIndexCount;

          // Number of instances to draw.
          UINT InstanceCount = 1;

          // The location of the first index read by the GPU from the index buffer.
          auto StartIndexLocation = ( UINT )drawCall2.mStartIndex;

          // A value added to each index before reading a vertex from the vertex buffer.
          // TODO: delete drawCall2.mStartVertex?
          INT BaseVertexLocation = 0;

          // A value added to each index before reading per - instance data from a vertex buffer.
          UINT StartInstanceLocation = 0;

          mCommandList->DrawIndexedInstanced(
            IndexCountPerInstance,
            InstanceCount,
            StartIndexLocation,
            BaseVertexLocation,
            StartInstanceLocation );
        }

        // Indicate that the back buffer will now be used to present.
        ResourceBarrier( backbufferColor->mResource.Get(), backbufferColor->mState, D3D12_RESOURCE_STATE_PRESENT );
      }
    }
  }
  mDrawCall2s.clear();
  FinishRendering( errors );
  TAC_HANDLE_ERROR( errors );
}

void RendererDX12::FinishRendering( Errors& errors )
{


  // Close the command list ( must be done prior to executing the command list )
  TAC_DX12_CALL( errors, mCommandList->Close );


  // Execute the command list ( must be done after closing the command list )
  auto commandLists = MakeArray< ID3D12CommandList* >( mCommandList.Get() );
  mCommandQueue->ExecuteCommandLists( commandLists.size(), commandLists.data() );

  for( DX12Window* window : mWindows )
  {
    window->Submit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  bool shouldSignalFence = true;
  UINT64 fenceDevicedRemovedValue = UINT64_MAX;
  UINT64 nextFenceValue = mFenceValue + 1;
  if( nextFenceValue == fenceDevicedRemovedValue )
    nextFenceValue = 0;
  if( shouldSignalFence )
  {
    // Schedule a Signal command in the queue.
    mCommandQueue->Signal( mFence.Get(), nextFenceValue );
  }

  // wait for command lists to finish executing
  for( ;; )
  {
    UINT64 completedFenceValue = mFence->GetCompletedValue();
    if( completedFenceValue == fenceDevicedRemovedValue )
    {
      errors = "the device has been removed because\n";
      errors += GetDeviceRemovedReason();
      TAC_HANDLE_ERROR( errors );
    }
    if( completedFenceValue == nextFenceValue )
      break;
    HANDLE fenceEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );
    Assert( fenceEvent );
    OnDestruct( CloseHandle( fenceEvent ) );
    HRESULT hr = mFence->SetEventOnCompletion( nextFenceValue, fenceEvent );
    if( FAILED( hr ) )
    {
      errors = "failed to set fence event";
      TAC_HANDLE_ERROR( errors );
    }
    WaitForSingleObject( fenceEvent, INFINITE );
  }
  mFenceValue = nextFenceValue;


  // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12commandallocator-reset
  // Indicates to re-use the memory that is associated with the command allocator.
  // should be done... before resetting the command list?
  HRESULT hr = mCommandAllocator->Reset();
  if( hr != S_OK )
  {
    if( hr == E_FAIL )
    {
      errors = "There is an actively recording command list referencing the command allocator";
    }
    else
    {
      errors = "Failed to reset command allocator " + TryInferDX12ErrorStr( hr );
    }
    TAC_HANDLE_ERROR( errors );
  }
  // should be done... after resetting the command allocator?
  TAC_DX12_CALL( errors, mCommandList->Reset, mCommandAllocator.Get(), nullptr );
}

void RendererDX12::ResourceBarrier( ID3D12Resource* resource, D3D12_RESOURCE_STATES& oldState, const D3D12_RESOURCE_STATES& newState )
{
  D3D12_RESOURCE_BARRIER barrier;
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = resource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = oldState;
  barrier.Transition.StateAfter = newState;
  mCommandList->ResourceBarrier( 1, &barrier );
  oldState = newState;
}

String RendererDX12::GetDeviceRemovedReason()
{
  HRESULT hr = mDevice->GetDeviceRemovedReason();
  return GetDeviceRemovedReason( hr );
}

String RendererDX12::GetDeviceRemovedReason( HRESULT hr )
{
  switch( hr )
  {
  case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG The graphics driver stopped responding because of an invalid combination of graphics commands sent by the app.If you get this error repeatedly, it is a likely indication that your app caused the device to hang and needs to be debugged.";
  case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED The graphics device has been physically removed, turned off, or a driver upgrade has occurred.This happens occasionally and is normal; your app or game should recreate device resources as described in this topic.";
  case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET The graphics device failed because of a badly formed command.If you get this error repeatedly, it may mean that your code is sending invalid drawing commands.";
  case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR The graphics driver encountered an error and reset the device.";
  case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL The application provided invalid parameter data.If you get this error even once, it means that your code caused the device removed condition and must be debugged.";
  case S_OK: return "S_OK Returned when a graphics device was enabled, disabled, or reset without invalidating the current graphics device.For example, this error code can be returned if an app is using Windows Advanced Rasterization Platform( WARP ) and a hardware adapter becomes available.";
      TAC_ASSERT_INVALID_DEFAULT_CASE( hr );
  }
  //InvalidCodePath;
  return "";
}

int registerDX12 = []()
{
  static struct DirectX12RendererFactory : public RendererFactory
  {
    DirectX12RendererFactory()
    {
      mRendererName = RendererNameDirectX12;
    }
    void CreateRenderer() override
    {
      new RendererDX12;
    }
  } factory;
  RendererRegistry::Instance().mFactories.push_back( &factory );
  return 0;
}( );
}
*/
