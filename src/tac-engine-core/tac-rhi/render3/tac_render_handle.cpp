#include "tac_render_handle.h" //self-inc

#include "tac-std-lib/error/tac_assert.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  IHandle::IHandle( int i ) : mIndex( i ) {}
  int IHandle::GetIndex() const { return mIndex; }
  bool IHandle::IsValid() const { return mIndex != -1; }


  // -----------------------------------------------------------------------------------------------
  const char* HandleTypeToString( HandleType t )
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
  // -----------------------------------------------------------------------------------------------

  ResourceHandle::ResourceHandle( HandleType t, IHandle h ) : IHandle( h ), mHandleType{ t } {}
  ResourceHandle::ResourceHandle( SwapChainHandle h ) : ResourceHandle( HandleType::kSwapChain, h ) {}
  ResourceHandle::ResourceHandle( PipelineHandle h )  : ResourceHandle( HandleType::kPipeline,  h ) {}
  ResourceHandle::ResourceHandle( ProgramHandle h )   : ResourceHandle( HandleType::kProgram,   h ) {}
  ResourceHandle::ResourceHandle( BufferHandle h )    : ResourceHandle( HandleType::kBuffer,    h ) {}
  ResourceHandle::ResourceHandle( TextureHandle h )   : ResourceHandle( HandleType::kTexture,   h ) {}
  ResourceHandle::ResourceHandle( SamplerHandle h )   : ResourceHandle( HandleType::kSampler,   h ) {}
  ResourceHandle::operator SwapChainHandle() const { TAC_ASSERT( mHandleType == HandleType::kSwapChain ); return GetIndex(); }
  ResourceHandle::operator PipelineHandle() const  { TAC_ASSERT( mHandleType == HandleType::kPipeline );  return GetIndex(); }
  ResourceHandle::operator ProgramHandle() const   { TAC_ASSERT( mHandleType == HandleType::kProgram );   return GetIndex(); }
  ResourceHandle::operator BufferHandle() const    { TAC_ASSERT( mHandleType == HandleType::kBuffer );    return GetIndex(); }
  ResourceHandle::operator TextureHandle() const   { TAC_ASSERT( mHandleType == HandleType::kTexture );   return GetIndex(); }
  ResourceHandle::operator SamplerHandle() const   { TAC_ASSERT( mHandleType == HandleType::kSampler );   return GetIndex(); }
  HandleType ResourceHandle::GetHandleType() const { return mHandleType; }

 // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render
