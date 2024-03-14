// This file implements the rendering backend using directx11

#pragma once

#define TAC_USING_STD_MODULES() 1

#if !TAC_USING_STD_MODULES()
// There's some weird shit going on with c++20 std modules (import std),
// and vcruntime_new align_val_t due to the inclusion of <wrl/client.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
using Microsoft::WRL::ComPtr;
#endif

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-rhi/renderer/tac_render_frame.h"
//#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/string/tac_string_identifier.h"
#include "tac-win32/tac_win32.h"
#include "tac-win32/renderer/dxgi/tac_dxgi.h"
#include "tac-win32/tac_win32_com_ptr.h"

#include <d3d11_3.h> // ID3D11Device3, ID3D11RasterizerState2

#define TAC_DX11_CALL( call )                                                                      \
{                                                                                                  \
  const HRESULT hr = call;                                                                         \
  const bool failed = FAILED( hr );                                                                \
  if( failed )                                                                                     \
  {                                                                                                \
    TAC_CALL( Tac::Render::DX11CallAux( #call, hr, errors ) );                                     \
  }                                                                                                \
}

#define TAC_DX11_CALL_RETURN( retval, call )                                                       \
{                                                                                                  \
  const HRESULT hr = call;                                                                         \
  const bool failed = FAILED( hr );                                                                \
  if( failed )                                                                                     \
  {                                                                                                \
    TAC_CALL_RET( retval, Tac::Render::DX11CallAux( #call, hr, errors ) );                         \
  }                                                                                                \
}

namespace Tac::Render
{
  void DX11CallAux( const char*, const HRESULT, Errors& );

  struct CommunistPtr;
#define TAC_RELEASE_IUNKNOWN( p ) { if( p ){ p->Release(); p = nullptr; } }

  struct ConstantBuffer
  {
    //ComPtr<ID3D11Buffer> mBuffer;
    ID3D11Buffer* mBuffer = nullptr;

    //            mName is used to
    //            1) Insure that multiple cbuffers aren't created with the same name
    //            2) Generate Program::mConstantBuffers while processing shader source
    String        mName;

    void clear();
    
  };

  struct DX11Program
  {
    ConstantBuffers            mConstantBuffers;

    ID3D11VertexShader*        mVertexShader = nullptr;
    ID3D11GeometryShader*      mGeometryShader = nullptr;
    ID3D11PixelShader*         mPixelShader = nullptr;

    ID3DBlob*                  mInputSig = nullptr;
  };

  struct Texture
  {
    ID3D11DepthStencilView*    mTextureDSV = {};
    ID3D11Texture2D*           mTexture2D = {};
    ID3D11Texture3D*           mTexture3D = {};
    ID3D11RenderTargetView*    mTextureRTV = {};
    ID3D11ShaderResourceView*  mTextureSRV = {};
    ID3D11UnorderedAccessView* mTextureUAV = {};
  };

  struct Framebuffer
  {
    // Window framebuffers own their depth textures, rtv, dsv.
    //
    // Render-to-texture framebuffers do not.

    FLOAT                      mClearColorRGBA[ 4 ] = { 0, 0, 0, 1 };
    bool                       mClearEachFrame = true;

    int                        mBufferCount = 0;
    PCom<IDXGISwapChain4>      mSwapChain;
    ID3D11DepthStencilView*    mDepthStencilView = nullptr;
    ID3D11RenderTargetView*    mRenderTargetView = nullptr;
    ID3D11Texture2D*           mDepthTexture = nullptr;
    HWND                       mHwnd = nullptr;
    String                     mDebugName; // Used when resizing the framebuffer
  };

  struct VertexBuffer
  {
    ID3D11Buffer* mBuffer = nullptr;
    UINT          mStride = 0;
    //Format mFormat; bad. format is in the vertexformat
  };

  struct IndexBuffer
  {
    ID3D11Buffer* mBuffer = nullptr;
    Format        mFormat;
  };

  struct MagicBuffer
  {
    ID3D11Buffer*              mBuffer = nullptr;
    ID3D11UnorderedAccessView* mUAV = nullptr;
    ID3D11ShaderResourceView*  mSRV = nullptr;
    void SetDebugName( const StringView& );
    void clear();
  };

  using BoundSrvSlots = Array<
    ID3D11ShaderResourceView*,
    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT >;

  struct BoundSRVs
  {
    static BoundSRVs DrawCallSRVs( const DrawCall* );

    //            Slots are allowed to be empty (ie, not memory contiguous)
    BoundSrvSlots mBoundShaderResourceViews;

    int           mMaxUsedIndex = -1;
    int           mBoundTextureCount = 0;
    HashValue     mHash = 0;
  };

  using BoundCBufSlots = Array<
    ID3D11Buffer*,
    D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT >;

  struct BoundCBufs
  {
    static BoundCBufs ShaderCBufs( const ConstantBuffers& );

    BoundCBufSlots mBoundConstantBuffers;
    int            mMaxUsedIndex = -1;
    int            mBoundCBufCount = 0;
    HashValue      mHash = 0;
  };

  struct RendererDirectX11 : public Renderer
  {
    ~RendererDirectX11();

    // Virtual functions

    void Init( Errors& ) override;
    void RenderBegin( const Frame*, Errors& ) override;
    void RenderDrawCall( const Frame*, const DrawCall*, Errors& ) override;
    void RenderEnd( float dt, const Frame*, Errors& ) override;
    void SwapBuffers() override;
    OutProj GetPerspectiveProjectionAB(InProj) override;
    void AddBlendState( const CommandDataCreateBlendState*, Errors& ) override;
    void AddConstantBuffer( const CommandDataCreateConstantBuffer*, Errors& ) override;
    void AddDepthState( const CommandDataCreateDepthState*, Errors& ) override;
    void AddFramebuffer( const CommandDataCreateFramebuffer*, Errors& ) override;
    void AddFramebufferForRenderToTexture( const CommandDataCreateFramebuffer*, Errors& );
    void AddFramebufferForWindow( const CommandDataCreateFramebuffer*, Errors& );

    void AddIndexBuffer( const CommandDataCreateIndexBuffer*, Errors& ) override;
    void AddRasterizerState( const CommandDataCreateRasterizerState*, Errors& ) override;
    void AddSamplerState( const CommandDataCreateSamplerState*, Errors& ) override;
    void AddShader( const CommandDataCreateShader*, Errors& ) override;
    void AddTexture( const CommandDataCreateTexture*, Errors& ) override;
    void AddMagicBuffer( const CommandDataCreateMagicBuffer*, Errors& ) override;
    void AddVertexBuffer( const CommandDataCreateVertexBuffer*, Errors& ) override;
    void AddVertexFormat( const CommandDataCreateVertexFormat*, Errors& ) override;
    void RemoveBlendState( BlendStateHandle, Errors& ) override;
    void RemoveConstantBuffer( ConstantBufferHandle, Errors& ) override;
    void RemoveDepthState( DepthStateHandle, Errors& ) override;
    void RemoveFramebuffer( FramebufferHandle, Errors& ) override;
    void RemoveIndexBuffer( IndexBufferHandle, Errors& ) override;
    void RemoveRasterizerState( RasterizerStateHandle, Errors& ) override;
    void RemoveSamplerState( SamplerStateHandle, Errors& ) override;
    void RemoveShader( ShaderHandle, Errors& ) override;
    void RemoveTexture( TextureHandle, Errors& ) override;
    void RemoveMagicBuffer( MagicBufferHandle, Errors& ) override;
    void RemoveVertexBuffer( VertexBufferHandle, Errors& ) override;
    void RemoveVertexFormat( VertexFormatHandle, Errors& ) override;
    void ResizeFramebuffer( const CommandDataResizeFramebuffer*, Errors& ) override;
    void SetRenderObjectDebugName( const CommandDataSetRenderObjectDebugName*, Errors& ) override;
    void UpdateIndexBuffer( const CommandDataUpdateIndexBuffer*, Errors& ) override;
    void UpdateTextureRegion( const CommandDataUpdateTextureRegion*, Errors& ) override;
    void UpdateVertexBuffer( const CommandDataUpdateVertexBuffer*, Errors& ) override;
    void UpdateConstantBuffer( const CommandDataUpdateConstantBuffer*, Errors& ) override;

    // THis could use string hash server...
    void DebugGroupBegin( StringView ) override;
    void DebugMarker( StringView ) override;
    void DebugGroupEnd() override;


    // Render draw call functions

    void RenderDrawCallShader( const DrawCall* );
    void RenderDrawCallBlendState( const DrawCall* );
    void RenderDrawCallDepthState( const DrawCall* );
    void RenderDrawCallIndexBuffer( const DrawCall* );
    void RenderDrawCallVertexBuffer( const DrawCall* );
    void RenderDrawCallRasterizerState( const DrawCall* );
    void RenderDrawCallSamplerState( const DrawCall* );
    void RenderDrawCallVertexFormat( const DrawCall* );
    void RenderDrawCallViewAndUAV( const Frame*, const DrawCall* );
    void RenderDrawCallTextures( const DrawCall* );
    void RenderDrawCallPrimitiveTopology( const DrawCall* );
    void RenderDrawCallIssueDrawCommand( const DrawCall* );

    AssetPathStringView GetShaderPath( const ShaderNameStringView& ) const override;
    AssetPathStringView GetShaderDir() const override;

    // Non-virtual functions

    //void LoadShaderInternal( ShaderDX11LoadData* loadData,
    //                         String name,
    //                         String str,
    //                         Errors& errors );


    static RendererDirectX11* GetInstance();
    static ID3D11InfoQueue* GetInfoQueue();

    void        UpdateBuffer( ID3D11Buffer*, const void* bytes, int byteCount, Errors& );

    ConstantBufferHandle FindCbufferOfName( const StringView& );

    DX11Program*          FindProgram( ShaderHandle ) ;
    Framebuffer*      FindFramebuffer( FramebufferHandle );
    Texture*          FindTexture( TextureHandle );
    IndexBuffer*      FindIndexBuffer( IndexBufferHandle );
    VertexBuffer*     FindVertexBuffer( VertexBufferHandle );
    MagicBuffer*      FindMagicBuffer( MagicBufferHandle );
    ConstantBuffer*   FindConstantBuffer( ConstantBufferHandle );

    ID3D11InfoQueue*           mInfoQueueDEBUG = nullptr;
    ID3DUserDefinedAnnotation* mUserAnnotationDEBUG = nullptr;
    PCom<ID3D11Device3>        mDevice;
    PCom<ID3D11DeviceContext4> mDeviceContext;

    Texture                    mTextures[ kMaxTextures ] = {};
    MagicBuffer                mMagicBuffers[ kMaxMagicBuffers ] = {};
    VertexBuffer               mVertexBuffers[ kMaxVertexBuffers ] = {};
    IndexBuffer                mIndexBuffers[ kMaxIndexBuffers ] = {};
    Framebuffer                mFramebuffers[ kMaxFramebuffers ] = {};
    FramebufferHandle          mWindows[ kMaxFramebuffers ];
    int                        mWindowCount = 0;
    ID3D11RasterizerState*     mRasterizerStates[ kMaxRasterizerStates ] = {};
    ID3D11SamplerState*        mSamplerStates[ kMaxSamplerStates ] = {};
    ID3D11DepthStencilState*   mDepthStencilStates[ kMaxDepthStencilStates ] = {};
    ID3D11InputLayout*         mInputLayouts[ kMaxInputLayouts ] = {};
    ID3D11BlendState*          mBlendStates[ kMaxBlendStates ] = {};
    ConstantBuffer             mConstantBuffers[ kMaxConstantBuffers ] = {};
    DX11Program                mPrograms[ kMaxPrograms ] = {};


    //                         Currently bound render variables
    PrimitiveTopology          mBoundPrimitiveTopology = PrimitiveTopology::Unknown;
    ID3D11BlendState*          mBoundBlendState = nullptr;
    ID3D11DepthStencilState*   mBoundDepthStencilState = nullptr;
    BoundCBufs                 mBoundConstantBuffers;
    BoundSRVs                  mBoundSRVs;

    //DrawCallSamplers           mBoundSamplers;
    Optional< HashValue >      mBoundSamplerHash;

    ViewHandle                 mBoundViewHandle;
    VertexBufferHandle         mBoundVertexBuffer;
    IndexBufferHandle          mBoundIndexBuffer;
    bool                       mBoundFramebuffersThisFrame[ kMaxFramebuffers ] = {};
    DrawCallUAVs               mBoundDrawCallUAVs;
    VertexFormatHandle         mBoundDrawCallVertexFormat;

  };


  void   RegisterRendererDirectX11();


// -------------------------------------------------------------------------------------------------



} // namespace Tac::Render

