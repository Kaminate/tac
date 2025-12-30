#include "tac_dx12_pipeline_bindless_array.h" // self-inc

#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  static auto GetHeapType( D3D12ProgramBindType type ) -> D3D12_DESCRIPTOR_HEAP_TYPE
  {
    if( type.IsBuffer() || type.IsTexture() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    if( type.IsSampler() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

    TAC_ASSERT_INVALID_CODE_PATH;
    return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
  }

  static auto GetClassification( IBindlessArray::Params params ) -> D3D12ProgramBindType::Classification
  {
    const bool isShaderResource{ params.mBinding & Binding::ShaderResource };
    const bool isBuffer{ params.mHandleType == HandleType::kBuffer };
    const bool isTexture{ params.mHandleType == HandleType::kTexture };

    if( isBuffer && isShaderResource )
      return D3D12ProgramBindType::Classification::kBufferSRV;

    if( isTexture && isShaderResource )
      return D3D12ProgramBindType::Classification::kTextureSRV;

    return D3D12ProgramBindType::Classification::kUnknown;
  }

  // -----------------------------------------------------------------------------------------------

  BindlessArray::BindlessArray( IBindlessArray::Params params ) : IBindlessArray( params )
  {
    const D3D12ProgramBindType::Classification classification{ GetClassification( params ) };
    TAC_ASSERT( classification != D3D12ProgramBindType::Classification::kUnknown );
    mProgramBindType = D3D12ProgramBindType( classification );
  }

  void BindlessArray::CheckType( ResourceHandle h )
  {
    if constexpr( kIsDebugMode )
    {

      const D3D12ProgramBindType bindType{ mProgramBindType };
      const HandleType handleType{ h.GetHandleType() };
      TAC_ASSERT( bindType.IsValid() );
      TAC_ASSERT( h.IsValid() );
      TAC_ASSERT( !bindType.IsBuffer() || ( handleType == HandleType::kBuffer ) );
      TAC_ASSERT( !bindType.IsTexture() || ( handleType == HandleType::kTexture ) );
      TAC_ASSERT( !bindType.IsSampler() || ( handleType == HandleType::kSampler ) );
    }
  }

  void BindlessArray::Commit( const CommitParams commitParams )
  {
    ( commitParams.mCommandList->*(
      commitParams.mIsCompute
      ? &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
      : &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable ) )
      ( commitParams.mRootParameterIndex, mDescriptorRegion.GetGPUHandle() );
  }

  void BindlessArray::SetFenceSignal( const FenceSignal fenceSignal )
  {
    mFenceSignal = fenceSignal;
  }

  void BindlessArray::Resize( const int newSize, Errors& errors )
  {
    const int oldSize{ mHandles.size() };
    TAC_ASSERT( mUnusedBindings.empty() );
    TAC_ASSERT( newSize > oldSize );
    mHandles.resize( newSize );

    // Allocating a new binding pops the back, so add in reverse order
    const int addedCount{ newSize - oldSize };
    for( int i{}; i < addedCount; ++i )
      mUnusedBindings.push_back( Binding{ newSize - i - 1 } );

    const D3D12_DESCRIPTOR_HEAP_TYPE heapType { GetHeapType( mProgramBindType ) };
    DX12Renderer& renderer                    { DX12Renderer::sRenderer };
    DX12DescriptorHeapMgr& heapMgr            { renderer.mDescriptorHeapMgr };
    DX12DescriptorHeap& heap                  { heapMgr.mGPUHeaps[ heapType ] };
    DX12DescriptorAllocator* regionMgr        { heap.GetGPURegionMgr() };

    if( mDescriptorRegion.IsValid() )
      mDescriptorRegion.Free( mFenceSignal );

    mDescriptorRegion = regionMgr->Alloc( newSize );
    for( int i{}; i < mHandles.size(); ++i )
    {
      const Binding binding( i );
      const IHandle handle{ mHandles[ i ] };
      if( handle.IsValid() )
      {
        TAC_CALL( CopyDescriptor( handle, binding, errors ) );
      }
    }
  }

  auto BindlessArray::Bind( ResourceHandle h, Errors& errors ) -> Binding
  {
    CheckType( h );

    if( mUnusedBindings.empty() )
    {
      const int oldSize{ mHandles.size() };
      const int newSize{ ( int )( ( oldSize + 2 ) * 1.5 ) };
      TAC_CALL_RET( Resize( newSize, errors ) );
    }

    const Binding binding{ mUnusedBindings.back() };
    mUnusedBindings.pop_back();
    mHandles[ binding.GetIndex() ] = h;

    TAC_CALL_RET( CopyDescriptor( h, binding, errors ) );

    return binding;
  }

  void BindlessArray::CopyDescriptor( IHandle h, Binding binding, Errors& errors )
  {

    DX12Renderer&   renderer   { DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    ID3D12Device*   device     { renderer.mDevice };

    const D3D12ProgramBindType::Classification classification{
      mProgramBindType.GetClassification() };
    const int iHandle{ h.GetIndex() };

    DX12Descriptor cpuDescriptor;
    DX12TransitionHelper transitionHelper;

    switch( classification )
    {
    case D3D12ProgramBindType::kTextureSRV:
    {
      const TextureHandle textureHandle{ iHandle };
      DX12Texture* texture{ textureMgr->FindTexture( textureHandle ) };
      TAC_ASSERT( texture );
      const DX12TransitionHelper::Params transitionParams
      {
        .mResource    { &texture->mResource },
        .mStateAfter  { D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE },
      };
      transitionHelper.Append( transitionParams );
      cpuDescriptor = texture->mSRV.GetValue();
    } break;

    case D3D12ProgramBindType::kBufferSRV:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );
      const DX12TransitionHelper::Params transitionParams
      {
        .mResource    { &buffer->mResource },
        .mStateAfter  { D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE },
      };
      transitionHelper.Append( transitionParams );
      cpuDescriptor = buffer->mSRV.GetValue();
    } break;
    default: TAC_ASSERT_INVALID_CASE( classification ); break;
    }

    if( !transitionHelper.empty() )
    {
      DX12ContextManager* contextManager{ &renderer.mContextManager };
      TAC_CALL( DX12Context* context{ contextManager->GetContext( errors ) } );
      context->SetSynchronous();
      IContext::Scope contextScope{ context };
      ID3D12GraphicsCommandList* commandList { context->GetCommandList() };
      transitionHelper.ResourceBarrier( commandList );
      TAC_CALL( context->Execute( errors ) );
    }

    TAC_ASSERT( cpuDescriptor.IsValid() );
    const D3D12_DESCRIPTOR_HEAP_TYPE heapType{ GetHeapType( mProgramBindType ) };
    const D3D12_CPU_DESCRIPTOR_HANDLE dst{ mDescriptorRegion.GetCPUHandle( binding.GetIndex() ) };
    const D3D12_CPU_DESCRIPTOR_HANDLE src{ cpuDescriptor.GetCPUHandle() };
    device->CopyDescriptorsSimple( 1, dst, src, heapType );
  }

  void BindlessArray::Unbind( Binding binding )
  {
    mUnusedBindings.push_back( binding );
    mHandles[ binding.GetIndex() ] = IHandle::kInvalidIndex;
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac::Render

