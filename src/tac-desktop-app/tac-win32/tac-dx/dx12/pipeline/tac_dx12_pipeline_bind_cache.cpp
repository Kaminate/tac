#include "tac_dx12_pipeline_bind_cache.h" // self-inc

#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  static D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType( D3D12ProgramBindType type )
  {
    if( type.IsBuffer() || type.IsTexture() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    if( type.IsSampler() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

    TAC_ASSERT_INVALID_CODE_PATH;
    return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
  }

  static D3D12ProgramBindType::Classification GetClassification( IBindlessArray::Params params )
  {
    const bool isShaderResource{ ( params.mBinding & Binding::ShaderResource ) != Binding::None };
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

  void                          BindlessArray::CheckType( ResourceHandle h )
  {
    if constexpr ( !kIsDebugMode )
      return;

    const D3D12ProgramBindType type{ mProgramBindType };
    TAC_ASSERT( type.IsValid() );
    TAC_ASSERT( !type.IsBuffer() || h.IsBuffer() );
    TAC_ASSERT( !type.IsTexture() || h.IsTexture() );
    TAC_ASSERT( !type.IsSampler() || h.IsSampler() );
  }

  void                          BindlessArray::Commit( CommitParams commitParams )
  {
    ID3D12GraphicsCommandList* commandList{ commitParams.mCommandList };
    const bool isCompute{ commitParams.mIsCompute };
    const UINT rootParameterIndex{ commitParams.mRootParameterIndex };

    const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ mDescriptorRegion.GetGPUHandle() };

    if( isCompute )
      commandList->SetComputeRootDescriptorTable( rootParameterIndex, gpuHandle );
    else
      commandList->SetGraphicsRootDescriptorTable( rootParameterIndex, gpuHandle );
  }

  void                          BindlessArray::SetFenceSignal( FenceSignal fenceSignal )
  {
    mFenceSignal = fenceSignal;
  }

  void                          BindlessArray::Resize( const int newSize )
  {
    const int oldSize{ mHandles.size() };

    TAC_ASSERT( mUnusedBindings.empty() );
    TAC_ASSERT( newSize > oldSize );

    mHandles.resize( newSize );

    // Allocating a new binding pops the back, so add in reverse order
    const int addedCount{ newSize - oldSize };
    for( int i{}; i < addedCount; ++i )
      mUnusedBindings.push_back( Binding{ newSize - i - 1 } );

    const D3D12_DESCRIPTOR_HEAP_TYPE heapType{ GetHeapType( mProgramBindType ) };
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12DescriptorHeapMgr& heapMgr{renderer.mDescriptorHeapMgr};
    DX12DescriptorHeap& heap{ heapMgr.mGPUHeaps[ heapType ] };
    DX12DescriptorAllocator* regionMgr{ heap.GetRegionMgr() };


    DX12DescriptorRegion newRegion{ regionMgr->Alloc( newSize ) };

    if( mDescriptorRegion.IsValid() )
    {
      ID3D12Device* device{ renderer.mDevice };
      device->CopyDescriptorsSimple( oldSize,
                                     newRegion.GetCPUHandle(),
                                     mDescriptorRegion.GetCPUHandle(),
                                     heapType );
      mDescriptorRegion.Free( mFenceSignal );
    }


    mDescriptorRegion = ( DX12DescriptorRegion&& )newRegion;
  }

  IBindlessArray::Binding       BindlessArray::Bind( ResourceHandle h, Errors& errors )
  {
    CheckType( h );

    if( mUnusedBindings.empty() )
    {
      const int oldSize{ mHandles.size() };
      const int newSize{ ( int )( ( oldSize + 2 ) * 1.5 ) };
      Resize( newSize );
    }

    const Binding binding{ mUnusedBindings.back() };
    mUnusedBindings.pop_back();
    mHandles[ binding.GetIndex() ] = h;

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
    }

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
    }
    }

    if( !transitionHelper.empty() )
    {
      DX12ContextManager* contextManager{ &renderer.mContextManager };

      TAC_CALL_RET( DX12Context* context{ contextManager->GetContext( errors ) } );
      context->SetSynchronous();
      IContext::Scope contextScope{ context };

      ID3D12GraphicsCommandList* commandList { context->GetCommandList() };
      transitionHelper.ResourceBarrier( commandList );
      TAC_CALL_RET( context->Execute( errors ) );
    }

    TAC_ASSERT( cpuDescriptor.IsValid() );

    const D3D12_DESCRIPTOR_HEAP_TYPE heapType{ GetHeapType( mProgramBindType ) };
    const D3D12_CPU_DESCRIPTOR_HANDLE dst{ mDescriptorRegion.GetCPUHandle( binding.GetIndex() ) };
    const D3D12_CPU_DESCRIPTOR_HANDLE src{ cpuDescriptor.GetCPUHandle() };
    device->CopyDescriptorsSimple( 1, dst, src, heapType );

    return binding;
  }

  void                          BindlessArray::Unbind( Binding binding )
  {
    mUnusedBindings.push_back( binding );
    mHandles[ binding.GetIndex() ] = IHandle::kInvalidIndex;
  }

#if 0
  Span< DX12Descriptor >        PipelineBindlessArray::GetDescriptors(
    DX12TransitionHelper* transitionHelper) const
  {
    TAC_ASSERT_UNIMPLEMENTED;
    return{};
  }
#endif

  // -----------------------------------------------------------------------------------------------

  DX12Descriptor         PipelineDynamicArray::GetDescriptor(
    IHandle ih,
    DX12TransitionHelper* transitionHelper ) const
  {
    DX12Renderer&   renderer   { DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    DX12SamplerMgr* samplerMgr { &renderer.mSamplerMgr };

    const D3D12ProgramBindType::Classification classification{
      mProgramBindType.GetClassification() };
    const int iHandle{ ih.GetIndex() };

    switch( classification )
    {
    case D3D12ProgramBindType::kTextureSRV:
    {
      const TextureHandle textureHandle{ iHandle };
      DX12Texture* texture{ textureMgr->FindTexture( textureHandle ) };
      TAC_ASSERT( texture );
      textureMgr->TransitionResource( &texture->mResource,
                                      Binding::ShaderResource,
                                      transitionHelper );
      return texture->mSRV.GetValue();
    }

    case D3D12ProgramBindType::kTextureUAV:
    {
      const TextureHandle textureHandle{ iHandle };
      DX12Texture* texture{ textureMgr->FindTexture( textureHandle ) };
      TAC_ASSERT( texture );
      textureMgr->TransitionResource( &texture->mResource,
                                      Binding::UnorderedAccess,
                                      transitionHelper );
      TAC_ASSERT( texture->mUAV.HasValue() );
      TAC_ASSERT( texture->mResource.GetState() & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
      return texture->mUAV.GetValue();
    }

    case D3D12ProgramBindType::kBufferSRV:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );
      TAC_ASSERT( buffer->mResource.GetState() & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      // [ ] Q: why the fuck is this calling a textureMgr function?
      //        arent we in the buffermgr?
      textureMgr->TransitionResource( &buffer->mResource,
                                      Binding::ShaderResource,
                                      transitionHelper );
      return buffer->mSRV.GetValue();
    }

    case D3D12ProgramBindType::kBufferUAV:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      const D3D12_RESOURCE_STATES stateBefore{ buffer->mResource.GetState() };
      TAC_ASSERT( buffer );
      TAC_ASSERT( stateBefore & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      // [ ] Q: why the fuck is this calling a textureMgr function?
      //        arent we in the buffermgr?
      textureMgr->TransitionResource( &buffer->mResource,
                                      Binding::UnorderedAccess,
                                      transitionHelper );
      const D3D12_RESOURCE_STATES stateAfter{ buffer->mResource.GetState() };
      TAC_ASSERT( stateAfter & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
      return buffer->mUAV.GetValue();
    }

    case D3D12ProgramBindType::kConstantBuffer:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );
      const D3D12_RESOURCE_STATES stateBefore{ buffer->mResource.GetState() };
      TAC_ASSERT( stateBefore & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      // [ ] Q: why the fuck is this calling a textureMgr function?
      //        arent we in the buffermgr?
      textureMgr->TransitionResource( &buffer->mResource,
                                      Binding::ConstantBuffer,
                                      transitionHelper );
      const D3D12_RESOURCE_STATES stateAfter{ buffer->mResource.GetState() };
      TAC_ASSERT( stateAfter & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
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

  Span< DX12Descriptor > PipelineDynamicArray::GetDescriptors(
    DX12TransitionHelper* transitionHelper ) const
  {
    if( mMaxBoundIndex == -1 )
      return {};

    const int nBound{ mMaxBoundIndex + 1 };
    TAC_ASSERT( nBound <= mHandleIndexes.size() );

    DX12Descriptor* dstBegin{
      ( DX12Descriptor* )FrameMemoryAllocate( sizeof( DX12Descriptor ) * nBound ) };
    DX12Descriptor* dstCur{ dstBegin };


    //const IHandle* hBegin{ mHandleIndexes.begin() };

    for( const IHandle& iHandle : Span< const IHandle >( mHandleIndexes.begin(), nBound ) )
    //for( const IHandle* hCur{ hBegin }; hCur < hBegin + nBound; hCur++ ) 
    //for( IHandle iHandle : mHandleIndexes )
    {
      //IHandle iHandle { *hCur };
      TAC_ASSERT( iHandle.IsValid() );

      const DX12Descriptor descriptor{ GetDescriptor( iHandle, transitionHelper ) };
      TAC_ASSERT( descriptor.IsValid() );
      *dstCur++ = descriptor;
    }

    return Span< DX12Descriptor >( dstBegin, nBound );
  }

  void                   PipelineDynamicArray::BindAtIndex( ResourceHandle h, int i )
  {
    CheckType( h );

    if( i >= mHandleIndexes.size() )
    {
      mHandleIndexes.resize( i + 5 ); // magic grow number
    }

    mHandleIndexes[ i ] = h;

    mMaxBoundIndex = Max( mMaxBoundIndex, i );
  }

  void                   PipelineDynamicArray::SetFence( FenceSignal fenceSignal )
  {
    mDescriptorRegion.Free( fenceSignal );
  }

  void                   PipelineDynamicArray::CheckType( ResourceHandle h )
  {
    if constexpr ( !kIsDebugMode )
      return;

    const D3D12ProgramBindType type{ mProgramBindType };
    TAC_ASSERT( type.IsValid() );
    TAC_ASSERT( !type.IsBuffer() || h.IsBuffer() );
    TAC_ASSERT( !type.IsTexture() || h.IsTexture() );
    TAC_ASSERT( !type.IsSampler() || h.IsSampler() );
  }

  void                   PipelineDynamicArray::Commit( CommitParams commitParams )
  {
    ID3D12GraphicsCommandList* commandList{ commitParams.mCommandList };
#if 0
    DX12DescriptorCaches* descriptorCaches{ commitParams.mDescriptorCaches };
#endif
    const bool isCompute{ commitParams.mIsCompute };
    const UINT rootParameterIndex{ commitParams.mRootParameterIndex };

    DX12Renderer&   renderer   { DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    DX12SamplerMgr* samplerMgr { &renderer.mSamplerMgr };
    ID3D12Device*   device     { renderer.mDevice };

    const D3D12_DESCRIPTOR_HEAP_TYPE heapType{ GetHeapType( mProgramBindType ) };

    DX12TransitionHelper transitionHelper;
    const Span< DX12Descriptor > cpuDescriptors{ GetDescriptors( &transitionHelper ) };
    transitionHelper.ResourceBarrier( commandList );

#if 0
    dynmc DX12DescriptorCache& descriptorCache{ ( *descriptorCaches )[ heapType ] };
    const DX12DescriptorRegion* gpuDescriptor{
      descriptorCache.GetGPUDescriptorForCPUDescriptors( cpuDescriptors ) };
#else
    DX12DescriptorHeap& heap{ renderer.mDescriptorHeapMgr.mGPUHeaps[ heapType ] };
    DX12DescriptorAllocator* descriptorAllocator{ heap.GetRegionMgr() };
    const int nDescriptors{ cpuDescriptors.size() };
    mDescriptorRegion = ( DX12DescriptorRegion&& )descriptorAllocator->Alloc( nDescriptors );
    DX12DescriptorRegion* gpuDescriptor{ &mDescriptorRegion };
#endif

    for( int iDescriptor{}; iDescriptor < nDescriptors; ++iDescriptor )
    {
      DX12Descriptor cpuDescriptor { cpuDescriptors[ iDescriptor ] };
      DX12DescriptorHeap* srcHeap{ cpuDescriptor.mOwner };
      TAC_ASSERT( srcHeap );
      TAC_ASSERT( srcHeap->GetType() == heapType );
      const D3D12_CPU_DESCRIPTOR_HANDLE src{ cpuDescriptor.GetCPUHandle() };
      const D3D12_CPU_DESCRIPTOR_HANDLE dst{ gpuDescriptor->GetCPUHandle( iDescriptor ) };
      TAC_ASSERT( src.ptr );
      TAC_ASSERT( dst.ptr );
      device->CopyDescriptorsSimple( 1, dst, src, heapType );

      const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ gpuDescriptor->GetGPUHandle() };
      if( isCompute )
        commandList->SetComputeRootDescriptorTable( rootParameterIndex, gpuHandle );
      else
        commandList->SetGraphicsRootDescriptorTable( rootParameterIndex, gpuHandle );
    }
  }

  // -----------------------------------------------------------------------------------------------

#if 0
  Span< DX12Descriptor >
                 RootParameterBinding::GetDescriptors(
                   DX12TransitionHelper* transitionHelper ) const
  {
    if( mResourceHandle.IsValid() )
    {
      TAC_ASSERT_UNIMPLEMENTED;
      return {};
    }

    return mPipelineArray->GetDescriptors( transitionHelper );
  }
#endif

  void           RootParameterBinding::SetFence( FenceSignal fenceSignal )
  {
    if( mType == Type::kDynamicArray )
    {
      mPipelineDynamicArray.SetFence( fenceSignal );
    }
  }

  void           RootParameterBinding::Commit( CommitParams commitParams )
  {
    if( mProgramBindDesc.BindsAsDescriptorTable() )
    {
      if( mType == RootParameterBinding::Type::kDynamicArray )
      {
        mPipelineDynamicArray.Commit( commitParams );
      }
      else if( mType == Type::kBindlessArray )
      {
        // gross gross gross gross gross gross gross gross 
        ( ( BindlessArray* )mBindlessArray )->Commit( commitParams );
      }
      else
      {
        TAC_ASSERT_INVALID_CODE_PATH;
      }
    }
    else
    {
      const D3D12ProgramBindType programBindType{mProgramBindDesc.mType};
      TAC_ASSERT_MSG( !programBindType.IsTexture(),
                      "Textures must be bound through descriptor tables" );

      // this includes constant buffers
      TAC_ASSERT( programBindType.IsBuffer() );

      const bool isCompute{ commitParams.mIsCompute };

      const BufferHandle bufferHandle{ mResourceHandle.GetBufferHandle() };

      DX12Renderer& renderer{ DX12Renderer::sRenderer };
      DX12BufferMgr* bufferMgr{ &renderer.mBufMgr };

      const DX12Buffer* buffer{ bufferMgr->FindBuffer( bufferHandle ) };
      TAC_ASSERT( buffer );
      
      const D3D12_RESOURCE_STATES state{ buffer->mResource.GetState() };

      const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress{ buffer->mGPUVirtualAddr };
      TAC_ASSERT( gpuVirtualAddress );

      using CmdList = ID3D12GraphicsCommandList;
      using CmdListFn = void ( CmdList::* )( UINT, D3D12_GPU_VIRTUAL_ADDRESS );

      CmdListFn cmdListFn{};

      if( programBindType.IsConstantBuffer() )
      {
        TAC_ASSERT( state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
        cmdListFn = isCompute
          ? &CmdList::SetComputeRootConstantBufferView
          : &CmdList::SetGraphicsRootConstantBufferView;
      }
      else if( programBindType.IsSRV() )
      {
        TAC_ASSERT( state & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );

        cmdListFn = isCompute
          ? &CmdList::SetComputeRootShaderResourceView
          : &CmdList::SetGraphicsRootShaderResourceView;
      }
      else if( programBindType.IsUAV() )
      {
        TAC_ASSERT( state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
        cmdListFn = isCompute
          ? &CmdList::SetComputeRootUnorderedAccessView
          : &CmdList::SetGraphicsRootUnorderedAccessView;
      }

      TAC_ASSERT( cmdListFn );
      ID3D12GraphicsCommandList* commandList{ commitParams.mCommandList };
      ( commandList->*cmdListFn )( mRootParameterIndex, gpuVirtualAddress );
    }


  }

  // -----------------------------------------------------------------------------------------------

  PipelineBindCache::PipelineBindCache( const D3D12ProgramBindDescs& descs )
  {
    const int n{ descs.size() };

    for( int i{}; i < n; ++i )
    {
      const D3D12ProgramBindDesc& desc{ descs[ i ] };

      //PipelineDynamicArray pipelineDynamicArray;
      //if( desc.BindsAsDescriptorTable() && desc.mBindCount > 0 )
      //{
      //}

      RootParameterBinding rootParameterBinding;
      rootParameterBinding.mProgramBindDesc = desc;
      rootParameterBinding.mRootParameterIndex = i;
      //rootParameterBinding.mPipelineDynamicArray = ( PipelineDynamicArray&& )pipelineDynamicArray;
      rootParameterBinding.mPipelineDynamicArray.mProgramBindType = desc.mType;
      if( desc.mBindCount > 0 )
        rootParameterBinding.mPipelineDynamicArray.mHandleIndexes.resize( desc.mBindCount );
      rootParameterBinding.mType = desc.BindsAsDescriptorTable()
        ? RootParameterBinding::Type::kDynamicArray
        : RootParameterBinding::Type::kResourceHandle;

      //if( desc.mBindCount

      push_back( ( RootParameterBinding&& )rootParameterBinding );
    }
  }

} // namespace Tac::Render

