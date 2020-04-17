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
    };

    struct CommandBuffer
    {
      void Push( CommandType );
      void Push( const void* bytes, int byteCount );

      
      Vector<char> mBuffer;
    };

    struct DrawCall3
    {
      VertexBufferHandle mVertexBufferHandle;
      IndexBufferHandle mIndexBufferHandle;
    };

    const int kDrawCallCapacity = 1000;
    struct Frame
    {
      // can add a mutex here so multiple threads can add draw calls at once

      CommandBuffer mCommandBuffer;

      DrawCall3 mDrawCalls[ kDrawCallCapacity ];
      int mDrawCallCount = 0;
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
  }
}