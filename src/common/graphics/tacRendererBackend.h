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
      ResizeFramebuffer,
      //UpdateConstantBuffer,
    };

    struct UpdateConstantBuffers
    {
      static const int kCapacity = 2;
      struct UpdateConstantBuffer
      {
        const void* mBytes;
        int mByteCount;
        ConstantBufferHandle mConstantBufferHandle;
      } mUpdateConstantBufferDatas[ kCapacity ];
      int mUpdateConstantBufferDataCount;
      void Push( ConstantBufferHandle, const void* bytes, int byteCount );
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
      TextureHandle mTextureHandle;
      ViewId mViewId = InvalidViewId;
      int mStartIndex = 0;
      int mStartVertex = 0;
      int mIndexCount = 0;
      int mVertexCount = 0;
    };

    struct CommandBuffer
    {
      void PushCommand( CommandType type,
                        StackFrame stackFrame,
                        const void* bytes,
                        int byteCount );
      void Clear();
      const char* Data() const;
      int Size() const;
    private:
      void Push( const void* bytes, int byteCount );
      Vector< char > mBuffer;
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
      View mViews[ 100 ];
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

    struct CommandDataResizeFramebuffer
    {
      int mWidth = 0;
      int mHeight = 0;
      FramebufferHandle mFramebufferHandle;
    };

    struct CommandDataCreateShader
    {
      ShaderSource mShaderSource;
      ConstantBuffers mConstantBuffers;
      ShaderHandle mShaderHandle;
    };

    struct CommandDataCreateConstantBuffer
    {
      ConstantBufferHandle mConstantBufferHandle;
      int mByteCount = 0;
      int mShaderRegister = 0;
    };

    struct CommandDataCreateVertexBuffer
    {
      VertexBufferHandle mVertexBufferHandle;
      int mByteCount = 0;
      void* mOptionalInitialBytes = nullptr;
      int mStride = 0;
      Access mAccess = Access::Default;
    };

    struct CommandDataCreateIndexBuffer
    {
      IndexBufferHandle mIndexBufferHandle;
      int mByteCount;
      void* mOptionalInitialBytes;
      Access mAccess;
      Format mFormat;
    };

    struct CommandDataCreateBlendState
    {
      BlendStateHandle mBlendStateHandle;
      BlendState mBlendState;
    };

    struct CommandDataCreateVertexFormat
    {
      VertexFormatHandle mVertexFormatHandle;
      VertexDeclarations mVertexDeclarations;
      ShaderHandle mShaderHandle;
    };

    struct CommandDataUpdateVertexBuffer
    {
      VertexBufferHandle mVertexBufferHandle;
      const void* mBytes = nullptr;
      int mByteCount = 0;
    };

    struct CommandDataUpdateIndexBuffer
    {
      IndexBufferHandle mIndexBufferHandle;
      const void* mBytes = nullptr;
      int mByteCount = 0;
    };

    //struct CommandDataUpdateConstantBuffer
    //{
    //  ConstantBufferHandle mConstantBufferHandle;
    //  const void* mBytes = nullptr;
    //  int mByteCount = 0;
    //};

    struct CommandDataCreateFramebuffer
    {
      FramebufferHandle mFramebufferHandle;
      DesktopWindowHandle mDesktopWindowHandle;
      int mWidth = 0;
      int mHeight = 0;
    };

    struct CommandDataCreateDepthState
    {
      DepthStateHandle mDepthStateHandle;
      DepthState mDepthState;
    };


    struct CommandDataCreateTexture
    {
      TextureHandle mTextureHandle;
      TexSpec mTexSpec;
    };

    struct CommandDataUpdateTextureRegion
    {
      TextureHandle mTextureHandle;
      TexUpdate mTexUpdate;

    };

    struct CommandDataCreateRasterizerState
    {
      RasterizerStateHandle mRasterizerStateHandle;
      RasterizerState mRasterizerState;
    };

    struct CommandDataCreateSamplerState
    {
      SamplerState mSamplerState;
      SamplerStateHandle mSamplerStateHandle;
    };


  }

  struct Renderer
  {
    enum class Type
    {
      Vulkan,
      OpenGL4,
      DirectX11,
      DirectX12,
      Count,
    };

    static Renderer* Instance;
    Renderer();
    //virtual void CreateWindowContext( DesktopWindow* desktopWindow, Errors& errors ) {}

    virtual ~Renderer();
    virtual void Init( Errors& ) {};
    //virtual void ClearColor( Texture* texture, v4 rgba ) { TAC_UNIMPLEMENTED; }
    //virtual void ClearDepthStencil(
    //  DepthBuffer* depthBuffer,
    //  bool shouldClearDepth,
    //  float depth,
    //  bool shouldClearStencil,
    //  uint8_t stencil )
    //{
    //  TAC_UNIMPLEMENTED;
    //}

    //virtual void SetSamplerState(
    //  const String& samplerName,
    //  SamplerState* samplerState )
    //{
    //  TAC_UNIMPLEMENTED;
    //}

    //virtual void AddCbufferToShader( Shader* shader, CBuffer* cbuffer, ShaderType myShaderType )
    //{
    //  //Unimplemented;
    //}

    //virtual void DebugBegin( const String& section ) { TAC_UNIMPLEMENTED; }
    //virtual void DebugMark( const String& remark ) { TAC_UNIMPLEMENTED; }
    //virtual void DebugEnd() { TAC_UNIMPLEMENTED; }

    //virtual void DrawNonIndexed( int vertexCount = 0 ) { TAC_UNIMPLEMENTED; }

    //virtual void DrawIndexed( int elementCount, int idxOffset, int vtxOffset ) { TAC_UNIMPLEMENTED; }

    //virtual void Apply() { TAC_UNIMPLEMENTED; }

    //virtual void RenderFlush() { TAC_UNIMPLEMENTED; }
    //virtual void Render( Errors& errors ) { TAC_UNIMPLEMENTED; }
    virtual void Render2( const Render::Frame*, Errors& ) { TAC_UNIMPLEMENTED; }
    virtual void SwapBuffers() { TAC_UNIMPLEMENTED; }

    //virtual void SetPrimitiveTopology( Primitive primitive ) { TAC_UNIMPLEMENTED; }

    virtual void GetPerspectiveProjectionAB(
      float f,
      float n,
      float& a,
      float& b )
    {
      TAC_UNUSED_PARAMETER( f );
      TAC_UNUSED_PARAMETER( n );
      TAC_UNUSED_PARAMETER( a );
      TAC_UNUSED_PARAMETER( b );
      TAC_UNIMPLEMENTED;
    }
    //void DebugImgui();

    void ExecuteCommands( Render::CommandBuffer*, Errors& );

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


    String mName;
    Vector< DrawCall2 > mDrawCall2s;
  };
  String RendererTypeToString( Renderer::Type );

}