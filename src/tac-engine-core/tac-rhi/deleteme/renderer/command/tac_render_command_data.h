#pragma once

#include "tac-rhi/renderer/tac_renderer.h"

namespace Tac::Render
{

  struct CommandDataResizeFramebuffer
  {
    StackFrame            mStackFrame;
    int                   mWidth = 0;
    int                   mHeight = 0;
    FramebufferHandle     mFramebufferHandle;
  };

  struct CommandDataCreateShader
  {
    StackFrame            mStackFrame;
    ShaderNameStringView  mNameStringView;
    ShaderHandle          mShaderHandle;
  };

  struct CommandDataCreateConstantBuffer
  {
    StackFrame            mStackFrame;
    ConstantBufferHandle  mConstantBufferHandle;
    int                   mByteCount = 0;
    const char*           mName;
  };

  struct CommandDataCreateMagicBuffer
  {
    StackFrame            mStackFrame;
    MagicBufferHandle     mMagicBufferHandle;
    int                   mByteCount = 0;
    const void*           mOptionalInitialBytes = nullptr;
    int                   mStride = 0;
    Binding               mBinding = Binding::None;
    Access                mAccess = Access::Default;
  };

  struct CommandDataCreateVertexBuffer
  {
    StackFrame            mStackFrame;
    VertexBufferHandle    mVertexBufferHandle;
    int                   mByteCount = 0;
    const void*           mOptionalInitialBytes = nullptr;
    int                   mStride = 0;
    Access                mAccess = Access::Default;
  };

  struct CommandDataCreateIndexBuffer
  {
    StackFrame            mStackFrame;
    IndexBufferHandle     mIndexBufferHandle;
    int                   mByteCount;
    const void*           mOptionalInitialBytes;
    Access                mAccess;
    Format                mFormat;
  };

  struct CommandDataCreateBlendState
  {
    StackFrame            mStackFrame;
    BlendStateHandle      mBlendStateHandle;
    BlendState            mBlendState;
  };

  struct CommandDataSetRenderObjectDebugName
  {
    SamplerStateHandle    mSamplerStateHandle;
    ConstantBufferHandle  mConstantBufferHandle;
    VertexFormatHandle    mVertexFormatHandle;
    BlendStateHandle      mBlendStateHandle;
    DepthStateHandle      mDepthStateHandle;
    FramebufferHandle     mFramebufferHandle;
    MagicBufferHandle     mMagicBufferHandle;
    RasterizerStateHandle mRasterizerStateHandle;
    TextureHandle         mTextureHandle;
    VertexBufferHandle    mVertexBufferHandle;
    IndexBufferHandle     mIndexBufferHandle;
    const char*           mName = nullptr;
  };

  struct CommandDataCreateVertexFormat
  {
    StackFrame            mStackFrame;
    VertexFormatHandle    mVertexFormatHandle;
    VertexDeclarations    mVertexDeclarations;
    ShaderHandle          mShaderHandle;
  };

  struct CommandDataUpdateVertexBuffer
  {
    StackFrame            mStackFrame;
    VertexBufferHandle    mVertexBufferHandle;
    const void*           mBytes = nullptr;
    int                   mByteCount = 0;
  };

  struct CommandDataUpdateIndexBuffer
  {
    StackFrame            mStackFrame;
    IndexBufferHandle     mIndexBufferHandle;
    const void*           mBytes = nullptr;
    int                   mByteCount = 0;
  };

  struct CommandDataUpdateConstantBuffer
  {
    StackFrame            mStackFrame;
    ConstantBufferHandle  mConstantBufferHandle;
    const void*           mBytes = nullptr;
    int                   mByteCount = 0;
  };

  struct CommandDataCreateFramebuffer
  {
    StackFrame            mStackFrame;
    FramebufferHandle     mFramebufferHandle;

    //                    Used when creating a framebuffer for a window
    const void*           mNativeWindowHandle = nullptr;
    int                   mWidth = 0;
    int                   mHeight = 0;
    FramebufferTextures   mFramebufferTextures;
  };

  struct CommandDataCreateDepthState
  {
    StackFrame            mStackFrame;
    DepthStateHandle      mDepthStateHandle;
    DepthState            mDepthState;
  };

  struct CommandDataCreateTexture
  {
    StackFrame            mStackFrame;
    TextureHandle         mTextureHandle;
    TexSpec               mTexSpec;
  };

  struct CommandDataUpdateTextureRegion
  {
    StackFrame            mStackFrame;
    TextureHandle         mTextureHandle;
    TexUpdate             mTexUpdate;
  };

  struct CommandDataCreateRasterizerState
  {
    StackFrame            mStackFrame;
    RasterizerStateHandle mRasterizerStateHandle;
    RasterizerState       mRasterizerState;
  };

  struct CommandDataCreateSamplerState
  {
    StackFrame            mStackFrame;
    SamplerState          mSamplerState;
    SamplerStateHandle    mSamplerStateHandle;
  };

  struct CommandDataDestroy
  {
    StackFrame            mStackFrame;
    int                   mIndex;
  };


} // namespace Tac::Render
