// This file implements the rendering backend using directx11

#pragma once
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/shell/tacShell.h"
#include "src/shell/windows/tacWin32.h"
#include "src/shell/windows/tacDXGI.h"

#include <d3d11_1.h>

#include <map>
#include <set>
#include <thread>

namespace Tac
{
  namespace Render
  {

    struct ConstantBuffer
    {
      ID3D11Buffer* mBuffer = nullptr;
      int           mShaderRegister = 0;
    };

    struct Program
    {
      ID3D11VertexShader*       mVertexShader = nullptr;
      ID3D11GeometryShader*     mGeometryShader = nullptr;
      ID3D11PixelShader*        mPixelShader = nullptr;
      ID3DBlob*                 mInputSig = nullptr;
    };

    struct Texture
    {
      ID3D11Texture2D*          mTexture2D = {};
      ID3D11Texture3D*          mTexture3D = {};
      ID3D11RenderTargetView*   mTextureRTV = {};
      ID3D11ShaderResourceView* mTextureSRV = {};
    };

    struct Framebuffer
    {
      int                       mBufferCount = 0;
      IDXGISwapChain*           mSwapChain = nullptr;
      ID3D11DepthStencilView*   mDepthStencilView = nullptr;
      ID3D11RenderTargetView*   mRenderTargetView = nullptr;
      ID3D11Texture2D*          mDepthTexture = nullptr;
      HWND                      mHwnd = nullptr;
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

    struct RendererDirectX11 : public Renderer
    {
      ~RendererDirectX11();

      // Virtual functions

      void Init( Errors& ) override;
      void RenderBegin( const Render::Frame*, Errors& ) override;
      void RenderDrawCall( const Render::Frame*, const Render::DrawCall3*, Errors& ) override;
      void RenderEnd( const Render::Frame*, Errors& ) override;
      void SwapBuffers() override;
      void GetPerspectiveProjectionAB( float f,
                                       float n,
                                       float& a,
                                       float& b ) override;
      void AddBlendState( Render::CommandDataCreateBlendState*, Errors& ) override;
      void AddConstantBuffer( Render::CommandDataCreateConstantBuffer*, Errors& ) override;
      void AddDepthState( Render::CommandDataCreateDepthState*, Errors& ) override;
      void AddFramebuffer( Render::CommandDataCreateFramebuffer*, Errors& ) override;
      void AddIndexBuffer( Render::CommandDataCreateIndexBuffer*, Errors& ) override;
      void AddRasterizerState( Render::CommandDataCreateRasterizerState*, Errors& ) override;
      void AddSamplerState( Render::CommandDataCreateSamplerState*, Errors& ) override;
      void AddShader( Render::CommandDataCreateShader*, Errors& ) override;
      void AddTexture( Render::CommandDataCreateTexture*, Errors& ) override;
      void AddMagicBuffer( Render::CommandDataCreateMagicBuffer*, Errors& ) override;
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
      void SetRenderObjectDebugName( Render::CommandDataSetRenderObjectDebugName*, Errors& ) override;
      void UpdateIndexBuffer( Render::CommandDataUpdateIndexBuffer*, Errors& ) override;
      void UpdateTextureRegion( Render::CommandDataUpdateTextureRegion*, Errors& ) override;
      void UpdateVertexBuffer( Render::CommandDataUpdateVertexBuffer*, Errors& ) override;
      void UpdateConstantBuffer( Render::CommandDataUpdateConstantBuffer*, Errors& ) override;
      void DebugGroupBegin( StringView ) override;
      void DebugMarker( StringView ) override;
      void DebugGroupEnd() override;

      // Non-virtual functions

      //void LoadShaderInternal( ShaderDX11LoadData* loadData,
      //                         String name,
      //                         String str,
      //                         Errors& errors );
      void SetDebugName( ID3D11DeviceChild* directXObject,
                         StringView name );
      void UpdateBuffer( ID3D11Buffer*, const void* bytes, int byteCount, Errors& );

      ID3D11InfoQueue*           mInfoQueueDEBUG = nullptr;
      ID3DUserDefinedAnnotation* mUserAnnotationDEBUG = nullptr;
      ID3D11Device*              mDevice = nullptr;
      ID3D11DeviceContext*       mDeviceContext = nullptr;
      //DXGI                       mDxgi;
      Texture                    mTextures[ Render::kMaxTextures ] = {};
      MagicBuffer                mMagicBuffers[ Render::kMaxMagicBuffers ] = {};
      VertexBuffer               mVertexBuffers[ Render::kMaxVertexBuffers ] = {};
      IndexBuffer                mIndexBuffers[ Render::kMaxIndexBuffers ] = {};
      Framebuffer                mFramebuffers[ Render::kMaxFramebuffers ] = {};
      Render::FramebufferHandle  mWindows[ Render::kMaxFramebuffers ];
      int                        mWindowCount = 0;
      ID3D11RasterizerState*     mRasterizerStates[ Render::kMaxRasterizerStates ] = {};
      ID3D11SamplerState*        mSamplerStates[ Render::kMaxSamplerStates ] = {};
      ID3D11DepthStencilState*   mDepthStencilStates[ Render::kMaxDepthStencilStates ] = {};
      ID3D11InputLayout*         mInputLayouts[ Render::kMaxInputLayouts ] = {};
      ID3D11BlendState*          mBlendStates[ Render::kMaxBlendStates ] = {};
      ConstantBuffer             mConstantBuffers[ Render::kMaxConstantBuffers ] = {};
      Program                    mPrograms[ Render::kMaxPrograms ] = {};
      ID3D11Buffer*              mBoundConstantBuffers[ D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ] = {};
      int                        mBoundConstantBufferCount = 0;

      //                         Currently bound render variables
      ID3D11BlendState*          mBlendState = nullptr;
      ID3D11DepthStencilState*   mDepthStencilState = nullptr;
      Render::ViewHandle         mViewHandle;
      IndexBuffer*               mIndexBuffer = nullptr;
      bool                       mFramebuffersBoundEverThisFrame[ kMaxFramebuffers ];
    };
  } // namespace Render
} // namespace Tac

