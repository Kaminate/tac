#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  namespace Render
  {
    struct CommandDataCreateVertexBuffer
    {
      VertexBufferHandle mVertexBufferHandle;
    };

    struct CommandDataCreateIndexBuffer
    {
      IndexBufferHandle mIndexBufferHandle;
    };

    struct CommandDataCreateTexture
    {
      TextureHandle mTextureHandle;
    };

    struct CommandDataCreateFramebuffer
    {
      FramebufferHandle mHandle;
      int mWidth;
      int mHeight;
      void* mNativeWindowHandle;
    };

    struct CommandDataDestroyResource
    {
      ResourceId mResourceId;
    };

    struct CommandDataUpdateTextureRegion
    {
      TextureHandle mDst;
      Image mSrc;
      int mDstX;
      int mDstY;
    };

    struct CommandDataUpdateBuffer
    {
      ResourceId mResourceId;
      void* mBytes;
      int mByteCount;
    };

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
      void Push( CommandType type );

      void Push( const void* bytes, int byteCount );
      

      Vector<char> mBuffer;
    };

    struct Frame
    {
      CommandBuffer mCommandBuffer;
    };
  }
}