// renderer interface
// used for creating the things necessary to put pretty pixels on the screen
// ( textures, shaders, geometry ( vertexes + indexes ) )

#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
//#include "tac-std-lib/tac_core.h"
#include "tac-std-lib/memory/tac_memory.h"

#include "tac-rhi/identifier/tac_handle.h"

//import std; // initializer list for tac_fixed vaector???

namespace Tac { struct Errors; struct v2;}
namespace Tac::Render
{
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
    Geometry,
    Compute,
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

#if 0
  enum class Map
  {
    Read,
    Write,
    ReadWrite,
    WriteDiscard, // the previous contents will be discarded
  };
#endif

  // Use PrimitiveTopology

  //enum class Primitive
  //{
  //  TriangleList,
  //  LineList
  //};

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

  enum class Binding : int
  {
    None = 0b0000,

    // so what i really think this means is like,
    // create a ID3D11ShaderResourceView* for this
    ShaderResource = 0b0001,
    RenderTarget = 0b0010,
    DepthStencil = 0b0100,
    UnorderedAccess = 0b1000,
  };

  Binding operator | ( Binding, Binding );
  Binding operator & ( Binding, Binding );

  enum class PrimitiveTopology
  {
    Unknown = 0,
    TriangleList,
    PointList,
    LineList,
  };

  const char* GetSemanticName( Attribute );

  struct FormatElement
  {
    int          mPerElementByteCount { 0 };
    GraphicsType mPerElementDataType { GraphicsType::unknown };

    static const FormatElement sFloat;
  };

  // Used so the gpu can translate from cpu types to gpu types
  struct Format
  {
    int          CalculateTotalByteCount() const;
    int          mElementCount { 0 };
    int          mPerElementByteCount { 0 };
    GraphicsType mPerElementDataType { GraphicsType::unknown };

    static Format FromElements( FormatElement, int = 1 );
    static const Format sfloat;
    static const Format sv2;
    static const Format sv3;
    static const Format sv4;
  };


  // $$$ Should this still be called an "Image", since the data parameter was removed?
  struct Image
  {
    int    mWidth { 0 };
    int    mHeight { 0 };
    int    mDepth { 0 };
    Format mFormat;

    // Note that byte data should be passed as a separate argument,
    // and not as a member of this class
  };

  struct VertexDeclaration
  {
    Attribute mAttribute = Attribute::Count;
    Format    mTextureFormat;

    //        Offset of the variable from the vertex buffer
    //        ie: OffsetOf( MyVertexType, mPosition)
    int       mAlignedByteOffset { 0 };
  };

  struct ScissorRect
  {
    ScissorRect() = default;
    ScissorRect( float w, float h );
    ScissorRect( int w, int h );
    ScissorRect( const v2& );
    float mXMinRelUpperLeftCornerPixel { 0 };
    float mYMinRelUpperLeftCornerPixel { 0 };
    float mXMaxRelUpperLeftCornerPixel { 0 };
    float mYMaxRelUpperLeftCornerPixel { 0 };
  };

  // glViewport lets opengl know how to map the NDC coordinates to the framebuffer coordinates.
  struct Viewport
  {
    Viewport() = default;
    Viewport( float w, float h );
    Viewport( int w, int h );
    Viewport( const v2& );
    float mBottomLeftX { 0 };
    float mBottomLeftY { 0 };
    float mWidth       { 0 };
    float mHeight      { 0 };
    float mMinDepth    { 0 };
    float mMaxDepth    { 1 };
  };


  TAC_DEFINE_HANDLE( BlendStateHandle );
  TAC_DEFINE_HANDLE( ConstantBufferHandle );
  TAC_DEFINE_HANDLE( DepthStateHandle );
  TAC_DEFINE_HANDLE( FramebufferHandle );
  TAC_DEFINE_HANDLE( IndexBufferHandle );
  TAC_DEFINE_HANDLE( MagicBufferHandle );
  TAC_DEFINE_HANDLE( RasterizerStateHandle );
  TAC_DEFINE_HANDLE( SamplerStateHandle );
  TAC_DEFINE_HANDLE( ShaderHandle );
  TAC_DEFINE_HANDLE( TextureHandle );
  TAC_DEFINE_HANDLE( VertexBufferHandle );
  TAC_DEFINE_HANDLE( VertexFormatHandle );
  TAC_DEFINE_HANDLE( ViewHandle );

  //struct Frame;


  struct BlendState
  {
    BlendConstants mSrcRGB   { BlendConstants::One };
    BlendConstants mDstRGB   { BlendConstants::Zero };
    BlendMode      mBlendRGB { BlendMode::Add };
    BlendConstants mSrcA     { BlendConstants::One };
    BlendConstants mDstA     { BlendConstants::Zero };
    BlendMode      mBlendA   { BlendMode::Add };
  };



  struct ShaderNameString;
  struct ShaderNameStringView;

  struct ShaderNameStringView : public StringView
  {
    ShaderNameStringView() = default;
    ShaderNameStringView( const char* );// , Validation = DefaultValidation);
    ShaderNameStringView( const StringView& );// , Validation = DefaultValidation );
    ShaderNameStringView( const ShaderNameString& );
  };

  struct ShaderNameString : public String
  {
    ShaderNameString() = default;
    ShaderNameString( const StringView& );
  };

  // this exists so that other files can forward declare VertexDeclarations/... as a struct,
  // and not a typedef of FixedVector
  template< typename T, int N >
  struct FixedVectorWrapper
  {
    FixedVectorWrapper() = default;
    FixedVectorWrapper( const T& t ) { mElements = { t }; }
    //FixedVectorWrapper( std::initializer_list< T > ts ) : mElements{ ts } {}
    void push_back( const T& t )   { mElements.push_back( t ); }
    auto begin() const             { return mElements.begin(); } 
    auto end() const               { return mElements.end(); } 
    auto size() const              { return mElements.size(); } 
    auto empty() const             { return mElements.empty(); } 
    void clear()                   { mElements.clear(); }
    auto operator[]( int i ) const { return mElements[ i ]; }
    FixedVector< T, N > mElements;
  };

  struct VertexDeclarations : public FixedVectorWrapper< VertexDeclaration, 10 > {};
  struct FramebufferTextures : public FixedVectorWrapper< TextureHandle, 10 > {};
  struct DrawCallSamplers : public FixedVectorWrapper< SamplerStateHandle, 4 > {};
  struct DrawCallTextures : public FixedVectorWrapper< TextureHandle, 5 > {};
  struct ConstantBuffers : public FixedVectorWrapper< ConstantBufferHandle, 10 > {};

  struct DepthState
  {
    bool      mDepthTest { false };
    bool      mDepthWrite { false };
    DepthFunc mDepthFunc { ( DepthFunc )0 };
  };

  struct TexSpec
  {
    Image       mImage;
    int         mPitch                  {}; // byte count between texel rows
    const void* mImageBytes             {};
    const void* mImageBytesCubemap[ 6 ] {};
    Binding     mBinding                { Binding::None };
    Access      mAccess                 { Access::Default };
    CPUAccess   mCpuAccess              { CPUAccess::None };
  };

  struct TexUpdate
  {
    Image       mSrc;
    int         mDstX     {}; // column indexes, increases to the right
    int         mDstY     {}; // row indexes, increases downwards
    const void* mSrcBytes {};
    int         mPitch    {}; // byte count between pixel rows
  };

  struct RasterizerState
  {
    FillMode mFillMode                  { ( FillMode )0 };
    CullMode mCullMode                  { ( CullMode )0 };
    bool     mFrontCounterClockwise     { true };
    bool     mScissor                   { false };
    bool     mMultisample               { false };
    bool     mConservativeRasterization { false };
  };

  struct SamplerState
  {
    AddressMode mU       { ( AddressMode )0 };
    AddressMode mV       { ( AddressMode )0 };
    AddressMode mW       { ( AddressMode )0 };
    Comparison  mCompare { ( Comparison )0 };
    Filter      mFilter  { ( Filter )0 };
  };

  struct InProj  { float mNear, mFar; };
  struct OutProj { float mA, mB; };

  void                             Init( Errors& );
  void                             Uninit();

  // Executes render commands from the render frame and presents the swap chain
  void                             RenderFrame( Errors& );

  // only used when the platform thread is done
  void                             RenderFinish();

  // Call at the end of the logic thread, swaps the recorded submit frame with the render frame
  // for the renderer to render. Also does deferred handle stuff
  void                             SubmitFrame();

  void                             SubmitFinish();

  void*                            SubmitAlloc( int byteCount );
  const void*                      SubmitAlloc( const void* bytes, int byteCount );
  StringView                       SubmitAlloc( const StringView& );
  ViewHandle                       CreateView();
  void                             DestroyView( ViewHandle );
  ShaderHandle                     CreateShader( const ShaderNameStringView&, const StackFrame& );
  ConstantBufferHandle             CreateConstantBuffer( const char* name,
                                                         int mByteCount,
                                                         const StackFrame& );
  MagicBufferHandle                CreateMagicBuffer( int byteCount,
                                                      const void* mOptionalInitialBytes,
                                                      int stride,
                                                      Binding,
                                                      Access,
                                                      const StackFrame& );
  VertexBufferHandle               CreateVertexBuffer( int byteCount,
                                                       const void* optionalInitialBytes,
                                                       int stride,
                                                       Access,
                                                       const StackFrame& );
  IndexBufferHandle                CreateIndexBuffer( int byteCount,
                                                      const void* optionalInitialBytes,
                                                      Access,
                                                      const Format&,
                                                      const StackFrame& );
  TextureHandle                    CreateTexture( const TexSpec&, const StackFrame& );
  FramebufferHandle                CreateFramebufferForRenderToTexture( const FramebufferTextures&, const StackFrame& );
  FramebufferHandle                CreateFramebufferForWindow( const void* nativeWindowHandle,
                                                               int width,
                                                               int weight,
                                                               const StackFrame& );
  BlendStateHandle                 CreateBlendState( const BlendState&, const StackFrame& );
  RasterizerStateHandle            CreateRasterizerState( const RasterizerState&, const StackFrame& );
  SamplerStateHandle               CreateSamplerState( const SamplerState&, const StackFrame& );
  DepthStateHandle                 CreateDepthState( const DepthState&, const StackFrame& );
  VertexFormatHandle               CreateVertexFormat( const VertexDeclarations&, ShaderHandle, const StackFrame& );

  void                             DestroyVertexBuffer( VertexBufferHandle, const StackFrame& );
  void                             DestroyIndexBuffer( IndexBufferHandle, const StackFrame& );
  void                             DestroyTexture( TextureHandle, const StackFrame& );
  void                             DestroyMagicBuffer( MagicBufferHandle, const StackFrame& );
  void                             DestroyFramebuffer( FramebufferHandle, const StackFrame& );
  void                             DestroyShader( ShaderHandle, const StackFrame& );
  void                             DestroyVertexFormat( VertexFormatHandle, const StackFrame& );
  void                             DestroyConstantBuffer( ConstantBufferHandle, const StackFrame& );
  void                             DestroyDepthState( DepthStateHandle, const StackFrame& );
  void                             DestroyBlendState( BlendStateHandle, const StackFrame& );
  void                             DestroyRasterizerState( RasterizerStateHandle, const StackFrame& );
  void                             DestroySamplerState( SamplerStateHandle, const StackFrame& );

  void                             UpdateTextureRegion( TextureHandle,
                                                        const TexUpdate&,
                                                        const StackFrame& );
  void                             UpdateVertexBuffer( VertexBufferHandle,
                                                       const void*,
                                                       int,
                                                       const StackFrame& );
  void                             UpdateIndexBuffer( IndexBufferHandle,
                                                      const void*,
                                                      int,
                                                      const StackFrame& );

  void                             UpdateConstantBuffer( ConstantBufferHandle,
                                                         const void*,
                                                         int,
                                                         const StackFrame& );

  void                             ResizeFramebuffer( FramebufferHandle,
                                                      int w,
                                                      int h,
                                                      const StackFrame& );

  void                             SetViewFramebuffer( ViewHandle, FramebufferHandle );
  void                             SetViewScissorRect( ViewHandle, const ScissorRect& );
  void                             SetViewport( ViewHandle, const Viewport& );
  void                             SetIndexBuffer( IndexBufferHandle, int iStart, int count );
  void                             SetVertexBuffer( VertexBufferHandle, int iStart, int count );
  void                             SetBlendState( BlendStateHandle );
  void                             SetRasterizerState( RasterizerStateHandle );
  void                             SetSamplerState( const DrawCallSamplers& );
  void                             SetDepthState( DepthStateHandle );
  void                             SetVertexFormat( VertexFormatHandle );
  void                             SetShader( ShaderHandle );
  void                             SetTexture( const DrawCallTextures& );
  void                             SetPrimitiveTopology( PrimitiveTopology );

  //                               hmm.jpg
  //                               this shouldnt be PixelShader, it can be used in any shader?
  void                             SetPixelShaderUnorderedAccessView( TextureHandle, int );
  void                             SetPixelShaderUnorderedAccessView( MagicBufferHandle, int );

  void                             SetBreakpointWhenThisFrameIsRendered();
  void                             SetBreakpointWhenNextDrawCallIsExecuted();
  void                             Submit( ViewHandle, const StackFrame& );
  OutProj                          GetPerspectiveProjectionAB(InProj);
  void                             BeginGroup( const StringView&, const StackFrame& );
  void                             EndGroup( const StackFrame& );

  void                             SetRenderObjectDebugName( FramebufferHandle,     const StringView& );
  void                             SetRenderObjectDebugName( TextureHandle,         const StringView& );
  void                             SetRenderObjectDebugName( MagicBufferHandle,     const StringView& );
  void                             SetRenderObjectDebugName( VertexBufferHandle,    const StringView& );
  void                             SetRenderObjectDebugName( IndexBufferHandle,     const StringView& );
  void                             SetRenderObjectDebugName( RasterizerStateHandle, const StringView& );
  void                             SetRenderObjectDebugName( BlendStateHandle,      const StringView& );
  void                             SetRenderObjectDebugName( DepthStateHandle,      const StringView& );
  void                             SetRenderObjectDebugName( VertexFormatHandle,    const StringView& );
  void                             SetRenderObjectDebugName( ConstantBufferHandle,  const StringView& );
  void                             SetRenderObjectDebugName( SamplerStateHandle,    const StringView& );

  enum class RendererAPI
  {
    Vulkan,
    OpenGl4,
    DirectX11,
    DirectX12,
    Count,
  };

  struct Renderer;
  using RendererFactory = Renderer * ( * )( );
  RendererFactory  GetRendererFactory( RendererAPI );

  void             SetRendererFactory( RendererAPI, RendererFactory );

  template< typename T>
  void             SetRendererFactory( RendererAPI api )
  {
    const RendererFactory factory { []()->Renderer* { return TAC_NEW T }; };
    SetRendererFactory( api, factory );
  }

  const char*      ToString( RendererAPI );

} // namespace Tac::Render

#define TAC_RENDER_GROUP_BLOCK( text )                             \
      Tac::Render::BeginGroup( text, TAC_STACK_FRAME );            \
      TAC_ON_DESTRUCT( Tac::Render::EndGroup( TAC_STACK_FRAME ) );
