#pragma once

#include "common/tacRenderer.h"
#include "common/tacShell.h"
#include "tacDXGI.h"

#include <d3d12.h>

// ComPtr
#include <wrl.h> 
#include <comdef.h> // temp
using Microsoft::WRL::ComPtr;

struct TacString;

struct TacTextureDX12 : public TacTexture
{
  ComPtr< ID3D12Resource > mResource;
  D3D12_CPU_DESCRIPTOR_HANDLE mCpuDescriptorHandle = {};
  DXGI_FORMAT mDxgiFormat = DXGI_FORMAT_UNKNOWN;
  D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;
};

struct TacDX12Window : public TacRendererWindowData
{
  // Equivalent to bgfx's RendererContextD3D12::submit ( not bgfx::submit )
  //                      ^ used to render a frame           ^ used to add a draw call
  void Submit( TacErrors& errors ) override;

  void GetCurrentBackbufferTexture( TacTexture** texture ) override;
  void DebugDoubleCheckBackbufferIndex();

  // Indexes into backbufferColor, for double/triple/... buffering
  int mBackbufferIndex = 0;
  ComPtr< IDXGISwapChain > mSwapChain;
  TacVector< TacTextureDX12* > mBackbufferColors;
};

struct TacDepthBufferDX12 : public TacDepthBuffer
{
  ComPtr< ID3D12Resource > mBackbufferDepthStencil;
  D3D12_CPU_DESCRIPTOR_HANDLE mBackbufferDepthCpuDescriptorHandle = {};
  DXGI_FORMAT mDxgiFormat = DXGI_FORMAT_UNKNOWN;
};

struct TacPSOStuff
{
  TacShader* mShader = nullptr;
  TacBlendState* mBlendState = nullptr;
  TacRasterizerState* mRasterizerState = nullptr;
  TacDepthState* mDepthState = nullptr;
  TacVertexFormat* mVertexFormat = nullptr;
  TacTexture* mFramebufferTexture = nullptr;
  TacDepthBuffer* mFramebufferDepth = nullptr;
  //bool operator =( const TacPSOStuff& rhs )
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

struct TacConstantBufferDX12 : public TacCBuffer
{
  D3D12_GPU_DESCRIPTOR_HANDLE mCBufferDestDescriptorGPU = {};
  D3D12_CPU_DESCRIPTOR_HANDLE mCBufferDestDescriptorCPU = {};
  void SendUniforms( void* bytes ) override;
  void* mMappedData = nullptr;
};


struct TacRendererDX12 : public TacRenderer
{
  void Init( TacErrors& errors ) override;
  void CreateWindowContext( TacDesktopWindow* desktopWindow, TacErrors& errors ) override;
  void AddVertexBuffer( TacVertexBuffer** vertexBuffer, TacVertexBufferData vertexBufferData, TacErrors& errors ) override;
  void AddIndexBuffer( TacIndexBuffer** indexBuffer, TacIndexBufferData indexBufferData, TacErrors& errors ) override;
  void AddConstantBuffer( TacCBuffer** outputCbuffer, TacCBufferData cBufferData, TacErrors& errors ) override;
  void AddShader( TacShader** shader, TacShaderData shaderData, TacErrors& errors ) override;
  void Render( TacErrors& errors ) override;

  void ResourceBarrier( ID3D12Resource* resource, D3D12_RESOURCE_STATES& oldState, const D3D12_RESOURCE_STATES& newState );
  void GetPSOStuff( const TacDrawCall2& drawCall2, ID3D12PipelineState **pppipelineState, TacErrors& errors );
  TacString GetDeviceRemovedReason();
  TacString GetDeviceRemovedReason( HRESULT hr );
  void FinishRendering( TacErrors& errors );

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
  TacVector< TacDX12Window* > mWindows;
  TacVector< TacPSOStuff > mPSOStuff;

  TacDXGI mDXGI;

  SIZE_T scratchIncrement = 0;
};
