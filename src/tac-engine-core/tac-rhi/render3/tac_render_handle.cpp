#include "tac_render_handle.h" //self-inc

#include "tac-std-lib/error/tac_assert.h"

  auto Tac::Render::HandleTypeToString( HandleType t ) -> const char*
  {
    switch(t)
    {
    case HandleType::kBuffer:    return "Buffer";
    case HandleType::kSwapChain: return "SwapChain";
    case HandleType::kPipeline:  return "Pipeline";
    case HandleType::kProgram:   return "Program";
    case HandleType::kTexture:   return "Texture";
    case HandleType::kSampler:   return "Sampler";
    default: TAC_ASSERT_INVALID_CASE( t ); return "?";
    }
  }

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  IHandle::IHandle( int i ) : mIndex( i ) {}
  auto IHandle::GetIndex() const -> int{ return mIndex; }
  bool IHandle::IsValid() const { return mIndex != -1; }


  // -----------------------------------------------------------------------------------------------
  // -----------------------------------------------------------------------------------------------

  ResourceHandle::ResourceHandle( HandleType t, IHandle h ) : mHandle( h ), mHandleType{ t } {}
  ResourceHandle::ResourceHandle( SwapChainHandle h ) : ResourceHandle( HandleType::kSwapChain, h ) {}
  ResourceHandle::ResourceHandle( PipelineHandle h )  : ResourceHandle( HandleType::kPipeline,  h ) {}
  ResourceHandle::ResourceHandle( ProgramHandle h )   : ResourceHandle( HandleType::kProgram,   h ) {}
  ResourceHandle::ResourceHandle( BufferHandle h )    : ResourceHandle( HandleType::kBuffer,    h ) {}
  ResourceHandle::ResourceHandle( TextureHandle h )   : ResourceHandle( HandleType::kTexture,   h ) {}
  ResourceHandle::ResourceHandle( SamplerHandle h )   : ResourceHandle( HandleType::kSampler,   h ) {}
  ResourceHandle::operator SwapChainHandle() const { TAC_ASSERT( mHandleType == HandleType::kSwapChain ); return SwapChainHandle( mHandle.GetIndex() ); }
  ResourceHandle::operator PipelineHandle() const  { TAC_ASSERT( mHandleType == HandleType::kPipeline );  return PipelineHandle( mHandle.GetIndex() ); }
  ResourceHandle::operator ProgramHandle() const   { TAC_ASSERT( mHandleType == HandleType::kProgram );   return ProgramHandle( mHandle.GetIndex() ); }
  ResourceHandle::operator BufferHandle() const    { TAC_ASSERT( mHandleType == HandleType::kBuffer );    return BufferHandle( mHandle.GetIndex() ); }
  ResourceHandle::operator TextureHandle() const   { TAC_ASSERT( mHandleType == HandleType::kTexture );   return TextureHandle( mHandle.GetIndex() ); }
  ResourceHandle::operator SamplerHandle() const   { TAC_ASSERT( mHandleType == HandleType::kSampler );   return SamplerHandle( mHandle.GetIndex() ); }

 // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render
