// This file implements the rendering backend using directx11

#pragma once

#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_renderer_backend.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/tac_hash.h"
#include "src/common/containers/tac_optional.h"
#include "src/common/string/tac_string_identifier.h"
#include "src/shell/windows/tacwinlib/tac_win32.h"
#include "src/shell/windows/tacwinlib/renderer/tac_dxgi.h"

#include <d3d11_1.h>
#include <d3d11_3.h> // ID3D11Device3, ID3D11RasterizerState2

//#include <map>
//#include <set>
//#include <thread>

namespace Tac
{
  namespace Render
  {
    struct ConstantBuffer
    {
      ID3D11Buffer* mBuffer = nullptr;
      //int           mShaderRegister = 0;
      String        mName;
      //StringID      mNameID;

    };

    struct Program
    {
      ConstantBuffers            mConstantBuffers;
      ID3D11VertexShader*        mVertexShader = nullptr;
      ID3D11GeometryShader*      mGeometryShader = nullptr;
      ID3D11PixelShader*         mPixelShader = nullptr;
      ID3DBlob*                  mInputSig = nullptr;
    };

    struct Texture
    {
      //ID3D11Resource*            GetResource();
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
      // Render-to-texture framebuffers do no.

      FLOAT                      mClearColorRGBA[ 4 ] = { 0, 0, 0, 1 };
      bool                       mClearEachFrame = true;

      int                        mBufferCount = 0;
      IDXGISwapChain*            mSwapChain = nullptr;
      ID3D11DepthStencilView*    mDepthStencilView = nullptr;
      ID3D11RenderTargetView*    mRenderTargetView = nullptr;
      ID3D11Texture2D*           mDepthTexture = nullptr;
      HWND                       mHwnd = nullptr;
      //StackFrame                 mCreationStackFrame;
      String                     mDebugName;
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
    };

    struct BoundSRVs
    {
      BoundSRVs() = default;
      BoundSRVs( const DrawCall* );
      void Clear();
      bool operator ==( const BoundSRVs& );
      ID3D11ShaderResourceView*  mBoundShaderResourceViews[ D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT ] = {};
      int                        mBoundShaderResourceViewCount = 0;
    };

    struct RendererDirectX11 : public Renderer
    {
      ~RendererDirectX11();

      // Virtual functions

      void Init( Errors& ) override;
      void RenderBegin( const Frame*, Errors& ) override;
      void RenderDrawCall( const Frame*, const DrawCall*, Errors& ) override;
      void RenderEnd( const Frame*, Errors& ) override;
      void SwapBuffers() override;
      void GetPerspectiveProjectionAB( float f,
                                       float n,
                                       float& a,
                                       float& b ) override;
      void AddBlendState( CommandDataCreateBlendState*, Errors& ) override;
      void AddConstantBuffer( CommandDataCreateConstantBuffer*, Errors& ) override;
      void AddDepthState( CommandDataCreateDepthState*, Errors& ) override;
      void AddFramebuffer( CommandDataCreateFramebuffer*, Errors& ) override;
      void AddIndexBuffer( CommandDataCreateIndexBuffer*, Errors& ) override;
      void AddRasterizerState( CommandDataCreateRasterizerState*, Errors& ) override;
      void AddSamplerState( CommandDataCreateSamplerState*, Errors& ) override;
      void AddShader( CommandDataCreateShader*, Errors& ) override;
      void AddTexture( CommandDataCreateTexture*, Errors& ) override;
      void AddMagicBuffer( CommandDataCreateMagicBuffer*, Errors& ) override;
      void AddVertexBuffer( CommandDataCreateVertexBuffer*, Errors& ) override;
      void AddVertexFormat( CommandDataCreateVertexFormat*, Errors& ) override;
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
      void ResizeFramebuffer( CommandDataResizeFramebuffer*, Errors& ) override;
      void SetRenderObjectDebugName( CommandDataSetRenderObjectDebugName*, Errors& ) override;
      void UpdateIndexBuffer( CommandDataUpdateIndexBuffer*, Errors& ) override;
      void UpdateTextureRegion( CommandDataUpdateTextureRegion*, Errors& ) override;
      void UpdateVertexBuffer( CommandDataUpdateVertexBuffer*, Errors& ) override;
      void UpdateConstantBuffer( CommandDataUpdateConstantBuffer*, Errors& ) override;

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

      String GetShaderPath( StringView ) override;

      // Non-virtual functions

      //void LoadShaderInternal( ShaderDX11LoadData* loadData,
      //                         String name,
      //                         String str,
      //                         Errors& errors );
      void        SetDebugName( ID3D11DeviceChild*, StringView );
      StringView  GetDebugName( ID3D11DeviceChild* );
      void        SetDebugName( IDXGIObject* , StringView );
      void        UpdateBuffer( ID3D11Buffer*, const void* bytes, int byteCount, Errors& );

      ID3D11InfoQueue*           mInfoQueueDEBUG = nullptr;
      ID3DUserDefinedAnnotation* mUserAnnotationDEBUG = nullptr;
      ID3D11Device*              mDevice = nullptr;
      ID3D11Device3*             mDevice3 = nullptr;
      ID3D11DeviceContext*       mDeviceContext = nullptr;
      //DXGI                       mDxgi;
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
      Program                    mPrograms[ kMaxPrograms ] = {};

      //                         Currently bound render variables
      PrimitiveTopology          mBoundPrimitiveTopology = PrimitiveTopology::Unknown;
      ID3D11Buffer*              mBoundConstantBuffers[ D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ] = {};
      int                        mBoundConstantBufferCount = 0;
      ID3D11BlendState*          mBoundBlendState = nullptr;
      ID3D11DepthStencilState*   mBoundDepthStencilState = nullptr;
      BoundSRVs                  mBoundSRVs;

      //DrawCallSamplers           mBoundSamplers;
      Optional< HashedValue >    mBoundSamplerHash;

      ViewHandle                 mBoundViewHandle;
      VertexBufferHandle         mBoundVertexBuffer;
      IndexBufferHandle          mBoundIndexBuffer;
      bool                       mBoundFramebuffersThisFrame[ kMaxFramebuffers ] = {};
      DrawCallUAVs               mBoundDrawCallUAVs;
      VertexFormatHandle         mBoundDrawCallVertexFormat;

    };


    // impl in tac_renderer_directx11_shader_preprocess.cpp
    String PreprocessShaderSource( StringView, Errors& );

    void   RegisterRendererDirectX11();
  } // namespace Render
} // namespace Tac

