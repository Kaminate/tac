// This file implements the rendering backend using directx11

#pragma once
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/tacShell.h"
#include "src/shell/windows/tacWin32.h"
#include "src/shell/windows/tacDXGI.h"

#include <d3d11_1.h>

#include <map>
#include <set>
#include <thread>

namespace Tac
{


  struct CBufferDX11;
  struct RendererDirectX11;


  struct ConstantFinder
  {
    CBufferDX11* mCBuffer;
    int mConstantIndex;
  };

  struct Sampler
  {
    String mName; // not debug name, dude idk what the fuck this is
    ShaderType mShaderType = ShaderType::Count;
    int mSamplerIndex = 0;
  };

  // Resources

  //struct CBufferDX11 : public CBuffer
  //{
  //  ~CBufferDX11();
  //  void SendUniforms( void* bytes ) override;

  //  ID3D11Buffer* mDXObj = nullptr;
  //};

  //struct TextureDX11 : public Texture
  //{
  //  TextureDX11();
  //  ~TextureDX11();
  //  void Clear() override;
  //  void* GetImguiTextureID() override;

  //  ID3D11Resource* mDXObj = nullptr;
  //  ID3D11ShaderResourceView* mSrv = nullptr;
  //  ID3D11RenderTargetView* mRTV = nullptr;
  //};

  //struct DepthBufferDX11 : public DepthBuffer
  //{
  //  ~DepthBufferDX11();
  //  void Init( Errors& errors );
  //  void Clear() override;

  //  ID3D11Resource* mDXObj = nullptr;
  //  ID3D11DepthStencilView* mDSV = nullptr;
  //};

  struct ShaderDX11LoadData
  {
    void Release();
    ID3D11VertexShader* mVertexShader = nullptr;
    ID3D11PixelShader* mPixelShader = nullptr;
    ID3DBlob* mInputSig = nullptr;
  };

  //struct ShaderDX11 : public Shader
  //{
  //  ~ShaderDX11();

  //  ShaderDX11LoadData mLoadData;

  //  Vector< Sampler* > mTextures;
  //  Vector< Sampler* > mSamplers;
  //  Sampler* FindTexture( const String& name );
  //  Sampler* FindSampler( const String& name );
  //  Sampler* Find( Vector< Sampler* >& samplers, const String& name );

  //};

  //struct VertexBufferDX11 : public VertexBuffer
  //{
  //  ~VertexBufferDX11();
  //  ID3D11Buffer* mDXObj = nullptr;
  //  void Overwrite( void* data, int byteCount, Errors& errors ) override;
  //};

  //struct IndexBufferDX11 : public IndexBuffer
  //{
  //  ~IndexBufferDX11();
  //  ID3D11Buffer* mDXObj = nullptr;
  //  DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;
  //  void Overwrite( void* data, int byteCount, Errors& errors ) override;
  //};

  //struct SamplerStateDX11 : public SamplerState
  //{
  //  ~SamplerStateDX11();
  //  ID3D11SamplerState* mDXObj = nullptr;
  //};

  //struct DepthStateDX11 : public DepthState
  //{
  //  ~DepthStateDX11();
  //  ID3D11DepthStencilState* mDXObj = nullptr;
  //};

  //struct BlendStateDX11 : public BlendState
  //{
  //  ~BlendStateDX11();
  //  ID3D11BlendState* mDXObj = nullptr;
  //};

  //struct RasterizerStateDX11 : public RasterizerState
  //{
  //  ~RasterizerStateDX11();
  //  ID3D11RasterizerState* mDXObj = nullptr;
  //};

  //struct VertexFormatDX11 : public VertexFormat
  //{
  //  ~VertexFormatDX11();
  //  ID3D11InputLayout* mDXObj = nullptr;
  //};

  //struct DX11Window : public RendererWindowData
  //{
  //  ~DX11Window();

  //  void SwapBuffers( Errors& errors );
  //  void CreateRenderTarget( Errors& errors );
  //  void GetCurrentBackbufferTexture( Texture** texture ) override;
  //  void OnResize( Errors& errors ) override;
  //  IDXGISwapChain* mSwapChain = nullptr;
  //  // | I don't understand why this isn't a thing.
  //  // | Shouldn't there be one texture per buffer?
  //  // v ie: double buffering should have an array of 2 textures, right?
  //  //Vector< TextureDX11* > mBackbufferColors;
  //  //int mBufferIndex = 0;
  //  TextureDX11* mBackbufferColor = nullptr;
  //};

  struct RendererDirectX11 : public Renderer
  {
    static RendererDirectX11* Instance;
    RendererDirectX11();
    ~RendererDirectX11();

    void Init( Errors& errors ) override;
    //void Render( Errors& errors ) override;
    void Render2( const Render::Frame*, Errors& errors ) override;
    //void RenderFlush() override;
    void SwapBuffers() override;

    //void CreateWindowContext( DesktopWindow* desktopWindow, Errors& errors ) override;


    //void ClearColor( Texture* texture, v4 rgba ) override;
    //void ClearDepthStencil(
    //  DepthBuffer* depthBuffer,
    //  bool shouldClearDepth,
    //  float depth,
    //  bool shouldClearStencil,
    //  uint8_t stencil ) override;

    //void ReloadShader( Shader* shader, Errors& errors ) override;
    //void AddShader( Shader** outputShader, const ShaderData&, Errors& errors ) override;
    //void GetShaders( Vector< Shader* > & ) override;

    //void AddSamplerState( SamplerState**, const SamplerStateData&, Errors& errors ) override;
    //void AddSampler(
    //  const String& samplerName,
    //  Shader* shader,
    //  ShaderType shaderType,
    //  int samplerIndex ) override;
    //void SetSamplerState(
    //  const String& samplerName,
    //  SamplerState* samplerState ) override;

    //void AddTextureResource( Tac::Texture**, const TextureData&, Errors& errors ) override;
    //void AddTextureResourceCube( Tac::Texture** texture, const TextureData& textureData, void** sixCubeDatas, Errors& errors ) override;
    //void AddTexture(
    //  const String& textureName,
    //  Shader* shader,
    //  ShaderType shaderType,
    //  int samplerIndex ) override;
    //void SetTexture(
    //  const String& textureName,
    //  Tac::Texture* texture ) override;

    //void AddDepthBuffer( DepthBuffer** outputDepthBuffer, const DepthBufferData&, Errors& errors ) override;

    //void AddConstantBuffer( CBuffer** outputCbuffer, const CBufferData&, Errors& errors ) override;

    //void AddBlendState( BlendState**, const BlendStateData&, Errors& errors ) override;

    //// rasterizer state

    //void AddRasterizerState( RasterizerState**, const RasterizerStateData&, Errors& errors ) override;

    //void AddDepthState( DepthState**, const DepthStateData&, Errors& errors ) override;

    //void AddVertexFormat( VertexFormat**, const VertexFormatData&, Errors& errors ) override;

    //void DebugBegin( const String& section ) override;
    //void DebugMark( const String& remark ) override;
    //void DebugEnd() override;

    //void DrawNonIndexed( int vertCount ) override;
    //void DrawIndexed( int elementCount, int idxOffset, int vtxOffset ) override;

    //void Apply() override;

    //void SetViewport(
    //  float xRelBotLeftCorner,
    //  float yRelBotLeftCorner,
    //  float wIncreasingRight,
    //  float hIncreasingUp ) override;

    //void SetPrimitiveTopology( Primitive primitive ) override;

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
      ShaderDX11LoadData* loadData,
      String name,
      String str,
      Errors& errors );

    //void CreateTexture(
    //  const Image& image,
    //  void* optionalInitialBytes,
    //  ID3D11Resource** texture,
    //  Access access,
    //  std::set< CPUAccess > cpuAccess,
    //  std::set< Binding > binding,
    //  const String& debugName,
    //  Errors& errors );

    //void CopyTextureRegion(
    //  Texture* dst,
    //  Image src,
    //  int x,
    //  int y,
    //  Errors& errors ) override;

    //void CreateShaderResourceView(
    //  ID3D11Resource* resource,
    //  ID3D11ShaderResourceView** srv,
    //  const String& debugName,
    //  Errors& errors );

    String AppendInfoQueueMessage( HRESULT hr );
    void SetDebugName( ID3D11DeviceChild* directXObject,
                       StringView name );


    ID3D11InfoQueue* mInfoQueueDEBUG = nullptr;
    ID3DUserDefinedAnnotation* mUserAnnotationDEBUG = nullptr;

    //std::map< ShaderType, Vector< TextureDX11* > >mCurrentTextures;
    //std::set< ShaderType > mCurrentTexturesDirty;
    //std::map< ShaderType, Vector< SamplerStateDX11* > >mCurrentSamplers;
    //std::set< ShaderType > mCurrentSamplersDirty;
    //std::set< CBufferDX11* > mCbuffers;
    //std::set< ShaderDX11* > mShaders;

    ID3D11Device* mDevice = nullptr;
    ID3D11DeviceContext* mDeviceContext = nullptr;
    DXGI mDxgi;

    //Vector< DX11Window* > mWindows;

    // do you think these should live in the base renderer class?
    //ShaderDX11* mCurrentlyBoundShader = nullptr;
    //VertexBufferDX11* mCurrentlyBoundVertexBuffer = nullptr;
    //IndexBufferDX11* mCurrentlyBoundIndexBuffer = nullptr;
    //BlendStateDX11* mCurrentlyBoundBlendState = nullptr;
    //RasterizerStateDX11* mCurrentlyBoundRasterizerState = nullptr;
    //DepthStateDX11* mCurrentlyBoundDepthState = nullptr;
    //VertexFormatDX11* mCurrentlyBoundVertexFormat = nullptr;
    //Vector< const Texture* > mCurrentlyBoundTextures;
    //SamplerStateDX11* mCurrentlyBoundSamplerState = nullptr;
    //RenderView* mCurrentlyBoundView = nullptr;
    //Vector< RenderView* > mFrameBoundRenderViews;


    // --- Resources begin ---

    struct Texture
    {
      ID3D11Texture2D* mTexture2D = {};
      ID3D11RenderTargetView* mTextureRTV = {};
      ID3D11ShaderResourceView* mTextureSRV = {};
    } mTextures[ Render::kMaxTextures ] = {};
    struct VertexBuffer
    {
      ID3D11Buffer* mBuffer;
      UINT mStride;
      //Format mFormat; bad. format is in the vertexformat
    } mVertexBuffers[ Render::kMaxVertexBuffers ] = {};
    struct IndexBuffer
    {
      ID3D11Buffer* mBuffer;
      Format mFormat;
    } mIndexBuffers[ Render::kMaxIndexBuffers ] = {};
    struct Framebuffer
    {
      int mBufferCount = 0;
      IDXGISwapChain* mSwapChain = nullptr;
      ID3D11DepthStencilView* mDepthStencilView = nullptr;
      ID3D11RenderTargetView* mRenderTargetView = nullptr;
      ID3D11Texture2D* mDepthTexture = nullptr;
      HWND mHwnd = nullptr;
    } mFramebuffers[ Render::kMaxFramebuffers ] = {};
    Render::FramebufferHandle mWindows[ Render::kMaxFramebuffers ];
    int mWindowCount = 0;
    ID3D11RasterizerState* mRasterizerStates[ Render::kMaxRasterizerStates ] = {};
    ID3D11SamplerState* mSamplerStates[ Render::kMaxSamplerStates ] = {};
    ID3D11DepthStencilState* mDepthStencilStates[ Render::kMaxDepthStencilStates ] = {};
    ID3D11InputLayout* mInputLayouts[ Render::kMaxInputLayouts ] = {};
    ID3D11BlendState* mBlendStates[ Render::kMaxBlendStates ] = {};
    struct ConstantBuffer
    {
      ID3D11Buffer* mBuffer = nullptr;
      int mShaderRegister = 0;
    } mConstantBuffers[ Render::kMaxConstantBuffers ] = {};
    struct Program
    {
      ID3D11VertexShader* mVertexShader = nullptr;
      ID3D11PixelShader* mPixelShader = nullptr;
      ID3DBlob* mInputSig = nullptr;
    } mPrograms[ Render::kMaxPrograms ] = {};


    // this should all be virtual
    void AddBlendState( Render::CommandDataCreateBlendState*, Errors& ) override;
    void AddConstantBuffer( Render::CommandDataCreateConstantBuffer*, Errors& ) override;
    void AddDepthState( Render::CommandDataCreateDepthState*, Errors& ) override;
    void AddFramebuffer( Render::CommandDataCreateFramebuffer*, Errors& ) override;
    void AddIndexBuffer( Render::CommandDataCreateIndexBuffer*, Errors& ) override;
    void AddRasterizerState( Render::CommandDataCreateRasterizerState*, Errors& ) override;
    void AddSamplerState( Render::CommandDataCreateSamplerState*, Errors& ) override;
    void AddShader( Render::CommandDataCreateShader*, Errors& ) override;
    void AddTexture( Render::CommandDataCreateTexture*, Errors& ) override;
    void AddVertexBuffer( Render::CommandDataCreateVertexBuffer*, Errors& ) override;
    void AddVertexFormat( Render::CommandDataCreateVertexFormat*, Errors& ) override;
    void RemoveBlendState( Render::BlendStateHandle, Errors& ) override;
    void RemoveConstantBuffer( Render::ConstantBufferHandle, Errors& ) override;
    void RemoveDepthState( Render::DepthStateHandle, Errors& ) override;
    void RemoveFramebuffer( Render::FramebufferHandle, Errors& ) override;
    void RemoveIndexBuffer( Render::IndexBufferHandle, Errors& ) override;
    void RemoveRasterizerState( Render::RasterizerStateHandle, Errors& ) override;
    void RemoveSamplerState( Render::SamplerStateHandle, Errors& ) override;
    void RemoveShader( Render::ShaderHandle, Errors& ) override;
    void RemoveTexture( Render::TextureHandle, Errors& ) override;
    void RemoveVertexBuffer( Render::VertexBufferHandle, Errors& ) override;
    void RemoveVertexFormat( Render::VertexFormatHandle, Errors& ) override;
    void ResizeFramebuffer( Render::CommandDataResizeFramebuffer*, Errors& ) override;
    void UpdateIndexBuffer( Render::CommandDataUpdateIndexBuffer*, Errors& ) override;
    void UpdateTextureRegion( Render::CommandDataUpdateTextureRegion*, Errors& ) override;
    void UpdateVertexBuffer( Render::CommandDataUpdateVertexBuffer*, Errors& ) override;




    // frame buffers?

    // --- Resources end ---

    void UpdateBuffer( ID3D11Buffer* buffer, const void* bytes, int byteCount, Errors& errors );

  };

}
