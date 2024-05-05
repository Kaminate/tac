#pragma once

#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/error/tac_stack_frame.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/memory/tac_smart_ptr.h"

namespace Tac{ struct Errors; }
namespace Tac::Filesystem{ struct Path; }
namespace Tac::Render
{
  struct IContext;
  struct IDevice;

  struct IHandle
  {
    IHandle( int );
    int GetIndex() const;
    bool IsValid() const;
  private:
    int mIndex;
  };

  struct SwapChainHandle : public IHandle { SwapChainHandle ( int i = -1 ) : IHandle{ i } {} };
  struct PipelineHandle  : public IHandle { PipelineHandle  ( int i = -1 ) : IHandle( i ) {} };
  struct ProgramHandle   : public IHandle { ProgramHandle   ( int i = -1 ) : IHandle{ i } {} };
  struct BufferHandle    : public IHandle { BufferHandle    ( int i = -1 ) : IHandle{ i } {} };
  struct TextureHandle   : public IHandle { TextureHandle   ( int i = -1 ) : IHandle{ i } {} };

  //enum RasterizerType
  //{
  //};

  //enum BlendType
  //{
  //};

  // this is like... inferior to Render::Format in almost every way, except for specifying
  // typeless formats and depth stencil formats, and compressed formats, afaik
  // maybe i can split them into a single class that contains both?
  enum TexFmt
  {
    kUnknown = 0,
    kD24S8,
    kRGBA16F,
  };


  //enum DepthStencilType
  //{
  //};


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
    int          mPerElementByteCount {};
    GraphicsType mPerElementDataType { GraphicsType::unknown };

    static const FormatElement sFloat;
  };

  // Used so the gpu can translate from cpu types to gpu types
  struct Format
  {
    int           CalculateTotalByteCount() const;
    static Format FromElements( FormatElement, int = 1 );

    int           mElementCount        {};
    int           mPerElementByteCount {};
    GraphicsType  mPerElementDataType  { GraphicsType::unknown };

    static const Format sfloat;
    static const Format sv2;
    static const Format sv3;
    static const Format sv4;
  };

  struct VertexDeclaration
  {
    Attribute mAttribute         { Attribute::Count };
    Format    mFormat            {};

    //        Offset of the variable from the vertex buffer
    //        ie: OffsetOf( MyVertexType, mPosition)
    int       mAlignedByteOffset {};
  };

  struct VertexDeclarations : public FixedVector< VertexDeclaration, 10 > {};

  // $$$ Should this still be called an "Image", since the data parameter was removed?
  struct Image
  {
    int    mWidth  {};
    int    mHeight {};
    int    mDepth  {};
    Format mFormat;
    TexFmt mFormat2{ kUnknown };

    // Note that byte data should be passed as a separate argument,
    // and not as a member of this class
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
  };

  Binding operator | ( Binding, Binding );
  Binding operator & ( Binding, Binding );

  struct CreateTextureParams
  {
    Image       mImage;
    int         mPitch                  {}; // byte count between texel rows
    const void* mImageBytes             {};
    const void* mImageBytesCubemap[ 6 ] {};
    Binding     mBinding                { Binding::None };
    Usage       mAccess                 { Usage::Default }; // TODO: rename as Usage
    CPUAccess   mCpuAccess              { CPUAccess::None };
    StringView  mOptionalName           {};
    StackFrame  mStackFrame             {};
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
    int           mStride        {}; // used in creating the SRV
    Usage         mUsage         { Usage::Default }; // TODO: rename to `mUsage`
    Binding       mBinding       { Binding::None };
    CPUAccess     mCpuAccess     { CPUAccess::None };
    GpuBufferMode mGpuBufferMode { GpuBufferMode::kUndefined };
    Format        mGpuBufferFmt  {}; // used if the GpuBufferMode is kFormatted
    StringView    mOptionalName  {};
    StackFrame    mStackFrame    {};
  };

  struct UpdateTextureParams
  {
  };

  struct ProgramParams
  {
    StringView mFileStem;
    StackFrame mStackFrame;
  };

  struct PipelineParams
  {
    ProgramHandle            mProgram;
    //RasterizerType         mRasterizer;
    //BlendType              mBlend;
    //DepthStencilType       mDepthStencilType;
    //SwapChainHandle               mRenderTarget;
    FixedVector< TexFmt, 8 > mRTVColorFmts;
    TexFmt                   mDSVDepthFmt { TexFmt::kUnknown };
    VertexDeclarations       mVtxDecls;
    // root sig? <-- parse using dx reflection
    //PrimTopology     mPrimTopo;
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
    int mVertexCount   {};
  };

  struct IContext
  {
    struct Scope
    {
      Scope( IContext* );
      ~Scope();
      IContext* operator ->();
      IContext* GetContext() { return mContext; }
    private:
      IContext* mContext{};
    };


    virtual void SetViewport( v2i ) {}
    virtual void SetScissor( v2i ) {}
    virtual void SetRenderTargets( Targets ) {}
    virtual void SetPipeline( PipelineHandle ) {}
    virtual void SetSynchronous() {}

    // represents a region of time ( from the call till the end of scope  StringView )
    virtual void DebugEventBegin( StringView ) {}
    virtual void DebugEventEnd() {}

    // represents a single point in time
    virtual void DebugMarker( StringView ) {}

    virtual void ClearColor( TextureHandle, v4 ) {}
    virtual void ClearDepth( TextureHandle, float ) {}
    virtual void UpdateTexture( TextureHandle, UpdateTextureParams, Errors& ) {}
    virtual void UpdateBuffer( BufferHandle, UpdateBufferParams, Errors& ) {}
    virtual void Draw( DrawArgs ) {};
    virtual void Execute( Errors& ) {}

  protected:
    virtual void Retire() = 0;
  };

  struct RenderApi
  {
    struct InitParams
    {
      int                     mMaxGPUFrameCount { 2 };
      const Filesystem::Path& mShaderOutputPath;
    };

    static void Init( InitParams, Errors& );
    static void Uninit();
    static int              GetMaxGPUFrameCount();
    static Filesystem::Path GetShaderOutputPath();
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

    virtual void Init( Errors& ) {};

    virtual Info            GetInfo() const { return {}; }

    virtual PipelineHandle  CreatePipeline( PipelineParams, Errors& ) {}
    virtual void            DestroyPipeline( PipelineHandle ) {}

    virtual ProgramHandle   CreateProgram( ProgramParams, Errors& ) {}
    virtual void            DestroyProgram( ProgramHandle ) {}

    virtual SwapChainHandle CreateSwapChain( SwapChainParams, Errors& ) {}
    virtual void            ResizeSwapChain( SwapChainHandle, v2i ) {}
    virtual SwapChainParams GetSwapChainParams( SwapChainHandle ) { return {}; }
    virtual void            DestroySwapChain( SwapChainHandle ) {}
    virtual TextureHandle   GetSwapChainCurrentColor( SwapChainHandle ) { return {}; }
    virtual TextureHandle   GetSwapChainDepth( SwapChainHandle ) { return {}; }

    virtual BufferHandle    CreateBuffer( CreateBufferParams, Errors& ) {}
    virtual void            DestroyBuffer( BufferHandle ) {}

    virtual TextureHandle   CreateTexture( CreateTextureParams, Errors& ) {}
    virtual void            DestroyTexture( TextureHandle ) {}

    virtual IContext::Scope CreateRenderContext( Errors& );
  };
}

