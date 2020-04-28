// renderer interface
// used for creating the things necessary to put pretty pixels on the screen
// ( textures, shaders, geometry ( vertexes + indexes ) )

#pragma once

#include "src/common/containers/tacVector.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacVector4.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacPreprocessor.h"
#include <mutex>
#include <set>


// ok so like
// there will be a render thread
// there will be a submit thread
// and then like theres worker threads
//
// the render thread uses render resources
// the submit thread uses submit resources
// the worker threads use encoders.
// the worker thread encoders use a encoder semaphore


namespace Tac
{
  struct DesktopWindow;
  struct Renderer;
  struct CBuffer;
  struct Shell;



  const v4 colorGrey = v4( v3( 1, 1, 1 ) * 95.0f, 255 ) / 255.0f;
  const v4 colorOrange = v4( 255, 200, 84, 255 ) / 255.0f;
  const v4 colorGreen = v4( 0, 255, 112, 255 ) / 255.0f;
  const v4 colorBlue = v4( 84, 255, 255, 255 ) / 255.0f;
  const v4 colorRed = v4( 255, 84, 84, 255 ) / 255.0f;
  const v4 colorMagenta = v4( 255, 84, 255, 255 ) / 255.0f;

  enum class Attribute // Used to hardcode shader semantics/indexes
  {
    Position,
    Normal,
    Texcoord,
    Color,
    BoneIndex,
    BoneWeight,
    Coeffs,
    Count
  };
  enum class GraphicsType
  {
    unknown,
    sint,
    uint,
    snorm,
    unorm,
    real
  };
  enum class DepthFunc
  {
    Less,
    LessOrEqual,
  };
  enum class AddressMode
  {
    Wrap,
    Clamp,
    Border,
  };
  enum class Comparison
  {
    Always,
    Never,
  };
  enum class Filter
  {
    Point,
    Linear,
    Aniso,
  };
  enum class ShaderType
  {
    Vertex,
    Fragment,
    Count,
  };
  enum class Access
  {
    Default = 0, // ?
    Static, // Never gonna change
    Dynamic, // Gonna change ( debug draw, animation )
  };
  enum class CPUAccess
  {
    None = 0b00,
    Read = 0b01,
    Write = 0b10
  };
  enum class Map
  {
    Read,
    Write,
    ReadWrite,
    WriteDiscard, // the previous contents will be discarded
  };
  enum class Primitive
  {
    TriangleList,
    LineList
  };
  enum class BlendMode
  {
    Add,
  };
  enum class BlendConstants
  {
    One,
    Zero,
    SrcRGB,
    SrcA,
    OneMinusSrcA,
  };
  enum class FillMode
  {
    Solid,
    Wireframe
  };
  enum class CullMode
  {
    None,
    Back,
    Front
  };
  enum class Binding
  {
    None = 0b00,
    ShaderResource = 0b01,
    RenderTarget = 0b10,
  };

  const char* GetSemanticName( Attribute attribType );

  // Used so the gpu can translate from cpu types to gpu types
  struct Format
  {
    int CalculateTotalByteCount() const;
    int mElementCount = 0;
    int mPerElementByteCount = 0;
    GraphicsType mPerElementDataType = GraphicsType::unknown;
  };

  const Format formatv2 = { 2, sizeof( float ), GraphicsType::real };
  const Format formatv3 = { 3, sizeof( float ), GraphicsType::real };


  struct Image
  {
    int mWidth = 0;
    int mHeight = 0;

    Format mFormat;

    // byte data should be passed as a separate argument, not as a member of this class
  };
  struct Constant
  {
    String mName;
    int mOffset = 0;
    int mSize = 0;
  };
  struct VertexDeclaration
  {
    Attribute mAttribute = Attribute::Count;
    Format mTextureFormat;

    // Offset of the variable from the vertex buffer
    // ie: OffsetOf( MyVertexType, mPosition)
    int mAlignedByteOffset = 0;
  };

  //struct DepthBufferData : public RendererResource
  //{
  //  int width = 0;
  //  int height = 0;

  //  int mDepthBitCount = 0;
  //  GraphicsType mDepthGraphicsType = GraphicsType::unknown;

  //  int mStencilBitCount = 0;
  //  GraphicsType mStencilType = GraphicsType::unknown;
  //};

  struct DefaultCBufferPerFrame
  {
    m4 mView;
    m4 mProjection;
    float mFar;
    float mNear;
    v2 mGbufferSize;
    static String name_view() { return "View"; };
    static String name_proj() { return "Projection"; };
    static String name_far() { return "far"; };
    static String name_near() { return "near"; };
    static String name_gbuffersize() { return "gbufferSize"; };
    static const int shaderRegister = 0;
  };
  struct DefaultCBufferPerObject
  {
    m4 World;
    v4 Color;
    static String name_world() { return "World"; };
    static String name_color() { return "Color"; };
    static const int shaderRegister = 1;
  };

  v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated );

  struct ScissorRect
  {
    float mXMinRelUpperLeftCornerPixel = 0;
    float mYMinRelUpperLeftCornerPixel = 0;
    float mXMaxRelUpperLeftCornerPixel = 0;
    float mYMaxRelUpperLeftCornerPixel = 0;
  };

  // glViewport lets opengl know how to map the NDC coordinates to the framebuffer coordinates.
  struct Viewport
  {
    float mViewportBottomLeftCornerRelFramebufferBottomLeftCornerX = 0;
    float mViewportBottomLeftCornerRelFramebufferBottomLeftCornerY = 0;
    float mViewportPixelWidthIncreasingRight = 0;
    float mViewportPixelHeightIncreasingUp = 0;
    float mMinDepth = 0;
    float mMaxDepth = 1;
  };

  //struct RenderView
  //{
  //  Texture* mFramebuffer = nullptr;
  //  DepthBuffer* mFramebufferDepth = nullptr;
  //  Viewport mViewportRect;
  //  ScissorRect mScissorRect;
  //  m4 mView = m4::Identity();
  //  m4 mProj = m4::Identity();
  //  v4 mClearColorRGBA = v4( 0, 0, 0, 1 );
  //};

  enum PrimitiveTopology
  {
    TriangleList,
    LineList,
    Count,
  };



  namespace Render
  {
    typedef int ViewId;
    typedef int ResourceId;
    const ResourceId NullResourceId = -1;

#define TAC_RENDER_HANDLE_BODY                            \
    {                                                     \
      ResourceId mResourceId = NullResourceId;            \
      bool IsValid() const                                \
      {                                                   \
        return mResourceId != NullResourceId;             \
      }                                                   \
    }
    struct ShaderHandle          TAC_RENDER_HANDLE_BODY;
    struct VertexBufferHandle    TAC_RENDER_HANDLE_BODY;
    struct IndexBufferHandle     TAC_RENDER_HANDLE_BODY;
    struct ConstantBufferHandle  TAC_RENDER_HANDLE_BODY;
    struct TextureHandle         TAC_RENDER_HANDLE_BODY;
    struct FramebufferHandle     TAC_RENDER_HANDLE_BODY;
    struct BlendStateHandle      TAC_RENDER_HANDLE_BODY;
    struct RasterizerStateHandle TAC_RENDER_HANDLE_BODY;
    struct SamplerStateHandle    TAC_RENDER_HANDLE_BODY;
    struct DepthStateHandle      TAC_RENDER_HANDLE_BODY;
    struct VertexFormatHandle    TAC_RENDER_HANDLE_BODY;
#undef TAC_RENDER_HANDLE_BODY


    struct Frame;


    void RenderFrame();
    void SubmitFrame();

    void Init();

    void* SubmitAlloc( int byteCount );
    const void* SubmitAlloc( const void* bytes, int byteCount );
    //void SubmitAllocBeginFrame();

    struct CommandDataCreateVertexBuffer
    {
      int mByteCount = 0;
      void* mOptionalInitialBytes = nullptr;
      int mStride;
      //Format mFormat; no. format goes in the inputlayout
      Access mAccess = Access::Default;
    };

    struct CommandDataCreateIndexBuffer
    {
      int mByteCount = 0;
      void* mOptionalInitialBytes = nullptr;
      Access mAccess = Access::Default;
      Format mFormat;
    };

    struct CommandDataCreateConstantBuffer
    {
      int mByteCount = 0;
      int mShaderRegister = 0;
    };

    struct CommandDataCreateBlendState
    {
      BlendConstants srcRGB = BlendConstants::One;
      BlendConstants dstRGB = BlendConstants::Zero;
      BlendMode blendRGB = BlendMode::Add;
      BlendConstants srcA = BlendConstants::One;
      BlendConstants dstA = BlendConstants::Zero;
      BlendMode blendA = BlendMode::Add;
    };
    struct CommandDataCreateRasterizerState
    {
      FillMode fillMode = ( FillMode )0;
      CullMode cullMode = ( CullMode )0;
      bool frontCounterClockwise = false;
      bool scissor = false;
      bool multisample = false;
    };

    struct CommandDataCreateSamplerState
    {
      AddressMode u = ( AddressMode )0;
      AddressMode v = ( AddressMode )0;
      AddressMode w = ( AddressMode )0;
      Comparison compare = ( Comparison )0;
      Filter filter = ( Filter )0;
    };

    struct CommandDataCreateDepthState
    {
      bool depthTest = false;
      bool depthWrite = false;
      DepthFunc depthFunc = ( DepthFunc )0;
    };

    struct CommandDataCreateVertexFormat
    {
      VertexDeclaration mVertexFormatDatas[10];
      int mVertexFormatDataCount = 0;
      ShaderHandle mShaderHandle;

      void AddVertexDeclaration( VertexDeclaration v )
      {
        mVertexFormatDatas[ mVertexFormatDataCount++ ] = v;
      }
    };

    struct CommandDataCreateTexture
    {
      Image mImage;
      int mPitch = 0; // byte count between texel rows
      const void* mImageBytes = nullptr;
      const void* mImageBytesCubemap[ 6 ] = {};
      Binding mBinding = Binding::None;
      Access mAccess = Access::Default;
      CPUAccess mCpuAccess = CPUAccess::None;
    };

    struct CommandDataCreateFramebuffer
    {
      int mWidth = 0;
      int mHeight = 0;
      void* mNativeWindowHandle = nullptr;
    };

    struct CommandDataUpdateTextureRegion
    {
      Image mSrc;
      int mDstX = 0;
      int mDstY = 0;
      const void* mSrcBytes = nullptr;
      int mPitch = 0; // byte count between pixel rows
    };

    struct CommandDataUpdateBuffer
    {
      const void* mBytes = nullptr;
      int mByteCount = 0;
    };

    struct CommandDataCreateShader
    {
      // can load from either
      StringView mShaderPath;
      StringView mShaderStr;
      ConstantBufferHandle mConstantBuffers[10];
      int mConstantBufferCount = 0;
      void AddConstantBuffer(ConstantBufferHandle handle)
      {
        mConstantBuffers[mConstantBufferCount++] = handle;
      }
    };

    ShaderHandle                     CreateShader( StringView, CommandDataCreateShader, StackFrame );
    ConstantBufferHandle             CreateConstantBuffer( StringView, CommandDataCreateConstantBuffer, StackFrame );
    VertexBufferHandle               CreateVertexBuffer( StringView, CommandDataCreateVertexBuffer, StackFrame );
    IndexBufferHandle                CreateIndexBuffer( StringView, CommandDataCreateIndexBuffer, StackFrame );
    TextureHandle                    CreateTexture( StringView, CommandDataCreateTexture, StackFrame );
    FramebufferHandle                CreateFramebuffer( StringView,
                                                        CommandDataCreateFramebuffer,
                                                        StackFrame );
    BlendStateHandle                 CreateBlendState( StringView, CommandDataCreateBlendState, StackFrame );
    RasterizerStateHandle            CreateRasterizerState( StringView, CommandDataCreateRasterizerState, StackFrame );
    SamplerStateHandle               CreateSamplerState( StringView, CommandDataCreateSamplerState, StackFrame );
    DepthStateHandle                 CreateDepthState( StringView, CommandDataCreateDepthState, StackFrame );
    VertexFormatHandle               CreateVertexFormat( StringView, CommandDataCreateVertexFormat, StackFrame );

    void                             DestroyVertexBuffer( VertexBufferHandle, StackFrame );
    void                             DestroyIndexBuffer( IndexBufferHandle, StackFrame );
    void                             DestroyTexture( TextureHandle, StackFrame );
    void                             DestroyFramebuffer( FramebufferHandle, StackFrame );
    void                             DestroyShader( ShaderHandle, StackFrame );
    void                             DestroyVertexFormat( VertexFormatHandle, StackFrame );
    void                             DestroyConstantBuffer( ConstantBufferHandle, StackFrame );
    void                             DestroyConstantBuffer( ConstantBufferHandle, StackFrame );
    void                             DestroyDepthState( DepthStateHandle, StackFrame );
    void                             DestroyBlendState( BlendStateHandle, StackFrame );
    void                             DestroyRasterizerState( RasterizerStateHandle, StackFrame );
    void                             DestroySamplerState( SamplerStateHandle, StackFrame );

    void                             UpdateTextureRegion( TextureHandle mDst,
                                                          CommandDataUpdateTextureRegion,
                                                          StackFrame );
    void                             UpdateVertexBuffer( VertexBufferHandle,
                                                         CommandDataUpdateBuffer,
                                                         StackFrame );
    void                             UpdateIndexBuffer( IndexBufferHandle,
                                                        CommandDataUpdateBuffer,
                                                        StackFrame );
    void                             UpdateConstantBuffer( ConstantBufferHandle,
                                                           CommandDataUpdateBuffer,
                                                           StackFrame );

    void                             SetViewFramebuffer( ViewId viewId,
                                                         FramebufferHandle framebufferHandle );
    void                             SetIndexBuffer( IndexBufferHandle, int iStart, int count );
    void                             SetVertexBuffer( VertexBufferHandle, int iStart, int count );
    void                             SetBlendState( BlendStateHandle );
    void                             SetRasterizerState( RasterizerStateHandle );
    void                             SetSamplerState( SamplerStateHandle );
    void                             SetDepthState( DepthStateHandle );
    void                             SetVertexFormat( VertexFormatHandle );
    //void                             SetUniform( ConstantBufferHandle, const void* bytes, int byteCount );

    void                             Submit( ViewId viewId );
  }

  struct DrawCall2
  {
    Render::ShaderHandle mShader;
    Render::VertexBufferHandle mVertexBuffer;
    Render::IndexBufferHandle mIndexBuffer;
    int mStartIndex = 0;
    int mIndexCount = 0;
    int mVertexCount = 0;
    Render::BlendStateHandle mBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle mSamplerState;
    Render::DepthStateHandle mDepthState;
    Render::VertexFormatHandle mVertexFormat;
    Vector< Render::TextureHandle > mTextureHandles;
    Render::ConstantBufferHandle mUniformDst;
    Vector< char > mUniformSrcc;
    StackFrame mFrame;
    PrimitiveTopology mPrimitiveTopology = PrimitiveTopology::TriangleList;

    template< typename T>
    void CopyUniformSource( const T& t ) { CopyUniformSource( &t, sizeof( T ) ); }
    void CopyUniformSource( const void* bytes, int byteCount );
  };

  struct Renderer
  {
    enum class Type
    {
      Vulkan,
      OpenGL4,
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
    virtual void Render2( Render::Frame*, Errors& ) { TAC_UNIMPLEMENTED; }
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


    String mName;
    void AddDrawCall( const DrawCall2& drawCall );
    Vector< DrawCall2 > mDrawCall2s;
  };

  struct RendererFactory
  {
  public:
    virtual ~RendererFactory() = default;
    void CreateRendererOuter();
    String mRendererName;
  protected:
    virtual void CreateRenderer() { TAC_UNIMPLEMENTED; }
  };

  struct RendererRegistry
  {
    static RendererRegistry& Instance();
    RendererFactory* FindFactory( StringView name );

    Vector< RendererFactory* > mFactories;
  };

  String ToString( Renderer::Type );
  const static String RendererNameVulkan = "Vulkan";
  const static String RendererNameOpenGL4 = "OpenGL4";
  const static String RendererNameDirectX11 = "DirectX11";
  const static String RendererNameDirectX12 = "DirectX12";
}
