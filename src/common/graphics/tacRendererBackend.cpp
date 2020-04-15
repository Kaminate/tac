#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacOS.h"
#include "src/shell/tacDesktopApp.h"

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

    void CommandBuffer::Push( CommandType type )
    {
      Push( &type, sizeof( CommandType ) );
    }
    void CommandBuffer::Push( const void* bytes, int byteCount )
    {
      const int bufferSize = mBuffer.size();
      mBuffer.resize( mBuffer.size() + byteCount );
      MemCpy( mBuffer.data() + bufferSize, bytes, byteCount );
    }

    struct View
    {
      FramebufferHandle mFramebufferHandle;
    };

    static View gViews[ 10 ];


    static Frame gFrames[ 2 ];
    static Frame* gRenderFrame = &gFrames[ 0 ];
    static Frame* gSubmitFrame = &gFrames[ 1 ];
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

    static bool IsSubmitAllocated( void* data )
    {
      const bool result =
        data >= gSubmitRingBufferBytes &&
        data < gSubmitRingBufferBytes + gSubmitRingBufferCapacity;
      return data;
    }

    //static void AssertSubmitAllocated( void* data )
    //{
    //  TAC_ASSERT( data >= gSubmitRingBufferBytes );
    //  TAC_ASSERT( data < gSubmitRingBufferBytes + gSubmitRingBufferCapacity );
    //}

    static void SubmitAllocInit( const int ringBufferByteCount )
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

    VertexBufferHandle CreateVertexBuffer( StringView name,
                                           CommandDataCreateBuffer commandData,
                                           StackFrame frame )
    {
      const ResourceId resourceId = gResourceManager.mIdCollectionVertexBuffer.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateVertexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }

    IndexBufferHandle CreateIndexBuffer( StringView name,
                                         CommandDataCreateBuffer commandData,
                                         StackFrame frame )
    {
      const ResourceId resourceId = gResourceManager.mIdCollectionIndexBuffer.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateIndexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }

    TextureHandle CreateTexture( StringView name,
                                 CommandDataCreateTexture commandData,
                                 StackFrame frame )
    {
      const ResourceId resourceId = gResourceManager.mIdCollectionTexture.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateTexture );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }

    FramebufferHandle CreateFramebuffer( StringView name,
                                         CommandDataCreateFramebuffer commandData,
                                         StackFrame frame )
    {
      const ResourceId resourceId = gResourceManager.mIdCollectionFramebuffer.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateFramebuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }

    void DestroyVertexBuffer( VertexBufferHandle handle,
                              StackFrame frame )
    {
      gResourceManager.mIdCollectionVertexBuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyVertexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }

    void DestroyIndexBuffer( IndexBufferHandle handle, StackFrame frame )
    {
      gResourceManager.mIdCollectionIndexBuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyIndexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }

    void DestroyTexture( TextureHandle handle, StackFrame frame )
    {
      gResourceManager.mIdCollectionTexture.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyTexture );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }

    void DestroyFramebuffer( FramebufferHandle handle, StackFrame frame )
    {
      gResourceManager.mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyFramebuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }

    void UpdateTextureRegion(
      TextureHandle handle,
      CommandDataUpdateTextureRegion commandData,
      StackFrame frame )
    {
      gSubmitFrame->mCommandBuffer.Push( CommandType::UpdateTextureRegion );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void UpdateVertexBuffer( VertexBufferHandle handle,
                             CommandDataUpdateBuffer commandData,
                             StackFrame frame )

    {
      if( !IsSubmitAllocated(commandData.mBytes) )
        commandData.mBytes = Render::SubmitAlloc( commandData.mBytes, commandData.mByteCount );
      gSubmitFrame->mCommandBuffer.Push( CommandType::UpdateVertexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void UpdateIndexBuffer( IndexBufferHandle handle,
                            CommandDataUpdateBuffer commandData,
                            StackFrame frame )
    {
      if( !IsSubmitAllocated(commandData.mBytes) )
        commandData.mBytes = Render::SubmitAlloc( commandData.mBytes, commandData.mByteCount );
      gSubmitFrame->mCommandBuffer.Push( CommandType::UpdateIndexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void SetViewFramebuffer( ViewId viewId, FramebufferHandle framebufferHandle )
    {
      gViews[ viewId ].mFramebufferHandle = framebufferHandle;
    }

    // need lots of comments pls

    // i think these 2 semaphores just ensure that RenderFrame() and SubmitFrame()
    // alternate calls
    Semaphore::Handle gSubmitSemaphore;
    Semaphore::Handle gRenderSemaphore;

    void RenderFrame()
    {
      TAC_ASSERT( gThreadType == ThreadType::Main );

      Semaphore::WaitAndDecrement( gSubmitSemaphore );

      Errors errors;
      Renderer::Instance->Render2( gRenderFrame, errors );
      TAC_ASSERT( errors.empty() );

      Semaphore::Increment( gRenderSemaphore );

      Renderer::Instance->SwapBuffers();
    }

    static int gFrameCount;
    void SubmitFrame()
    {
      TAC_ASSERT( gThreadType == ThreadType::Stuff );
      Semaphore::WaitAndDecrement( gRenderSemaphore );

      // submit finish

      Swap( gRenderFrame, gSubmitFrame );
      gSubmitFrame->mCommandBuffer.mBuffer.clear();
      gFrameCount++;

      // submit start

      Semaphore::Increment( gSubmitSemaphore );
    }
    void Init( int ringBufferByteCount )
    {
      SubmitAllocInit( ringBufferByteCount );
      gSubmitSemaphore = Semaphore::Create();
      gRenderSemaphore = Semaphore::Create();

      // i guess well make render frame go first
      Semaphore::Increment( gSubmitSemaphore );
    }
  }
}
