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

    struct Frame
    {
      // can add a mutex here so multiple threads can add draw calls at once
      DrawCall3 mDrawCalls[ 100 ];
      int mDrawCallCount = 0;

      CommandBuffer mCommandBuffer;
    };
  }
}