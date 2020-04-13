#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"

namespace Tac
{
  namespace Render
  {
    struct IdCollection
    {
      ResourceId                       Alloc( StringView name, Tac::StackFrame frame );
      void                             Free( ResourceId );
      Vector< ResourceId >             mFree;
      int                              mAllocCounter = 0;
      Vector< String >                 mNames;
      Vector< Tac::StackFrame >        mFrames;
    };

    struct CommandBuffer
    {
      void Push( CommandType type )
      {
        Push( &type, sizeof( CommandType ) );
      }
      void Push( const void* bytes, int byteCount )
      {
        const int bufferSize = mBuffer.size();
        mBuffer.resize( mBuffer.size() + byteCount );
        MemCpy( mBuffer.data() + bufferSize, bytes, byteCount );
      }

      Vector<char> mBuffer;
    };

    struct View
    {
      FramebufferHandle mFramebufferHandle;
    };

    struct Frame
    {
      CommandBuffer mCommandBuffer;
      Vector< View > mViews;
    };
    //extern Frame gRenderFrame;
    //extern Frame gSubmitFrame;
    //Frame gRenderFrame;
    //Frame gSubmitFrame;
    static Frame gRenderFrame;
    static Frame gSubmitFrame;
    static struct// struct ResourceManager
    {
      // why do i need a mutex?
      std::mutex                       ResourceLock;
      IdCollection                     mIdCollectionVertexBuffer;
      IdCollection                     mIdCollectionIndexBuffer;
      IdCollection                     mIdCollectionTexture;
      IdCollection                     mIdCollectionFramebuffer;
    } gResourceManager;

    ResourceId IdCollection::Alloc( StringView name, Tac::StackFrame frame )
    {
      if( mFree.empty() )
      {
        mNames.push_back( name );
        mFrames.push_back( frame );
        return mAllocCounter++;
      }
      const ResourceId result = mFree.back();
      mNames[ result ] = name;
      mFrames[ result ] = frame;
      mFree.pop_back();
      return result;
    }

    void IdCollection::Free( ResourceId id )
    {
      TAC_ASSERT( ( unsigned )id < ( unsigned )mAllocCounter );
      TAC_ASSERT( !Contains( mFree, id ) );
      mFree.push_back( id );
    }



    static char* gSubmitRingBufferBytes;
    static int gSubmitRingBufferCapacity;
    static int gSubmitRingBufferPos;

    void SubmitAllocInit( const int ringBufferByteCount )
    {
      gSubmitRingBufferCapacity = ringBufferByteCount;
      gSubmitRingBufferBytes = new char[ ringBufferByteCount ];
    }

    void* SubmitAlloc( const int byteCount )
    {
      if( gSubmitRingBufferPos + byteCount >= gSubmitRingBufferCapacity )
      {
        gSubmitRingBufferPos = byteCount;
        return gSubmitRingBufferBytes;
      }

      void* result = ( void* )( gSubmitRingBufferBytes + gSubmitRingBufferPos );
      gSubmitRingBufferPos += byteCount;
      return result;
    }

    void* SubmitAlloc( void* bytes, int byteCount )
    {
      void* dst = SubmitAlloc( byteCount );
      MemCpy( dst, bytes, byteCount );
      return dst;
    }

    //void SubmitAllocBeginFrame()
    //{
    //}

    VertexBufferHandle CreateVertexBuffer( StringView name, StackFrame frame )
    {
      const ResourceId resourceId = gResourceManager.mIdCollectionVertexBuffer.Alloc( name, frame );
      const VertexBufferHandle vertexBufferHandle = { resourceId };
      CommandDataCreateVertexBuffer commandData;
      commandData.mVertexBufferHandle = vertexBufferHandle;
      gSubmitFrame.mCommandBuffer.Push( CommandType::CreateVertexBuffer );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return vertexBufferHandle;
    }

    IndexBufferHandle CreateIndexBuffer( StringView name, StackFrame frame )
    {
      const ResourceId resourceId = gResourceManager.mIdCollectionIndexBuffer.Alloc( name, frame );
      const IndexBufferHandle indexBufferHandle = { resourceId };
      CommandDataCreateIndexBuffer commandData;
      commandData.mIndexBufferHandle = indexBufferHandle;
      gSubmitFrame.mCommandBuffer.Push( CommandType::CreateIndexBuffer );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return indexBufferHandle;
    }

    TextureHandle CreateTexture( StringView name, StackFrame frame )
    {
      const ResourceId resourceId = gResourceManager.mIdCollectionTexture.Alloc( name, frame );
      const TextureHandle textureHandle = { resourceId };
      CommandDataCreateTexture commandData;
      commandData.mTextureHandle = textureHandle;
      gSubmitFrame.mCommandBuffer.Push( CommandType::CreateTexture );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return textureHandle;
    }

    FramebufferHandle CreateFramebuffer( void* nativeWindowHandle,
                                         int width,
                                         int height,
                                         StringView name,
                                         StackFrame frame )
    {
      const ResourceId resourceId = gResourceManager.mIdCollectionFramebuffer.Alloc( name, frame );
      const FramebufferHandle handle = { resourceId };
      CommandDataCreateFramebuffer commandData;
      commandData.mHandle = handle;
      commandData.mNativeWindowHandle = nativeWindowHandle;
      commandData.mWidth = width;
      commandData.mHeight = height;
      gSubmitFrame.mCommandBuffer.Push( CommandType::CreateFramebuffer );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return handle;
    }

    void DestroyVertexBuffer( VertexBufferHandle handle )
    {
      gResourceManager.mIdCollectionVertexBuffer.Free( handle.mResourceId );
      const CommandDataDestroyResource commandData = { handle.mResourceId };
      gSubmitFrame.mCommandBuffer.Push( CommandType::DestroyVertexBuffer );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void DestroyIndexBuffer( IndexBufferHandle handle )
    {
      gResourceManager.mIdCollectionIndexBuffer.Free( handle.mResourceId );
      const CommandDataDestroyResource commandData = { handle.mResourceId };
      gSubmitFrame.mCommandBuffer.Push( CommandType::DestroyIndexBuffer );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void DestroyTexture( TextureHandle handle )
    {
      gResourceManager.mIdCollectionTexture.Free( handle.mResourceId );
      const CommandDataDestroyResource commandData = { handle.mResourceId };
      gSubmitFrame.mCommandBuffer.Push( CommandType::DestroyTexture );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void DestroyFramebuffer( FramebufferHandle handle )
    {
      gResourceManager.mIdCollectionFramebuffer.Free( handle.mResourceId );
      const CommandDataDestroyResource commandData = { handle.mResourceId };
      gSubmitFrame.mCommandBuffer.Push( CommandType::DestroyFramebuffer );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void UpdateTextureRegion(
      const TextureHandle mDst,
      const Image mSrc,
      const int mDstX,
      const int mDstY )
    {
      CommandDataUpdateTextureRegion commandData;
      commandData.mDst = mDst;
      commandData.mSrc = mSrc;
      commandData.mDstX = mDstX;
      commandData.mDstY = mDstY;

      gSubmitFrame.mCommandBuffer.Push( CommandType::UpdateTextureRegion );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void UpdateVertexBuffer( VertexBufferHandle handle,
                             void* bytes,
                             int byteCount )
    {
      CommandDataUpdateBuffer commandData;
      commandData.mByteCount = byteCount;
      commandData.mBytes = SubmitAlloc( bytes, byteCount );
      commandData.mResourceId = handle.mResourceId;
      gSubmitFrame.mCommandBuffer.Push( CommandType::UpdateVertexBuffer );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void UpdateIndexBuffer( IndexBufferHandle handle,
                            void* bytes,
                            int byteCount )
    {
      CommandDataUpdateBuffer commandData;
      commandData.mByteCount = byteCount;
      commandData.mBytes = SubmitAlloc( bytes, byteCount );
      commandData.mResourceId = handle.mResourceId;
      gSubmitFrame.mCommandBuffer.Push( CommandType::UpdateIndexBuffer );
      gSubmitFrame.mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

  }
}
