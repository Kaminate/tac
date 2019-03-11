// This file implements the rendering backend using directx11

#pragma once
#include "common/graphics/tacRenderer.h"
#include "common/tacShell.h"
#include "shell/windows/tacWindows.h"
#include "shell/windows/tacDXGI.h"

#include <d3d11_1.h>

#include <map>
#include <set>

struct TacCBufferDX11;
struct TacRendererDirectX11;


struct ConstantFinder
{
  TacCBufferDX11* mCBuffer;
  int mConstantIndex;
};

struct TacSampler
{
  TacString mName; // not debug name, dude idk what the fuck this is
  TacShaderType mShaderType = TacShaderType::Count;
  int mSamplerIndex = 0;
};

// Resources

struct TacCBufferDX11 : public TacCBuffer
{
  ~TacCBufferDX11();
  void SendUniforms( void* bytes ) override;

  ID3D11Buffer* mDXObj = nullptr;
};

struct TacTextureDX11 : public TacTexture
{
  TacTextureDX11();
  ~TacTextureDX11();
  void Clear() override;
  void* GetImguiTextureID() override;

  ID3D11Resource* mDXObj = nullptr;
  ID3D11ShaderResourceView* mSrv = nullptr;
  ID3D11RenderTargetView* mRTV = nullptr;
};

struct TacDepthBufferDX11 : public TacDepthBuffer
{
  ~TacDepthBufferDX11();
  void Init( TacErrors& errors );
  void Clear() override;

  ID3D11Resource* mDXObj = nullptr;
  ID3D11DepthStencilView* mDSV = nullptr;
};

struct TacShaderDX11 : public TacShader
{
  ~TacShaderDX11();
  ID3D11VertexShader* mVertexShader = nullptr;
  ID3D11PixelShader* mPixelShader = nullptr;
  ID3DBlob* mInputSig = nullptr;
  TacVector< TacSampler* > mTextures;
  TacVector< TacSampler* > mSamplers;
  TacSampler* FindTexture( const TacString& name );
  TacSampler* FindSampler( const TacString& name );
  TacSampler* Find( TacVector< TacSampler* >& samplers, const TacString& name );

};

struct TacVertexBufferDX11 : public TacVertexBuffer
{
  ~TacVertexBufferDX11();
  ID3D11Buffer* mDXObj = nullptr;
  void Overwrite( void* data, int byteCount, TacErrors& errors ) override;
};

struct TacIndexBufferDX11 : public TacIndexBuffer
{
  ~TacIndexBufferDX11();
  ID3D11Buffer* mDXObj = nullptr;
  DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;
  void Overwrite( void* data, int byteCount, TacErrors& errors ) override;
};

struct TacSamplerStateDX11 : public TacSamplerState
{
  ~TacSamplerStateDX11();
  ID3D11SamplerState* mDXObj = nullptr;
};

struct TacDepthStateDX11 : public TacDepthState
{
  ~TacDepthStateDX11();
  ID3D11DepthStencilState* mDXObj = nullptr;
};

struct TacBlendStateDX11 : public TacBlendState
{
  ~TacBlendStateDX11();
  ID3D11BlendState* mDXObj = nullptr;
};

struct TacRasterizerStateDX11 : public TacRasterizerState
{
  ~TacRasterizerStateDX11();
  ID3D11RasterizerState* mDXObj = nullptr;
};

struct TacVertexFormatDX11 : public TacVertexFormat
{
  ~TacVertexFormatDX11();
  ID3D11InputLayout* mDXObj = nullptr;
};

struct TacDX11Window : public TacRendererWindowData
{
  ~TacDX11Window();

  void CleanupRenderTarget();
  void SwapBuffers( TacErrors& errors );
  void CreateRenderTarget( TacErrors& errors );
  void GetCurrentBackbufferTexture( TacTexture** texture ) override;
  void OnResize( TacErrors& errors ) override;
  IDXGISwapChain* mSwapChain = nullptr;
  // | I don't understand why this isn't a thing.
  // | Shouldn't there be one texture per buffer?
  // v ie: double buffering should have an array of 2 textures, right?
  //TacVector< TacTextureDX11* > mBackbufferColors;
  //int mBufferIndex = 0;
  TacTextureDX11* mBackbufferColor = nullptr;
};

struct TacRendererDirectX11 : public TacRenderer
{
  TacRendererDirectX11();
  ~TacRendererDirectX11();

  void Init( TacErrors& errors ) override;
  void Render( TacErrors& errors ) override;
  void RenderFlush() override;

  void CreateWindowContext( TacDesktopWindow* desktopWindow, TacErrors& errors ) override;



  void AddVertexBuffer( TacVertexBuffer**, const TacVertexBufferData&, TacErrors& errors ) override;

  void AddIndexBuffer( TacIndexBuffer**, const TacIndexBufferData&, TacErrors& errors ) override;

  void ClearColor( TacTexture* texture, v4 rgba ) override;
  void ClearDepthStencil(
    TacDepthBuffer* depthBuffer,
    bool shouldClearDepth,
    float depth,
    bool shouldClearStencil,
    uint8_t stencil ) override;

  void ReloadShader( TacShader* shader, TacErrors& errors ) override;
  void AddShader( TacShader** outputShader, const TacShaderData&, TacErrors& errors ) override;
  void GetShaders( TacVector< TacShader* > & ) override;

  void AddSamplerState( TacSamplerState**, const TacSamplerStateData&, TacErrors& errors ) override;
  void AddSampler(
    const TacString& samplerName,
    TacShader* shader,
    TacShaderType shaderType,
    int samplerIndex ) override;
  void SetSamplerState(
    const TacString& samplerName,
    TacSamplerState* samplerState ) override;

  void AddTextureResource( TacTexture**, const TacTextureData&, TacErrors& errors ) override;
  void RemoveTextureResoure( TacTexture* texture );
  void AddTexture(
    const TacString& textureName,
    TacShader* shader,
    TacShaderType shaderType,
    int samplerIndex ) override;
  void SetTexture(
    const TacString& textureName,
    TacTexture* texture ) override;
  void GetTextures( TacVector< TacTexture* >& ) override;

  void AddDepthBuffer( TacDepthBuffer** outputDepthBuffer, const TacDepthBufferData&, TacErrors& errors ) override;

  void AddConstantBuffer( TacCBuffer** outputCbuffer, const TacCBufferData&, TacErrors& errors ) override;

  void AddBlendState( TacBlendState**, const TacBlendStateData&, TacErrors& errors ) override;

  // rasterizer state

  void AddRasterizerState( TacRasterizerState**, const TacRasterizerStateData&, TacErrors& errors ) override;

  void AddDepthState( TacDepthState**, const TacDepthStateData&, TacErrors& errors ) override;

  void AddVertexFormat( TacVertexFormat**, const TacVertexFormatData&, TacErrors& errors ) override;

  void DebugBegin( const TacString& section ) override;
  void DebugMark( const TacString& remark ) override;
  void DebugEnd() override;

  void DrawNonIndexed( int vertCount ) override;
  void DrawIndexed( int elementCount, int idxOffset, int vtxOffset ) override;

  void Apply() override;

  //void SetViewport(
  //  float xRelBotLeftCorner,
  //  float yRelBotLeftCorner,
  //  float wIncreasingRight,
  //  float hIncreasingUp ) override;

  void SetPrimitiveTopology( TacPrimitive primitive ) override;

  //void SetScissorRect(
  //  float x1,
  //  float y1,
  //  float x2,
  //  float y2 ) override;

  void GetPerspectiveProjectionAB(
    float f,
    float n,
    float& a,
    float& b ) override;


  void LoadShaderInternal(
    TacShaderDX11* shader,
    TacString str,
    TacErrors& errors );

  void CreateTexture(
    const TacImage& image,
    ID3D11Resource** texture,
    TacAccess access,
    std::set< TacCPUAccess > cpuAccess,
    std::set< TacBinding > binding,
    const TacString& debugName,
    TacErrors& errors );

  void CopyTextureRegion(
    TacTexture* dst,
    TacImage src,
    int x,
    int y,
    TacErrors& errors ) override;

  void CreateShaderResourceView(
    ID3D11Resource* resource,
    ID3D11ShaderResourceView** srv,
    const TacString& debugName,
    TacErrors& errors );

  TacString AppendInfoQueueMessage( HRESULT hr );
  void SetDebugName(
    ID3D11DeviceChild* directXObject,
    const TacString& name );

  ID3D11InfoQueue* mInfoQueueDEBUG = nullptr;
  ID3DUserDefinedAnnotation* mUserAnnotationDEBUG = nullptr;
  std::map< TacShaderType, TacVector< TacTextureDX11* > >mCurrentTextures;
  std::set< TacShaderType > mCurrentTexturesDirty;
  std::set< TacTextureDX11* > mTextures;
  std::map< TacShaderType, TacVector< TacSamplerStateDX11* > >mCurrentSamplers;
  std::set< TacShaderType > mCurrentSamplersDirty;
  std::set< TacCBufferDX11* > mCbuffers;
  std::set< TacShaderDX11* > mShaders;
  ID3D11Device* mDevice = nullptr;
  ID3D11DeviceContext* mDeviceContext = nullptr;
  TacDXGI mDxgi;
  TacVector< TacDX11Window* > mWindows;

  TacShaderDX11* mCurrentlyBoundShader = nullptr;
  TacVertexBufferDX11* mCurrentlyBoundVertexBuffer = nullptr;
  TacIndexBufferDX11* mCurrentlyBoundIndexBuffer = nullptr;
  TacBlendStateDX11* mCurrentlyBoundBlendState = nullptr;
  TacRasterizerStateDX11* mCurrentlyBoundRasterizerState = nullptr;
  TacDepthStateDX11* mCurrentlyBoundDepthState = nullptr;
  TacVertexFormatDX11* mCurrentlyBoundVertexFormat = nullptr;
  TacTextureDX11* mCurrentlyBoundTexture = nullptr;
  TacSamplerStateDX11* mCurrentlyBoundSamplerState = nullptr;
  TacRenderView* mCurrentlyBoundView = nullptr;
  TacVector< TacRenderView* > mFrameBoundRenderViews;

};

