// renderer interface
// used for creating the things necessary to put pretty pixels on the screen
// ( textures, shaders, geometry ( vertexes + indexes ) )

#pragma once

#include "src/common/tacPreprocessor.h"

namespace Tac
{
  struct StringView;
  struct Errors;

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
    int          CalculateTotalByteCount() const;
    int          mElementCount = 0;
    int          mPerElementByteCount = 0;
    GraphicsType mPerElementDataType = GraphicsType::unknown;
  };



  struct Image
  {
    int    mWidth = 0;
    int    mHeight = 0;

    Format mFormat;

    // byte data should be passed as a separate argument, not as a member of this class
  };

  //struct Constant
  //{
  //  String mName;
  //  int    mOffset = 0;
  //  int    mSize = 0;
  //};

  struct VertexDeclaration
  {
    Attribute mAttribute = Attribute::Count;
    Format    mTextureFormat;

    // Offset of the variable from the vertex buffer
    // ie: OffsetOf( MyVertexType, mPosition)
    int       mAlignedByteOffset = 0;
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

  struct ScissorRect
  {
    ScissorRect() = default;
    ScissorRect( float w, float h );
    ScissorRect( int w, int h );
    float mXMinRelUpperLeftCornerPixel = 0;
    float mYMinRelUpperLeftCornerPixel = 0;
    float mXMaxRelUpperLeftCornerPixel = 0;
    float mYMaxRelUpperLeftCornerPixel = 0;
  };

  // glViewport lets opengl know how to map the NDC coordinates to the framebuffer coordinates.
  struct Viewport
  {
    Viewport() = default;
    Viewport( float w, float h );
    Viewport( int w, int h );
    float mBottomLeftX = 0;
    float mBottomLeftY = 0;
    float mWidth = 0;
    float mHeight = 0;
    float mMinDepth = 0;
    float mMaxDepth = 1;
  };

  enum PrimitiveTopology
  {
    TriangleList,
    LineList,
    Count,
  };

  namespace Render
  {
    TAC_DEFINE_HANDLE( ShaderHandle );
    TAC_DEFINE_HANDLE( VertexBufferHandle );
    TAC_DEFINE_HANDLE( IndexBufferHandle );
    TAC_DEFINE_HANDLE( ConstantBufferHandle );
    TAC_DEFINE_HANDLE( TextureHandle );
    TAC_DEFINE_HANDLE( FramebufferHandle );
    TAC_DEFINE_HANDLE( BlendStateHandle );
    TAC_DEFINE_HANDLE( RasterizerStateHandle );
    TAC_DEFINE_HANDLE( SamplerStateHandle );
    TAC_DEFINE_HANDLE( DepthStateHandle );
    TAC_DEFINE_HANDLE( VertexFormatHandle );
    TAC_DEFINE_HANDLE( ViewHandle );

    struct Frame;


    struct BlendState
    {
      BlendConstants mSrcRGB = BlendConstants::One;
      BlendConstants mDstRGB = BlendConstants::Zero;
      BlendMode      mBlendRGB = BlendMode::Add;
      BlendConstants mSrcA = BlendConstants::One;
      BlendConstants mDstA = BlendConstants::Zero;
      BlendMode      mBlendA = BlendMode::Add;
    };

    struct VertexDeclarations
    {
      VertexDeclarations() = default;
      VertexDeclarations( VertexDeclaration );
      VertexDeclarations( VertexDeclaration, VertexDeclaration );
      void              AddVertexDeclaration( VertexDeclaration );
      VertexDeclaration mVertexFormatDatas[ 10 ];
      int               mVertexFormatDataCount = 0;
    };

    struct DrawCallTextures
    {
      DrawCallTextures() = default;
      DrawCallTextures( TextureHandle );
      DrawCallTextures( TextureHandle, TextureHandle );
      void                    AddTexture( TextureHandle );
      const TextureHandle*    begin() const;
      const TextureHandle*    end() const;
      TextureHandle           operator[]( int i ) const;
      TextureHandle           mTextures[ 2 ];
      int                     mTextureCount = 0;
    };

    struct ShaderSource
    {
      enum Type
      {
        kPath,
        kStr
      }                   mType;
      const char*         mStr;
      static ShaderSource FromPath( const char* );
      static ShaderSource FromStr( const char* );
    };

    struct ConstantBuffers
    {
      ConstantBuffers() = default;
      ConstantBuffers( ConstantBufferHandle );
      ConstantBuffers( ConstantBufferHandle, ConstantBufferHandle );
      ConstantBuffers( ConstantBufferHandle*, int );
      void                 AddConstantBuffer( ConstantBufferHandle handle );
      ConstantBufferHandle mConstantBuffers[ 10 ];
      int                  mConstantBufferCount = 0;
    };

    struct DepthState
    {
      bool      mDepthTest = false;
      bool      mDepthWrite = false;
      DepthFunc mDepthFunc = ( DepthFunc )0;
    };

    struct TexSpec
    {
      Image       mImage;
      int         mPitch = 0; // byte count between texel rows
      const void* mImageBytes = nullptr;
      const void* mImageBytesCubemap[ 6 ] = {};
      Binding     mBinding = Binding::None;
      Access      mAccess = Access::Default;
      CPUAccess   mCpuAccess = CPUAccess::None;
    };

    struct TexUpdate
    {
      Image       mSrc;
      int         mDstX = 0;
      int         mDstY = 0;
      const void* mSrcBytes = nullptr;
      int         mPitch = 0; // byte count between pixel rows
    };

    struct RasterizerState
    {

      FillMode mFillMode = ( FillMode )0;
      CullMode mCullMode = ( CullMode )0;
      bool     mFrontCounterClockwise = false;
      bool     mScissor = false;
      bool     mMultisample = false;
    };

    struct SamplerState
    {
      AddressMode mU = ( AddressMode )0;
      AddressMode mV = ( AddressMode )0;
      AddressMode mW = ( AddressMode )0;
      Comparison  mCompare = ( Comparison )0;
      Filter      mFilter = ( Filter )0;
    };


    void                             Init( Errors& );
    void                             RenderFrame( Errors& );
    void                             SubmitFrame();

    void*                            SubmitAlloc( int byteCount );
    const void*                      SubmitAlloc( const void* bytes, int byteCount );
    ViewHandle                       CreateView();
    void                             DestroyView( ViewHandle );
    ShaderHandle                     CreateShader(  ShaderSource, ConstantBuffers, StackFrame );
    ConstantBufferHandle             CreateConstantBuffer( int mByteCount,
                                                           int mShaderRegister,
                                                           StackFrame );
    VertexBufferHandle               CreateVertexBuffer( int mByteCount,
                                                         const void* mOptionalInitialBytes,
                                                         int mStride,
                                                         Access mAccess,
                                                         StackFrame );
    IndexBufferHandle                CreateIndexBuffer( int byteCount,
                                                        const void* optionalInitialBytes,
                                                        Access access,
                                                        Format format,
                                                        StackFrame );
    TextureHandle                    CreateTexture(  TexSpec, StackFrame );
    FramebufferHandle                CreateFramebuffer( const void* nativeWindowHandle,
                                                        int width,
                                                        int weight,
                                                        StackFrame );
    BlendStateHandle                 CreateBlendState( BlendState, StackFrame );
    RasterizerStateHandle            CreateRasterizerState( RasterizerState, StackFrame );
    SamplerStateHandle               CreateSamplerState( SamplerState, StackFrame );
    DepthStateHandle                 CreateDepthState( DepthState, StackFrame );
    VertexFormatHandle               CreateVertexFormat( VertexDeclarations, ShaderHandle, StackFrame );

    void                             DestroyVertexBuffer( VertexBufferHandle, StackFrame );
    void                             DestroyIndexBuffer( IndexBufferHandle, StackFrame );
    void                             DestroyTexture( TextureHandle, StackFrame );
    void                             DestroyFramebuffer( FramebufferHandle, StackFrame );
    void                             DestroyShader( ShaderHandle, StackFrame );
    void                             DestroyVertexFormat( VertexFormatHandle, StackFrame );
    void                             DestroyConstantBuffer( ConstantBufferHandle, StackFrame );
    void                             DestroyDepthState( DepthStateHandle, StackFrame );
    void                             DestroyBlendState( BlendStateHandle, StackFrame );
    void                             DestroyRasterizerState( RasterizerStateHandle, StackFrame );
    void                             DestroySamplerState( SamplerStateHandle, StackFrame );

    void                             UpdateTextureRegion( TextureHandle,
                                                          TexUpdate,
                                                          StackFrame );
    void                             UpdateVertexBuffer( VertexBufferHandle,
                                                         const void*,
                                                         int,
                                                         StackFrame );
    void                             UpdateIndexBuffer( IndexBufferHandle,
                                                        const void*,
                                                        int,
                                                        StackFrame );

    // Umm about this...
    void                             UpdateConstantBuffer( ConstantBufferHandle,
                                                           const void*,
                                                           int,
                                                           StackFrame );
    void                             UpdateConstantBuffer2( ConstantBufferHandle,
                                                            const void*,
                                                            int,
                                                            StackFrame );
    // Umm about this...

    void                             ResizeFramebuffer( FramebufferHandle,
                                                        int w,
                                                        int h,
                                                        StackFrame );

    void                             SetViewFramebuffer( ViewHandle, FramebufferHandle );
    void                             SetViewScissorRect( ViewHandle, ScissorRect );
    void                             SetViewport( ViewHandle, Viewport );
    void                             SetIndexBuffer( IndexBufferHandle, int iStart, int count );
    void                             SetVertexBuffer( VertexBufferHandle, int iStart, int count );
    void                             SetBlendState( BlendStateHandle );
    void                             SetRasterizerState( RasterizerStateHandle );
    void                             SetSamplerState( SamplerStateHandle );
    void                             SetDepthState( DepthStateHandle );
    void                             SetVertexFormat( VertexFormatHandle );
    void                             SetShader( ShaderHandle );
    void                             SetTexture( DrawCallTextures );
    void                             SetBreakpointWhenThisFrameIsRendered();
    void                             SetBreakpointWhenNextDrawCallIsExecuted();
    void                             Submit( ViewHandle, StackFrame );
    void                             GetPerspectiveProjectionAB( float f, float n, float& a, float& b );
    void                             BeginGroup( StringView, StackFrame );
    void                             EndGroup( StackFrame );
    void                             Uninit();

#define TAC_RENDER_GROUP_BLOCK( text )                          \
        Render::BeginGroup( text, TAC_STACK_FRAME );            \
        TAC_ON_DESTRUCT( Render::EndGroup( TAC_STACK_FRAME ) );

  }

  struct RendererFactory
  {
    const char* mRendererName;
    void( *mCreateRenderer )( );
  };

  // could move it to the c++ file, but plan to be able to
  // pick a renderer from a dropdown in the future
  struct RendererRegistry
  {
    RendererFactory* begin();
    RendererFactory* end();
  };

  RendererFactory*           RendererFactoriesFind( StringView );
  void                       RendererFactoriesRegister( RendererFactory );



  const char* const RendererNameVulkan = "Vulkan";
  const char* const RendererNameOpenGL4 = "OpenGL4";
  const char* const RendererNameDirectX11 = "DirectX11";
  const char* const RendererNameDirectX12 = "DirectX12";
  //const char* RendererTypeToString( Renderer::Type );
}

