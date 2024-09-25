#include "tac_dx12_pipeline.h" // self-inc
#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-dx/dx12/sampler/tac_dx12_sampler_mgr.h"
#include "tac-dx/dx12/buffer/tac_dx12_buffer_mgr.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------

  DX12Pipeline::Variable::Variable( D3D12ProgramBindDesc binding )
  {
    mBinding = binding;
    mHandleIndexes.resize( binding.mBindCount, -1 );
  }

  void DX12Pipeline::Variable::SetBuffer( BufferHandle h )
  {
    TAC_ASSERT( mBinding.mType. IsBuffer() );
    SetElement( h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetTexture( TextureHandle h )
  {
    TAC_ASSERT( mBinding.mType.IsTexture() );
    SetElement( h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetBufferAtIndex( int i, BufferHandle h )
  {
    TAC_ASSERT( mBinding.mType.IsBuffer() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetTextureAtIndex( int i, TextureHandle h )
  {
    TAC_ASSERT( mBinding.mType.IsTexture() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetSamplerAtIndex( int i, SamplerHandle h )
  {
    TAC_ASSERT( mBinding.mType.IsSampler() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetSampler( SamplerHandle h )
  {
    TAC_ASSERT( mBinding.mType.IsSampler() );
    SetElement( h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetElement( int iHandle )
  {
    TAC_ASSERT( mHandleIndexes.size() == 1 );
    TAC_ASSERT( mBinding.mBindCount == 1 );
    mHandleIndexes[ 0 ] = iHandle;
  }

  void DX12Pipeline::Variable::SetArrayElement( int iArray, int iHandle )
  {
    TAC_ASSERT_INDEX( iArray, 1000 ); // sanity
    TAC_ASSERT( mBinding.mBindCount >= 0 ); // sanity

    // resize unbounded array
    if( !mBinding.mBindCount && iArray >= mHandleIndexes.size() )
      mHandleIndexes.resize( iArray + 1, -1 );

    mHandleIndexes[ iArray ] = iHandle;
  }

  StringView DX12Pipeline::Variable::GetName() const
  {
    return mBinding.mName;
  }

  Span< DX12Descriptor > DX12Pipeline::Variable::GetDescriptors(
    DX12TransitionHelper* transitionHelper,
    DX12TextureMgr* mTextureMgr,
    DX12SamplerMgr* mSamplerMgr,
    DX12BufferMgr* mBufferMgr ) const
  {
    const int n{ mHandleIndexes.size() };
    DX12Descriptor* dst{
      ( DX12Descriptor* )FrameMemoryAllocate( sizeof( DX12Descriptor ) * n ) };

    Span< DX12Descriptor > result( dst, n );

    for( int iHandle : mHandleIndexes )
    {
      DX12Descriptor descriptor{
        GetDescriptor( iHandle, transitionHelper, mTextureMgr, mSamplerMgr, mBufferMgr ) };
      TAC_ASSERT( descriptor.IsValid() );
      *dst++ = descriptor;
    }

    return result;
  }

  DX12Descriptor DX12Pipeline::Variable::GetDescriptor( int iHandle,
                                                        DX12TransitionHelper* transitionHelper,
                                                        DX12TextureMgr* textureMgr,
                                                        DX12SamplerMgr* samplerMgr,
                                                        DX12BufferMgr* bufferMgr ) const
  {
    const D3D12ProgramBindDesc binding{ mBinding };
    const D3D12ProgramBindType::Classification classification{ binding.mType.GetClassification() };

    switch( classification )
    {
    case D3D12ProgramBindType::kTextureSRV:
    {
      const TextureHandle textureHandle{ iHandle };
      DX12Texture* texture{ textureMgr->FindTexture( textureHandle ) };
      TAC_ASSERT( texture );
      textureMgr->TransitionResource( texture->mResource.Get(),
                                      &texture->mState,
                                      Render::Binding::ShaderResource,
                                      transitionHelper );
      return texture->mSRV.GetValue();
    }

    case D3D12ProgramBindType::kTextureUAV:
    {
      const TextureHandle textureHandle{ iHandle };
      DX12Texture* texture{ textureMgr->FindTexture( textureHandle ) };
      TAC_ASSERT( texture );
      textureMgr->TransitionResource( texture->mResource.Get(),
                                      &texture->mState,
                                      Render::Binding::UnorderedAccess,
                                      transitionHelper );
      TAC_ASSERT( texture->mUAV.HasValue() );
      TAC_ASSERT( texture->mState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
      return texture->mUAV.GetValue();
    }

    case D3D12ProgramBindType::kBufferSRV:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );
      TAC_ASSERT( buffer->mState & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      textureMgr->TransitionResource( buffer->mResource.Get(),
                                      &buffer->mState,
                                      Render::Binding::ShaderResource,
                                      transitionHelper );
      return buffer->mSRV.GetValue();
    }

    case D3D12ProgramBindType::kBufferUAV:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );
      TAC_ASSERT( buffer->mState & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      textureMgr->TransitionResource( buffer->mResource.Get(),
                                      &buffer->mState,
                                      Render::Binding::UnorderedAccess,
                                      transitionHelper );
      TAC_ASSERT( buffer->mState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
      return buffer->mUAV.GetValue();
    }

    case D3D12ProgramBindType::kConstantBuffer:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );
      TAC_ASSERT( buffer->mState & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      textureMgr->TransitionResource( buffer->mResource.Get(),
                                      &buffer->mState,
                                      Render::Binding::ConstantBuffer,
                                      transitionHelper );
      TAC_ASSERT( buffer->mState & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
      TAC_ASSERT_UNIMPLEMENTED; // ???
      return {};
    }

    case D3D12ProgramBindType::kSampler:
    {
      DX12Sampler* sampler{ samplerMgr->FindSampler( SamplerHandle{ iHandle } ) };
      TAC_ASSERT( sampler );
      return sampler->mDescriptor;
    }

    default: TAC_ASSERT_INVALID_CASE( classification ); return {};
    }
  }

  // -----------------------------------------------------------------------------------------------

  DX12Pipeline::Variables::Variables( const D3D12ProgramBindDescs& bindings )
  {
    for( const D3D12ProgramBindDesc& binding : bindings )
      mShaderVariables.push_back( DX12Pipeline::Variable( binding ) );
  }

  int DX12Pipeline::Variables::size() const
  {
    return mShaderVariables.size();
  }

  dynmc DX12Pipeline::Variable* DX12Pipeline::Variables::begin()
  {
    return mShaderVariables.begin();
  }

  dynmc DX12Pipeline::Variable* DX12Pipeline::Variables::end()
  {
    return mShaderVariables.end();
  }

  const DX12Pipeline::Variable* DX12Pipeline::Variables::begin() const
  {
    return mShaderVariables.begin();
  }

  const DX12Pipeline::Variable* DX12Pipeline::Variables::end() const
  {
    return mShaderVariables.end();
  }

  const DX12Pipeline::Variable* DX12Pipeline::Variables::data() const
  {
    return mShaderVariables.data();
  }

  dynmc DX12Pipeline::Variable* DX12Pipeline::Variables::data() dynmc
  {
    return mShaderVariables.data();
  }

  const DX12Pipeline::Variable& DX12Pipeline::Variables::operator[]( int i ) const
  {
    return mShaderVariables[ i ];
  }
  
  // -----------------------------------------------------------------------------------------------

  void DX12Pipeline::BindlessArray::SetBufferAtIndex( int i, BufferHandle h)
  {
    TAC_ASSERT( mType.IsBuffer() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::BindlessArray::SetTextureAtIndex( int i, TextureHandle h )
  {
    TAC_ASSERT( mType.IsTexture() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::BindlessArray::SetSamplerAtIndex( int i, SamplerHandle h )
  {
    TAC_ASSERT( mType.IsSampler() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::BindlessArray::SetArrayElement( int i, int iHandle )
  {
    TAC_ASSERT_INDEX( i, 1000 ); // sanity

    // resize unbounded array
    if( i>= mHandleIndexes.size() )
      mHandleIndexes.resize( i + 1, -1 );

    mHandleIndexes[ i ] = iHandle;
  }

  // -----------------------------------------------------------------------------------------------

  bool DX12Pipeline:: IsValid() const
  {
    return mPSO.Get() != nullptr;
  }

} // namespace Tac::Render

