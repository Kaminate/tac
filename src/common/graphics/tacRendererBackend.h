#pragma once

#include "src/common/graphics/tacRenderer.h"
#include "src/common/containers/tacFixedVector.h"
#include "src/common/containers/tacVector.h"
#include "src/common/string/tacString.h"

namespace Tac
{
  namespace Render
  {
    const int kDrawCallCapacity = 1000;
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
    const int kMaxViews = 100;

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
      ResizeFramebuffer,
    };

    struct UpdateConstantBufferData
    {
      const void*          mBytes;
      int                  mByteCount;
      ConstantBufferHandle mConstantBufferHandle;
      StackFrame           mStackFrame;
    };
    typedef FixedVector< UpdateConstantBufferData, 2 > UpdateConstantBuffers;

    struct DrawCall3
    {
      StackFrame            mStackFrame;
      VertexBufferHandle    mVertexBufferHandle;
      IndexBufferHandle     mIndexBufferHandle;
      BlendStateHandle      mBlendStateHandle;
      RasterizerStateHandle mRasterizerStateHandle;
      SamplerStateHandle    mSamplerStateHandle;
      DepthStateHandle      mDepthStateHandle;
      VertexFormatHandle    mVertexFormatHandle;
      UpdateConstantBuffers mUpdateConstantBuffers;
      ShaderHandle          mShaderHandle;
      DrawCallTextures      mTextureHandle;
      ViewHandle            mViewHandle;
      int                   mStartIndex = 0;
      int                   mStartVertex = 0;
      int                   mIndexCount = 0;
      int                   mVertexCount = 0;
      int                   iUniformBegin = 0;
      int                   iUniformEnd = 0;
    };
    typedef FixedVector< DrawCall3, kDrawCallCapacity > DrawCalls;

    struct CommandBuffer
    {
      void           PushCommand( CommandType,
                                  const void* bytes,
                                  int byteCount );
      void           Resize( int newSize );
      void           Clear();
      const char*    Data() const;
      int            Size() const;
    private:
      void           Push( const void* bytes, int byteCount );
      Vector< char > mBuffer;
    };

    enum class UniformBufferEntryType
    {
      Unknown = 0,
      DebugGroupBegin,
      DebugMarker,
      DebugGroupEnd,
      UpdateConstantBuffer,
    };

    struct UniformBufferHeader
    {
      UniformBufferHeader() = default;
      UniformBufferHeader( UniformBufferEntryType, StackFrame );
      UniformBufferEntryType mType = UniformBufferEntryType::Unknown;
      int                    mCorruption = 0xd34db33f;
      StackFrame             mStackFrame;
    };

    struct UniformBuffer
    {
      struct Pusher
      {
        virtual Pusher*        PushData( const void*, int ) = 0;
        virtual Pusher*        PushString( StringView ) = 0;
        virtual Pusher*        PushNumber( int ) = 0;
      };

      Pusher*                  PushHeader( UniformBufferHeader );
      int                      size() const;
      void*                    data() const;
      void                     clear();


      struct Iterator
      {
        Iterator( const UniformBuffer*, int iBegin, int iEnd );
        UniformBufferHeader    PopHeader();
        void*                  PopData( int );
        int                    PopNumber();
        StringView             PopString();
        const char*            mCur;
        const char*            mEnd;
      };

    private:
      static const int         kByteCapacity = 256 * 1024;
      char                     mBytes[ kByteCapacity ] = {};
      int                      mByteCount = 0;
    };

    struct View
    {
      FramebufferHandle mFrameBufferHandle;
      Viewport          mViewport;
      ScissorRect       mScissorRect;
      bool              mViewportSet = false;
      bool              mScissorSet = false;
    };

    // This struct holds handles deleted this frame
    struct FreeDeferredHandles
    {
      // Let's say you do the following:
      //   Render::DestroyVertexBuffer( foo.vertexbuffer );
      //   bar.vertexBuffer = Render::CreateVertexBuffer();
      // We don't want to reuse the same vertex buffer handle, because it will be destroyed
      // at the end of the frame n during ExecuteCommands( mCommandBufferFrameEnd ),
      // causing problems for frame n+1 which is still trying to use the resource.
      // so instead defer deletion of the handle ( not the resource ) until we are done issuing commands.
      void FinishFreeingHandles();
      FixedVector< BlendStateHandle, kMaxBlendStates >           mFreedBlendStates;
      FixedVector< ConstantBufferHandle, kMaxConstantBuffers >   mFreedConstantBuffers;
      FixedVector< DepthStateHandle, kMaxDepthStencilStates >    mFreedDepthStencilStates;
      FixedVector< FramebufferHandle, kMaxFramebuffers >         mFreedFramebuffers;
      FixedVector< IndexBufferHandle, kMaxIndexBuffers >         mFreedIndexBuffers;
      FixedVector< RasterizerStateHandle, kMaxRasterizerStates > mFreedRasterizerStates;
      FixedVector< SamplerStateHandle, kMaxSamplerStates >       mFreedSamplerStates;
      FixedVector< ShaderHandle, kMaxPrograms >                  mFreedShaders;
      FixedVector< TextureHandle, kMaxTextures >                 mFreedTextures;
      FixedVector< VertexBufferHandle, kMaxVertexBuffers >       mFreedVertexBuffers;
      FixedVector< VertexFormatHandle, kMaxInputLayouts >        mFreedVertexFormatInputLayouts;
      FixedVector< ViewHandle, kMaxViews >                       mFreedViews;
    };

    struct Frame
    {
      Frame();
      void                Clear();
      CommandBuffer       mCommandBufferFrameBegin;
      CommandBuffer       mCommandBufferFrameEnd;
      DrawCalls           mDrawCalls;
      UniformBuffer       mUniformBuffer;
      View                mViews[ kMaxViews ];
      bool                mBreakOnFrameRender;
      int                 mBreakOnDrawCall;
      FreeDeferredHandles mFreeDeferredHandles;
    };

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
      ShaderSource          mShaderSource;
      ConstantBuffers       mConstantBuffers;
      ShaderHandle          mShaderHandle;
    };

    struct CommandDataCreateConstantBuffer
    {
      StackFrame            mStackFrame;
      ConstantBufferHandle  mConstantBufferHandle;
      int                   mByteCount = 0;
      int                   mShaderRegister = 0;
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

      //                    Used when creating a framebuffer for render-to-texture
      //TextureHandle         mTextures[ 10 ] = {};
      FramebufferTextures   mFramebufferTextures;
      //TextureHandle*        mTextures;
      //int                   mTextureCount = 0;
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

    bool IsSubmitAllocated( const void* data );

    void DebugPrintSubmitAllocInfo();

    typedef void ShaderReloadFunction( ShaderHandle, const char* );
    void ShaderReloadHelperAdd( ShaderHandle, const char* );
    void ShaderReloadHelperRemove( ShaderHandle );
    void ShaderReloadHelperUpdate( ShaderReloadFunction* );
  }


  struct Renderer
  {
    //enum class Type
    //{
    //  Vulkan,
    //  OpenGL4,
    //  DirectX11,
    //  DirectX12,
    //  Count,
    //};

    static Renderer* Instance;
    const char*      mName;
    Renderer();
    virtual ~Renderer();
    void         ExecuteCommands( Render::CommandBuffer*, Errors& );
    virtual void Init( Errors& ) {};
    virtual void RenderBegin( const Render::Frame*, Errors& ) = 0;
    virtual void RenderDrawCall( const Render::Frame*, const Render::DrawCall3*, Errors& ) = 0;
    virtual void RenderEnd( const Render::Frame*, Errors& ) = 0;

    virtual void SwapBuffers() = 0;
    virtual void GetPerspectiveProjectionAB( float f,
                                             float n,
                                             float& a,
                                             float& b ) = 0;
    virtual void AddBlendState( Render::CommandDataCreateBlendState*, Errors& ) = 0;
    virtual void AddConstantBuffer( Render::CommandDataCreateConstantBuffer*, Errors& ) = 0;
    virtual void AddDepthState( Render::CommandDataCreateDepthState*, Errors& ) = 0;
    virtual void AddFramebuffer( Render::CommandDataCreateFramebuffer*, Errors& ) = 0;
    virtual void AddIndexBuffer( Render::CommandDataCreateIndexBuffer*, Errors& ) = 0;
    virtual void AddRasterizerState( Render::CommandDataCreateRasterizerState*, Errors& ) = 0;
    virtual void AddSamplerState( Render::CommandDataCreateSamplerState*, Errors& ) = 0;
    virtual void AddShader( Render::CommandDataCreateShader*, Errors& ) = 0;
    virtual void AddTexture( Render::CommandDataCreateTexture*, Errors& ) = 0;
    virtual void AddVertexBuffer( Render::CommandDataCreateVertexBuffer*, Errors& ) = 0;
    virtual void AddVertexFormat( Render::CommandDataCreateVertexFormat*, Errors& ) = 0;
    virtual void UpdateTextureRegion( Render::CommandDataUpdateTextureRegion*, Errors& ) = 0;
    virtual void UpdateVertexBuffer( Render::CommandDataUpdateVertexBuffer*, Errors& ) = 0;
    virtual void UpdateIndexBuffer( Render::CommandDataUpdateIndexBuffer*, Errors& ) = 0;
    virtual void UpdateConstantBuffer( Render::CommandDataUpdateConstantBuffer*, Errors& ) = 0;
    virtual void ResizeFramebuffer( Render::CommandDataResizeFramebuffer*, Errors& ) = 0;
    virtual void RemoveBlendState( Render::BlendStateHandle, Errors& ) = 0;
    virtual void RemoveConstantBuffer( Render::ConstantBufferHandle, Errors& ) = 0;
    virtual void RemoveDepthState( Render::DepthStateHandle, Errors& ) = 0;
    virtual void RemoveRasterizerState( Render::RasterizerStateHandle, Errors& ) = 0;
    virtual void RemoveSamplerState( Render::SamplerStateHandle, Errors& ) = 0;
    virtual void RemoveShader( Render::ShaderHandle, Errors& ) = 0;
    virtual void RemoveVertexFormat( Render::VertexFormatHandle, Errors& ) = 0;
    virtual void RemoveVertexBuffer( Render::VertexBufferHandle, Errors& ) = 0;
    virtual void RemoveIndexBuffer( Render::IndexBufferHandle, Errors& ) = 0;
    virtual void RemoveTexture( Render::TextureHandle, Errors& ) = 0;
    virtual void RemoveFramebuffer( Render::FramebufferHandle, Errors& ) = 0;
    virtual void DebugGroupBegin( StringView ) = 0;
    virtual void DebugMarker( StringView ) = 0;
    virtual void DebugGroupEnd() = 0;
  };

  //String RendererTypeToString( Renderer::Type );
}