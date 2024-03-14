/*
#pragma once

#include "tac-rhi/Renderer.h"
#include "tac-engine-core/shell.h"
#include "tac-std-lib/tac_core.h"
#include "tac-win32/DXGI.h"

#include <d3d12.h>

// ComPtr
#include <wrl.h> 
#include <comdef.h> // temp
using Microsoft::WRL::ComPtr;

namespace Tac
{



struct TextureDX12 : public Texture
{
  ComPtr< ID3D12Resource > mResource;
  D3D12_CPU_DESCRIPTOR_HANDLE mCpuDescriptorHandle = {};
  DXGI_FORMAT mDxgiFormat = DXGI_FORMAT_UNKNOWN;
  D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;
};

struct DX12Window : public RendererWindowData
{
  // Equivalent to bgfx's RendererContextD3D12::submit ( not bgfx::submit )
  //                      ^ used to render a frame           ^ used to add a draw call
  void Submit( Errors& errors ) override;

  void GetCurrentBackbufferTexture( Texture** texture ) override;
  void DebugDoubleCheckBackbufferIndex();

  // Indexes into backbufferColor, for double/triple/... buffering
  int mBackbufferIndex = 0;
  ComPtr< IDXGISwapChain > mSwapChain;
  Vector< TextureDX12* > mBackbufferColors;
};

struct DepthBufferDX12 : public DepthBuffer
{
  ComPtr< ID3D12Resource > mBackbufferDepthStencil;
  D3D12_CPU_DESCRIPTOR_HANDLE mBackbufferDepthCpuDescriptorHandle = {};
  DXGI_FORMAT mDxgiFormat = DXGI_FORMAT_UNKNOWN;
};

struct PSOStuff
{
  Shader* mShader = nullptr;
  BlendState* mBlendState = nullptr;
  RasterizerState* mRasterizerState = nullptr;
  DepthState* mDepthState = nullptr;
  VertexFormat* mVertexFormat = nullptr;
  Texture* mFramebufferTexture = nullptr;
  DepthBuffer* mFramebufferDepth = nullptr;
  //bool operator =( const PSOStuff& rhs )
  //{
  //  return
  //    mShader == rhs.mShader &&
  //    mBlendState == rhs.mBlendState &&
  //    mRasterizerState == rhs.mRasterizerState &&
  //    mDepthState == rhs.mDepthState &&
  //    mVertexFormat == rhs.mVertexFormat &&
  //    mFramebufferTexture == rhs.mFramebufferTexture &&
  //    mFramebufferDepth == rhs.mFramebufferDepth;
  //}

  ComPtr< ID3D12PipelineState > mPipelineState;
};

struct ConstantBufferDX12 : public CBuffer
{
  D3D12_GPU_DESCRIPTOR_HANDLE mCBufferDestDescriptorGPU = {};
  D3D12_CPU_DESCRIPTOR_HANDLE mCBufferDestDescriptorCPU = {};
  void SendUniforms( void* bytes ) override;
  void* mMappedData = nullptr;
};

struct RendererDX12 : public Renderer
{
  static RendererDX12* Instance;
  RendererDX12();
  void Init( Errors& errors ) override;
  void CreateWindowContext( DesktopWindow* desktopWindow, Errors& errors ) override;
  void AddVertexBuffer( VertexBuffer** vertexBuffer, const VertexBufferData& vertexBufferData, Errors& errors ) override;
  void AddIndexBuffer( IndexBuffer** indexBuffer, const IndexBufferData& indexBufferData, Errors& errors ) override;
  void AddConstantBuffer( CBuffer** outputCbuffer, const CBufferData& cBufferData, Errors& errors ) override;
  void AddShader( Shader** shader, const ShaderData& shaderData, Errors& errors ) override;
  void Render( Errors& errors ) override;

  void ResourceBarrier( ID3D12Resource* resource, D3D12_RESOURCE_STATES& oldState, const D3D12_RESOURCE_STATES& newState );
  void GetPSOStuff( const DrawCall2& drawCall2, ID3D12PipelineState **pppipelineState, Errors& errors );
  String GetDeviceRemovedReason();
  String GetDeviceRemovedReason( HRESULT hr );
  void FinishRendering( Errors& errors );

  ComPtr< ID3D12CommandQueue > mCommandQueue;
  ComPtr< ID3D12DescriptorHeap > mDSVDescriptorHeap;
  ComPtr< ID3D12DescriptorHeap > mRTVDescriptorHeap;
  ComPtr< ID3D12DescriptorHeap > mScratchDescriptorHeap;
  ComPtr< ID3D12DescriptorHeap > mScratchDescriptorHeapCopyDest;
  ComPtr< ID3D12Debug > mDebugController;
  ComPtr< ID3D12InfoQueue > mInfoQueue;
  ComPtr< ID3D12Device > mDevice;
  ComPtr< ID3D12CommandAllocator > mCommandAllocator;
  ComPtr< ID3D12GraphicsCommandList > mCommandList;
  ComPtr< ID3D12Fence > mFence;
  UINT64 mFenceValue = 0;

  ComPtr< ID3D12Resource > mCBufferUpload;
  Vector< DX12Window* > mWindows;
  Vector< PSOStuff > mPSOStuff;

  DXGI mDXGI;

  SIZE_T scratchIncrement = 0;
};
}
*/
