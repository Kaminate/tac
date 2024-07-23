#pragma once

#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/error/tac_stack_frame.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/memory/tac_smart_ptr.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/tac_ints.h"

// i dont like how the compiler shows it as IHandle<3> instead of like IHandle<kTexture>
// also you cant fwd decalre
#define TAC_IS_IHANDLE_TEMPLATE() 0

namespace Tac{ struct Errors; }
namespace Tac::FileSys{ struct Path; }
namespace Tac::Render
{
  struct IContext;
  struct IDevice;

#if TAC_IS_IHANDLE_TEMPLATE()
  enum class HandleType
  {
    kSwapChain,
    kPipeline,
    kProgram,
    kBuffer,
    kTexture,
    kSampler,
  };
#endif

#if TAC_IS_IHANDLE_TEMPLATE()
  template< HandleType T > 
#endif
  struct IHandle
  {
    IHandle( int i = -1 ) : mIndex( i ) {}
    int GetIndex() const { return mIndex; }
    bool IsValid() const { return mIndex != -1; }
  private:
    int mIndex;
  };

#if TAC_IS_IHANDLE_TEMPLATE()
  using SwapChainHandle = IHandle< HandleType::kSwapChain >;
  using PipelineHandle  = IHandle< HandleType::kPipeline >;
  using ProgramHandle   = IHandle< HandleType::kProgram >;
  using BufferHandle    = IHandle< HandleType::kBuffer >;
  using TextureHandle   = IHandle< HandleType::kTexture >;
  using SamplerHandle   = IHandle< HandleType::kSampler >;
#else
  struct SwapChainHandle : public IHandle { SwapChainHandle ( int i = -1 ) : IHandle{ i } {} };
  struct PipelineHandle  : public IHandle { PipelineHandle  ( int i = -1 ) : IHandle( i ) {} };
  struct ProgramHandle   : public IHandle { ProgramHandle   ( int i = -1 ) : IHandle{ i } {} };
  struct BufferHandle    : public IHandle { BufferHandle    ( int i = -1 ) : IHandle{ i } {} };
  struct TextureHandle   : public IHandle { TextureHandle   ( int i = -1 ) : IHandle{ i } {} };
  struct SamplerHandle   : public IHandle { SamplerHandle   ( int i = -1 ) : IHandle{ i } {} };
#endif

  // Used for Textures and Index Buffers and Structured Vertex Buffers
  enum class TexFmt
  {
    kUnknown = 0,
    kD24S8,
    kRGBA16F,
    kR8_unorm,
    kR16_uint,
    kR32_uint,
    kR32_float,
    kRGBA8_unorm,
    kRGBA8_unorm_srgb,
  };

  int GetTexFmtSize( TexFmt );

  // -----------------------------------------------------------------------------------------------

  // vvv can/should this/some of section be deleted?

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
    Count,
  };

  enum class DepthFunc
  {
    Less,
    LessOrEqual,
  };

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


  struct FormatElement
  {
    static FormatElement GetFloat();

    int          mPerElementByteCount {};
    GraphicsType mPerElementDataType { GraphicsType::unknown };
  };


  // Used so the gpu can translate from cpu types to gpu types
  struct VertexAttributeFormat
  {
    int                          CalculateTotalByteCount() const;
    static VertexAttributeFormat FromElements( FormatElement, int = 1 );
    static VertexAttributeFormat GetFloat();
    static VertexAttributeFormat GetVector2();
    static VertexAttributeFormat GetVector3();
    static VertexAttributeFormat GetVector4();

    int           mElementCount        {};
    int           mPerElementByteCount {};
    GraphicsType  mPerElementDataType  { GraphicsType::unknown };

  };

  struct VertexDeclaration
  {
    Attribute             mAttribute         { Attribute::Count };
    VertexAttributeFormat mFormat            {};

    //                    Offset of the variable from the vertex buffer
    //                    ie: TAC_OFFSET_OF( MyVertexType, mPosition)
    int                   mAlignedByteOffset {};
  };

  struct VertexDeclarations : public FixedVector< VertexDeclaration, 10 > {};

  // $$$ Should this still be called an "Image", since the data parameter was removed?
  struct Image
  {
    int    mWidth   {};
    int    mHeight  {};
    int    mDepth   {};
    TexFmt mFormat  { TexFmt::kUnknown };

    // Note that byte data should be passed as a separate argument,
    // and not as a member of this class
  };

  struct BlendState
  {
    BlendConstants mSrcRGB   { BlendConstants::One };
    BlendConstants mDstRGB   { BlendConstants::OneMinusSrcA };
    BlendMode      mBlendRGB { BlendMode::Add };
    BlendConstants mSrcA     { BlendConstants::One };
    BlendConstants mDstA     { BlendConstants::One };
    BlendMode      mBlendA   { BlendMode::Add };
  };

  struct DepthState
  {
    bool      mDepthTest  {};
    bool      mDepthWrite {};
    DepthFunc mDepthFunc  { DepthFunc::Less };
  };

  struct RasterizerState
  {
    FillMode mFillMode              { FillMode::Solid };
    CullMode mCullMode              { CullMode::None };
    bool     mFrontCounterClockwise { true };
    bool     mMultisample           {};
  };


  // ^^^

  // -----------------------------------------------------------------------------------------------

  enum class PrimitiveTopology
  {
    Unknown = 0,
    PointList,
    TriangleList,
    LineList,
  };

  //struct PipelineBindDesc
  //{
  //};

  struct SwapChainParams
  {
    const void* mNWH     {};
    v2i         mSize    {};
    TexFmt      mColorFmt{};
    TexFmt      mDepthFmt{};
  };

  enum class Usage
  {
    // D3D11_USAGE_DEFAULT / GL_DYNAMIC_DRAW / Default heap ( D3D12 )
    Default = 0,

    // D3D11_USAGE_IMMUTABLE / GL_STATIC_DRAW / Default Heap ( D3D12 )
    Static,

    // D3D11_USAGE_DYNAMIC / GL_STREAM_DRAW / Upload Heap ( D3D12 )
    Dynamic,

    Staging,
  };

  enum class CPUAccess
  {
    None = 0b00,
    Read = 0b01, // can be mapped for cpu reading
    Write = 0b10 // can be mapped for cpu writing
  };

  enum class Binding
  {
    None            = 0b00000000,
    ShaderResource  = 0b00000001, // SRV (buffer, texture, ... )
    RenderTarget    = 0b00000010, // RTV
    DepthStencil    = 0b00000100, // DSV
    UnorderedAccess = 0b00001000, // UAV (rwbuffer, rwstructuredbuffer, rwtexture, append/consume)
    ConstantBuffer  = 0b00010000, // CBV
    VertexBuffer    = 0b00100000, // IA vtx buf
    IndexBuffer     = 0b01000000, // IA idx buf
  };

  Binding operator | ( Binding, Binding );
  Binding operator & ( Binding, Binding );

  struct CreateTextureParams
  {
    struct Subresource
    {
      const void* mBytes{};
      int         mPitch{}; // byte count between texel rows (aka stride)
    };

    using Subresources = Span< const Subresource >;

    struct CubemapFace
    {
      Subresources mSubresource;
    };

    using CubemapFaces = Array< CubemapFace, 6 >;

    Image        mImage;
    int          mMipCount{};
    Subresources mSubresources;
    CubemapFaces mCubemapFaces;

    Binding      mBinding                { Binding::None };
    Usage        mUsage                  { Usage::Default };

    //           Whether the cpu can read or write to the resource after it's been mapped
    CPUAccess    mCpuAccess              { CPUAccess::None };
    StringView   mOptionalName           {};
    StackFrame   mStackFrame             {};
  };

  enum class GpuBufferMode
  {
    kUndefined = 0,
    kFormatted,
    kStructured,
    kByteAddress,
  };

  struct CreateBufferParams
  {
    int           mByteCount     {};
    const void*   mBytes         {};
    int           mStride        {}; // used in creating the SRV and used for the input layout
    Usage         mUsage         { Usage::Default };
    Binding       mBinding       { Binding::None };
    CPUAccess     mCpuAccess     { CPUAccess::None };
    GpuBufferMode mGpuBufferMode { GpuBufferMode::kUndefined };

    //            Used if the GpuBufferMode is kFormatted, and for index buffers
    TexFmt        mGpuBufferFmt  { TexFmt::kUnknown };
    StringView    mOptionalName  {};
    StackFrame    mStackFrame    {};
  };

  struct UpdateTextureParams
  {
    Image                             mSrcImage;
    CreateTextureParams::Subresources mSrcSubresource;
    int                               mDstSubresourceIndex{};
    v2i                               mDstPos{};
  };

  struct ProgramParams
  {
    String     mFileStem;
    StackFrame mStackFrame;
  };

  struct PipelineParams
  {
    ProgramHandle            mProgram;
    BlendState               mBlendState;
    DepthState               mDepthState;
    RasterizerState          mRasterizerState;
    FixedVector< TexFmt, 8 > mRTVColorFmts;
    TexFmt                   mDSVDepthFmt       { TexFmt::kUnknown };
    VertexDeclarations       mVtxDecls;
    PrimitiveTopology        mPrimitiveTopology { PrimitiveTopology::TriangleList };
    String                   mName;
    StackFrame               mStackFrame;
  };

  // i think like a view could be a higher order construct, like in Tac::Space


  struct UpdateBufferParams
  {
    const void*  mSrcBytes;
    int          mSrcByteCount;
    int          mDstByteOffset;
  };

  struct Targets
  {
    TextureHandle mColors[ 8 ];
    TextureHandle mDepth;
  };

  struct DrawArgs
  {
    // ------------------------------------------------ //
    // No index buffer bound ( vertex buffer optional ) //
    // ------------------------------------------------ //
    int mVertexCount   {};
    int mStartVertex   {};

    // ---------------------------------------------- //
    // Index buffer bound ( vertex buffer optional )  //
    // ---------------------------------------------- //
    int mIndexCount    {};
    int mStartIndex    {};
  };

  struct IShaderVar
  {
    virtual void SetBuffer( BufferHandle ) {};
    virtual void SetTexture( TextureHandle ) {};
    virtual void SetSampler( SamplerHandle ) {};
    virtual void SetBufferAtIndex( int, BufferHandle ) {};
    virtual void SetTextureAtIndex( int, TextureHandle ) {};
    virtual void SetSamplerAtIndex( int, SamplerHandle ) {};
  };

  struct CreateSamplerParams
  {
    Filter     mFilter;
    StringView mName;
  };

  struct IContext
  {
    struct Scope
    {
      Scope( IContext* = nullptr );
      ~Scope();
      IContext* operator ->();
      IContext* GetContext() { return mContext; }
      operator IContext* ( ) { return mContext; }
    private:
      IContext* mContext{};
    };

    virtual void SetViewport( v2i ) {}
    virtual void SetScissor( v2i ) {}
    virtual void SetRenderTargets( Targets ) {}
    virtual void SetPrimitiveTopology( PrimitiveTopology ) {};
    virtual void SetPipeline( PipelineHandle ) {}
    virtual void SetSynchronous() {}

    // represents a region of time ( from the call till the end of scope  StringView )
    virtual void DebugEventBegin( StringView ) {}
    virtual void DebugEventEnd() {}

    // represents a single point in time
    virtual void DebugMarker( StringView ) {}

    virtual void ClearColor( TextureHandle, v4 ) {}
    virtual void ClearDepth( TextureHandle, float ) {}

    virtual void SetVertexBuffer( BufferHandle ) {}
    virtual void SetIndexBuffer( BufferHandle ) {}

    //                                       | hack?/todo? this makes it so that i allocate the dyn buffer
    //                                       | once per frame in this function, but then i have to group all
    //                                       | the updates together in a span.
    //                                       |
    //                                       | it would be nicer to just if(first call this frame){allocate buf}
    //                                       v then i can call updatebuffer from multiple places.
    virtual void UpdateBuffer( BufferHandle, Span< const UpdateBufferParams >, Errors& ) {}

    virtual void UpdateTexture( TextureHandle, UpdateTextureParams, Errors& ) {}

    virtual void Draw( DrawArgs ) {}
    virtual void Execute( Errors& ) {}
    virtual void CommitShaderVariables() {}

  protected:
    virtual void Retire() = 0;
  };

  struct RenderApi
  {
    struct InitParams
    {
      int                   mMaxGPUFrameCount { 2 };
      const FileSys::Path&  mShaderOutputPath;
    };

    static void Init( InitParams, Errors& );
    static void Uninit();
    static int              GetMaxGPUFrameCount();
    static FileSys::Path    GetShaderOutputPath();
    static IDevice*         GetRenderDevice();
    static void             SetRenderDevice( IDevice* );
  };

  struct NDCAttribs
  {
    // (-1, 1) for opengl, (0, 1) for directx
    float mMinZ;
    float mMaxZ;
  };

  struct IDevice
  {
    struct Info
    {
      NDCAttribs mNDCAttribs;
    };

    virtual void            Init( Errors& )                                  {};
    virtual void            Update( Errors& )                                {};

    virtual Info            GetInfo() const                                  { return {}; }

    virtual PipelineHandle  CreatePipeline( PipelineParams, Errors& )        {}
    virtual IShaderVar*     GetShaderVariable( PipelineHandle, StringView )  { return {}; }
    virtual void            DestroyPipeline( PipelineHandle )                {}

    virtual ProgramHandle   CreateProgram( ProgramParams, Errors& )          {}
    virtual void            DestroyProgram( ProgramHandle )                  {}

    virtual SamplerHandle   CreateSampler( CreateSamplerParams )             {}
    virtual void            DestroySampler( SamplerHandle )                  {}

    virtual SwapChainHandle CreateSwapChain( SwapChainParams, Errors& )      {}
    virtual void            ResizeSwapChain( SwapChainHandle, v2i, Errors& ) {}
    virtual SwapChainParams GetSwapChainParams( SwapChainHandle )            { return {}; }
    virtual void            DestroySwapChain( SwapChainHandle )              {}
    virtual TextureHandle   GetSwapChainCurrentColor( SwapChainHandle )      { return {}; }
    virtual TextureHandle   GetSwapChainDepth( SwapChainHandle )             { return {}; }
    virtual void            Present( SwapChainHandle, Errors& )              {};

    virtual BufferHandle    CreateBuffer( CreateBufferParams, Errors& )      {}
    virtual void            DestroyBuffer( BufferHandle )                    {}

    virtual TextureHandle   CreateTexture( CreateTextureParams, Errors& )    {}
    virtual void            DestroyTexture( TextureHandle )                  {}

    virtual IContext::Scope CreateRenderContext( Errors& )                   { return {}; }
  };
}

#define TAC_RENDER_GROUP_BLOCK( ctx, str ) ctx->DebugEventBegin( str ); TAC_ON_DESTRUCT( ctx->DebugEventEnd() )
