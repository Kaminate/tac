#include "tac_render_handle.h" //self-inc

#include "tac-std-lib/error/tac_assert.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  IHandle::IHandle( int i ) : mIndex( i ) {}
  int IHandle::GetIndex() const { return mIndex; }
  bool IHandle::IsValid() const { return mIndex != -1; }

  // -----------------------------------------------------------------------------------------------

  ResourceHandle::ResourceHandle( HandleType t, IHandle h ) : IHandle( h ), mHandleType{ t } {}
  ResourceHandle::ResourceHandle( SwapChainHandle h ) : ResourceHandle( HandleType::kSwapChain, h ) {}
  ResourceHandle::ResourceHandle( PipelineHandle h )  : ResourceHandle( HandleType::kPipeline,  h ) {}
  ResourceHandle::ResourceHandle( ProgramHandle h )   : ResourceHandle( HandleType::kProgram,   h ) {}
  ResourceHandle::ResourceHandle( BufferHandle h )    : ResourceHandle( HandleType::kBuffer,    h ) {}
  ResourceHandle::ResourceHandle( TextureHandle h )   : ResourceHandle( HandleType::kTexture,   h ) {}
  ResourceHandle::ResourceHandle( SamplerHandle h )   : ResourceHandle( HandleType::kSampler,   h ) {}
  bool ResourceHandle::IsSwapChain() const { return mHandleType == HandleType::kSwapChain; }
  bool ResourceHandle::IsPipeline() const  { return mHandleType == HandleType::kPipeline;  }
  bool ResourceHandle::IsProgram() const   { return mHandleType == HandleType::kProgram;   }
  bool ResourceHandle::IsBuffer() const    { return mHandleType == HandleType::kBuffer;    }
  bool ResourceHandle::IsTexture() const   { return mHandleType == HandleType::kTexture;   }
  bool ResourceHandle::IsSampler() const   { return mHandleType == HandleType::kSampler;   }
  SwapChainHandle ResourceHandle::GetSwapChainHandle() const { TAC_ASSERT( IsSwapChain() ); return SwapChainHandle( GetIndex() ); }
  PipelineHandle  ResourceHandle::GetPipelineHandle() const  { TAC_ASSERT( IsPipeline() );  return PipelineHandle( GetIndex() ); }
  ProgramHandle   ResourceHandle::GetProgramHandle() const   { TAC_ASSERT( IsProgram() );   return ProgramHandle( GetIndex() ); }
  BufferHandle    ResourceHandle::GetBufferHandle() const    { TAC_ASSERT( IsBuffer() );    return BufferHandle( GetIndex() ); }
  TextureHandle   ResourceHandle::GetTextureHandle() const   { TAC_ASSERT( IsTexture() );   return TextureHandle( GetIndex() ); }
  SamplerHandle   ResourceHandle::GetSamplerHandle() const   { TAC_ASSERT( IsSampler() );   return SamplerHandle( GetIndex() ); }

 // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render
