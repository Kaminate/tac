#pragma once

#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/error/tac_stack_frame.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/memory/tac_smart_ptr.h"

namespace Tac{ struct Errors; }
namespace Tac::Filesystem{ struct Path; }
namespace Tac::Render
{
  struct IContextBackend;
  struct IDevice;

  struct Handle
  {
    Handle( int );
    int GetIndex() const;
    bool IsValid() const;
  private:
    int mIndex;
  };

  struct FBHandle       : public Handle { FBHandle      ( int i = -1 ) : Handle{ i } {} };
  struct PipelineHandle : public Handle { PipelineHandle( int i = -1 ) : Handle( i ) {} };
  struct ProgramHandle  : public Handle { ProgramHandle ( int i = -1 ) : Handle{ i } {} };
  struct BufferHandle   : public Handle { BufferHandle  ( int i = -1 ) : Handle{ i } {} };
  struct TextureHandle  : public Handle { TextureHandle ( int i = -1 ) : Handle{ i } {} };

  //enum RasterizerType
  //{
  //};

  //enum BlendType
  //{
  //};

  enum TexFmt
  {
    kUnknown = 0,
    kD24S8,
    kRGBA16F,
  };


  //enum DepthStencilType
  //{
  //};


  //enum PrimTopology
  //{
  //};

  //struct PipelineBindDesc
  //{
  //};

  struct FrameBufferParams
  {
    const void* mNWH;
    v2i         mSize;
    TexFmt      mColorFmt;
  };

  struct CreateTextureParams
  {
  };

  struct UpdateTextureParams
  {
  };

  struct ProgramParams
  {
    StringView mFileStem;
  };

  struct PipelineParams
  {
    ProgramHandle            mProgram;
    //RasterizerType         mRasterizer;
    //BlendType              mBlend;
    //DepthStencilType       mDepthStencilType;
    //FBHandle               mRenderTarget;
    FixedVector< TexFmt, 8 > mRTVColorFmts;
    TexFmt                   mDSVDepthFmt = TexFmt::kUnknown;
    // root sig? <-- parse using dx reflection
    //PrimTopology     mPrimTopo;
  };

  // i think like a view could be a higher order construct, like in Tac::Space


  struct Context
  {
    ~Context();
    void SetViewport( v2i );
    void SetScissor( v2i );
    void SetRenderTarget( FBHandle );
    void Execute( Errors& );
    void ExecuteSynchronously( Errors& );

    // represents a region of time ( from the call till the end of scope )
    void DebugEvent( StringView ); 
    void DebugMarker( StringView ); // represents a single point in time

    IContextBackend* mContextBackend{};
  };

  struct CreateBufferParams
  {
    int mByteCount;
  };

  struct UpdateBufferParams
  {
    BufferHandle mHandle;
    const void*  mSrcBytes;
    int          mSrcByteCount;
    int          mDstByteOffset;
  };

  struct RenderApi
  {
    struct InitParams
    {
      int                     mMaxGPUFrameCount = 2;
      const Filesystem::Path& mShaderOutputPath;
    };

    static void Init( InitParams, Errors& );
    static int              GetMaxGPUFrameCount();
    static Filesystem::Path GetShaderOutputPath();
    static Context          CreateRenderContext( Errors& );
    static IDevice*         GetRenderDevice();
  };

  struct IDevice
  {
    virtual void Init( Errors& ) {};

    virtual PipelineHandle CreatePipeline( PipelineParams, Errors& ) {}
    virtual void           DestroyPipeline( PipelineHandle ) {}

    virtual ProgramHandle  CreateProgram( ProgramParams, Errors& ) {}
    virtual void           DestroyProgram( ProgramHandle ) {}

    virtual FBHandle       CreateFB( FrameBufferParams, Errors& ) {}
    virtual void           ResizeFB( FBHandle, v2i ) {}
    virtual TexFmt         GetFBFmt( FBHandle ) { return TexFmt::kUnknown; }
    virtual void           DestroyFB( FBHandle ) {}

    virtual BufferHandle   CreateBuffer( CreateBufferParams, Errors& ) {}
    virtual void           UpdateBuffer( UpdateBufferParams ) {}
    virtual void           DestroyBuffer( BufferHandle ) {}

    virtual TextureHandle  CreateTexture( CreateTextureParams, Errors& ) {}
    virtual void           UpdateTexture( UpdateTextureParams ) {}
    virtual void           DestroyTexture( TextureHandle ) {}

    virtual IContextBackend* CreateRenderContextBackend( Errors& ) { return {}; }
  };
}

