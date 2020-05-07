#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  namespace Render
  {
    enum class CommandType
    {
      CreateBlendState,
      CreateConstantBuffer,
      CreateDepthState,
      CreateFramebuffer,
      CreateIndexBuffer,
      CreateRasterizerState,
      CreateSamplerState,
      CreateShader,
      CreateTexture,
      CreateVertexBuffer,
      CreateVertexFormat,
      DestroyBlendState,
      DestroyConstantBuffer,
      DestroyDepthState,
      DestroyFramebuffer,
      DestroyIndexBuffer,
      DestroyRasterizerState,
      DestroySamplerState,
      DestroyShader,
      DestroyTexture,
      DestroyVertexBuffer,
      DestroyVertexFormat,
      UpdateIndexBuffer,
      UpdateTextureRegion,
      UpdateVertexBuffer,
      //UpdateConstantBuffer,
    };

    struct UpdateConstantBuffers
    {
      static const int kCapacity = 2;
      struct UpdateConstantBuffer
      {
        Render::CommandDataUpdateBuffer mData;
        ConstantBufferHandle mConstantBufferHandle;
      } mUpdateConstantBufferDatas[ kCapacity ];
      int mUpdateConstantBufferDataCount;
      void Push( ConstantBufferHandle, Render::CommandDataUpdateBuffer );
    };

    struct DrawCall3
    {
      VertexBufferHandle mVertexBufferHandle;
      IndexBufferHandle mIndexBufferHandle;
      BlendStateHandle mBlendStateHandle;
      RasterizerStateHandle mRasterizerStateHandle;
      SamplerStateHandle mSamplerStateHandle;
      DepthStateHandle mDepthStateHandle;
      VertexFormatHandle mVertexFormatHandle;
      UpdateConstantBuffers mUpdateConstantBuffers;
      ShaderHandle mShaderHandle;
      ViewId mViewId = InvalidViewId;
      int mStartIndex = 0;
      int mStartVertex = 0;
      int mIndexCount = 0;
      int mVertexCount = 0;
    };

    struct CommandBuffer
    {
      void Push( CommandType );
      void Push( const void* bytes, int byteCount );
      void PushCommandEnd();


      Vector<char> mBuffer;
    };

    const int kDrawCallCapacity = 1000;

    struct UniformBuffer
    {
      static const int kByteCapacity = 256 * 1024;
      char mBytes[ kByteCapacity ] = {};
      int mByteCount = 0;
    };

    struct View
    {
      FramebufferHandle mFrameBufferHandle;
      Viewport mViewport;
      ScissorRect mScissorRect;
    };

    struct Frame
    {
      // can add a mutex here so multiple threads can add draw calls at once

      CommandBuffer mCommandBuffer;

      DrawCall3 mDrawCalls[ kDrawCallCapacity ];
      int mDrawCallCount = 0;

      UniformBuffer mUniformBuffer;
      View mViews[100];
    };

    const int kMaxTextures = 1000;
    const int kMaxVertexBuffers = 1000;
    const int kMaxIndexBuffers = 1000;
    const int kMaxFramebuffers = 100;
    const int kMaxRasterizerStates = 100;
    const int kMaxSamplerStates = 100;
    const int kMaxDepthStencilStates = 100;
    const int kMaxInputLayouts = 100;
    const int kMaxBlendStates = 100;
    const int kMaxConstantBuffers = 100;
    const int kMaxPrograms = 100;

    bool IsSubmitAllocated( const void* data );

    void DebugPrintSubmitAllocInfo();
  }
}