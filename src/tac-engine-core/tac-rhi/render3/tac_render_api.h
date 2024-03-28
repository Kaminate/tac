#pragma once

#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/error/tac_stack_frame.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"

namespace Tac{ struct Errors; }
namespace Tac::Filesystem{ struct Path; }
namespace Tac::Render
{
  struct Handle
  {
    Handle( int );
    int GetIndex() const;
    bool IsValid() const;
  private:
    int mIndex;
  };

#if 0
#define TAC_DEFINE_HANDLE( T ) struct T : public Handle { T( int i = -1 ) : Handle{ i } {} }
  TAC_DEFINE_HANDLE( FBHandle );
  TAC_DEFINE_HANDLE( DynBufHandle );
#undef TAC_DEFINE_HANDLE
#else

  struct FBHandle       : public Handle { FBHandle( int i = -1 ) : Handle{ i } {} };
  struct DynBufHandle   : public Handle { DynBufHandle( int i = -1 ) : Handle{ i } {} };
  struct PipelineHandle : public Handle { PipelineHandle( int i = -1 ) : Handle( i ) {} };
  struct ProgramHandle  : public Handle { ProgramHandle( int i = -1 ) : Handle{ i } {} };
#endif


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

  struct ShaderProgramParams
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

  struct RenderApi
  {
    struct InitParams
    {
      int                     mMaxGPUFrameCount = 2;
      const Filesystem::Path& mShaderOutputPath;
    };

    struct UpdateDynBufParams
    {
      DynBufHandle mHandle;
      const void*  mSrcBytes;
      int          mSrcByteCount;
      int          mDstByteOffset;
    };

    static void Init( InitParams, Errors& );
    static int              GetMaxGPUFrameCount();
    static Filesystem::Path GetShaderOutputPath();

    static PipelineHandle CreateRenderPipeline( PipelineParams, Errors& );
    static void           DestroyRenderPipeline( PipelineHandle );

    static ProgramHandle  CreateShaderProgram( ShaderProgramParams, Errors& );
    static void           DestroyShaderProgram( ProgramHandle );

    static FBHandle       CreateFB( FrameBufferParams, Errors& );
    static void           ResizeFB( FBHandle, v2i );
    static TexFmt         GetFBFmt( FBHandle );
    static void           DestroyFB( FBHandle );

    static DynBufHandle   CreateDynBuf( int, StackFrame, Errors& );
    static void           UpdateDynBuf( UpdateDynBufParams );
    static void           DestroyDynBuf( DynBufHandle );
  };
}
