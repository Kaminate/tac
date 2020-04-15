#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  namespace Render
  {
    enum class CommandType
    {
      CreateVertexBuffer,
      CreateIndexBuffer,
      CreateTexture,
      CreateFramebuffer,

      DestroyVertexBuffer,
      DestroyIndexBuffer,
      DestroyTexture,
      DestroyFramebuffer,

      UpdateTextureRegion,
      UpdateVertexBuffer,
      UpdateIndexBuffer,
    };

    struct CommandBuffer
    {
      void Push( CommandType );
      void Push( const void* bytes, int byteCount );

      
      Vector<char> mBuffer;
    };

    struct Frame
    {
      CommandBuffer mCommandBuffer;
    };
  }
}