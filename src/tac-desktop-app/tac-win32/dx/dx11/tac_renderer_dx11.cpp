#include "tac_renderer_dx11.h" // self-inc

#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_frame_vector.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-std-lib/math/tac_math.h"
//#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-engine-core/profile/tac_profile.h"
//#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-engine-core/system/tac_desktop_window.h"
//#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"

#include "tac-desktop-app/tac_desktop_app.h"
#include "tac-win32/dx/dx11/shader/tac_dx11_shader_compiler.h"
#include "tac-win32/dx/dx11/shader/tac_dx11_shader_postprocess.h"
#include "tac-win32/dx/dx11/shader/tac_dx11_shader_program.h"
#include "tac-win32/dx/dx11/shader/tac_dx11_shader_preprocess.h"
#include "tac-win32/dx/dx11/tac_dx11_enum_helper.h"
#include "tac-win32/dx/dx11/tac_dx11_namer.h"
#include "tac-win32/dx/dxgi/tac_dxgi.h"
#include "tac-win32/dx/dxgi/tac_dxgi_debug.h"
#include "tac-win32/dx/pix/tac_pix.h"
#include "tac-desktop-app/tac_desktop_app_threads.h"

#include <initguid.h>
#include <d3dcompiler.h> // D3DCOMPILE_...
#include <d3dcommon.h> // WKPDID_D3DDebugObjectName, ID3DBlob

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "D3DCompiler.lib" )


#if _MSC_VER
#pragma warning( disable : 4505 ) // unreferenced local function has been removed
#endif

namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------

  // -----------------------------------------------------------------------------------------------

  static String TryInferDX11ErrorStr( const HRESULT res )
  {
    switch( res )
    {
      case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND	The file was not found.";
      case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS	There are too many unique instances of a particular type of state object.";
      case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS	There are too many unique instances of a particular type of view object.";
      case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD: return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD	The first call to ID3D11DeviceContext::Map after either ID3D11Device::CreateDeferredContext or ID3D11DeviceContext::FinishCommandList per Resource was not D3D11_MAP_WRITE_DISCARD.";

        // move to Dxgi.cpp?
      case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL The method call is invalid.For example, a method's parameter may not be a valid pointer.";
      case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING The previous blit operation that is transferring information to or from this surface is incomplete.";
      case DXGI_ERROR_ACCESS_DENIED: return "DXGI_ERROR_ACCESS_DENIED You tried to use a resource to which you did not have the required access privileges.This error is most typically caused when you write to a shared resource with read - only access.";
      case DXGI_ERROR_ACCESS_LOST: return "DXGI_ERROR_ACCESS_LOST The desktop duplication interface is invalid.The desktop duplication interface typically becomes invalid when a different type of image is displayed on the desktop.";
      case DXGI_ERROR_ALREADY_EXISTS: return "DXGI_ERROR_ALREADY_EXISTS The desired element already exists.This is returned by DXGIDeclareAdapterRemovalSupport if it is not the first time that the function is called.";
      case DXGI_ERROR_CANNOT_PROTECT_CONTENT: return "DXGI_ERROR_CANNOT_PROTECT_CONTENT DXGI can't provide content protection on the swap chain. This error is typically caused by an older driver, or when you use a swap chain that is incompatible with content protection.";
      case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG The application's device failed due to badly formed commands sent by the application. This is an design-time issue that should be investigated and fixed.";
      case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED The video card has been physically removed from the system, or a driver upgrade for the video card has occurred.The application should destroy and recreate the device.For help debugging the problem, call ID3D10Device::GetDeviceRemovedReason.";
      case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET The device failed due to a badly formed command.This is a run - time issue; The application should destroy and recreate the device.";
      case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR The driver encountered a problem and was put into the device removed state.";
      case DXGI_ERROR_FRAME_STATISTICS_DISJOINT: return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT An event( for example, a power cycle ) interrupted the gathering of presentation statistics.";
      case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE: return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE The application attempted to acquire exclusive ownership of an output, but failed because some other application( or device within the application ) already acquired ownership.";
      case DXGI_ERROR_MORE_DATA: return "DXGI_ERROR_MORE_DATA The buffer supplied by the application is not big enough to hold the requested data.";
      case DXGI_ERROR_NAME_ALREADY_EXISTS: return "DXGI_ERROR_NAME_ALREADY_EXISTS The supplied name of a resource in a call to IDXGIResource1::CreateSharedHandle is already associated with some other resource.";
      case DXGI_ERROR_NONEXCLUSIVE: return "DXGI_ERROR_NONEXCLUSIVE A global counter resource is in use, and the Direct3D device can't currently use the counter resource.";
      case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE The resource or request is not currently available, but it might become available later.";
      case DXGI_ERROR_NOT_FOUND: return "DXGI_ERROR_NOT_FOUND When calling IDXGIObject::GetPrivateData, the GUID passed in is not recognized as one previously passed to IDXGIObject::SetPrivateData or IDXGIObject::SetPrivateDataInterface.When calling IDXGIFactory::EnumAdapters or IDXGIAdapter::EnumOutputs, the enumerated ordinal is out of range.";
      case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED: return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED Reserved";
      case DXGI_ERROR_REMOTE_OUTOFMEMORY: return "DXGI_ERROR_REMOTE_OUTOFMEMORY Reserved";
      case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE: return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE The DXGI output( monitor ) to which the swap chain content was restricted is now disconnected or changed.";
      case DXGI_ERROR_SDK_COMPONENT_MISSING: return "DXGI_ERROR_SDK_COMPONENT_MISSING The operation depends on an SDK component that is missing or mismatched.";
      case DXGI_ERROR_SESSION_DISCONNECTED: return "DXGI_ERROR_SESSION_DISCONNECTED The Remote Desktop Services session is currently disconnected.";
      case DXGI_ERROR_UNSUPPORTED: return "DXGI_ERROR_UNSUPPORTED The requested functionality is not supported by the device or the driver.";
      case DXGI_ERROR_WAIT_TIMEOUT: return "DXGI_ERROR_WAIT_TIMEOUT The time - out interval elapsed before the next desktop frame was available.";

      case E_FAIL: return "E_FAIL	Attempted to create a device with the debug layer enabled and the layer is not installed.";
      case E_INVALIDARG: return "E_INVALIDARG	An invalid parameter was passed to the returning function.";
      case E_OUTOFMEMORY: return "E_OUTOFMEMORY	Direct3D could not allocate sufficient memory to complete the call.";
      case E_NOTIMPL: return "E_NOTIMPL	The method call isn't implemented with the passed parameter combination.";
      case S_FALSE: return "S_FALSE	Alternate success value, indicating a successful but nonstandard completion( the precise meaning depends on context ).";
      case S_OK: return "S_OK	No error occurred.";
      default: return "idk lol";
    }
  }

  void DX11CallAux( const char* fnCallWithArgs, const HRESULT res, Errors& errors )
  {
    String result;
    result += fnCallWithArgs;
    result += " returned ";
    result += Tac::ToString( ( const void* )( std::uintptr_t )res );

    const String inferredErrorMessage = TryInferDX11ErrorStr( res );
    if( !inferredErrorMessage.empty() )
    {
      result += "(";
      result += inferredErrorMessage;
      result += ")";
    }
    
    TAC_RAISE_ERROR( result );
  }


  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------

  // Helper classes

  

  // -----------------------------------------------------------------------------------------------


  static bool gVerbose;

  void   RegisterRendererDirectX11()
  {
    SetRendererFactory<RendererDirectX11>( RendererAPI::DirectX11 );
  }

  // -----------------------------------------------------------------------------------------------

  ID3D11InfoQueue* RendererDirectX11::GetInfoQueue()
  {
    RendererDirectX11* renderer = RendererDirectX11::GetInstance();
    return renderer->mInfoQueueDEBUG;
  }

  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------


  struct ScopedSeverityBreak
  {
    ScopedSeverityBreak();
    ~ScopedSeverityBreak();
  private:
    Vector< D3D11_MESSAGE_SEVERITY > mSeverities;
  };

  // -----------------------------------------------------------------------------------------------

  ScopedSeverityBreak::ScopedSeverityBreak()
  {
    const D3D11_MESSAGE_SEVERITY severities[] =
    {
      D3D11_MESSAGE_SEVERITY_CORRUPTION,
      D3D11_MESSAGE_SEVERITY_ERROR,
      D3D11_MESSAGE_SEVERITY_WARNING,
#if 0
      D3D11_MESSAGE_SEVERITY_INFO,
      D3D11_MESSAGE_SEVERITY_MESSAGE,
#endif
    };

    for( const D3D11_MESSAGE_SEVERITY severity : severities )
    {
      ID3D11InfoQueue* mInfoQueueDEBUG = RendererDirectX11::GetInfoQueue();
      const BOOL breakOnSeverity = mInfoQueueDEBUG->GetBreakOnSeverity( severity );
      if( breakOnSeverity )
      {
        mSeverities.push_back( severity );
        mInfoQueueDEBUG->SetBreakOnSeverity( severity, FALSE );
      }
    }
  }

  ScopedSeverityBreak::~ScopedSeverityBreak()
  {
    ID3D11InfoQueue* mInfoQueueDEBUG = RendererDirectX11::GetInfoQueue();
    for( const D3D11_MESSAGE_SEVERITY severity : mSeverities )
      mInfoQueueDEBUG->SetBreakOnSeverity( severity, TRUE );
  }


  // -----------------------------------------------------------------------------------------------

  static void WaitUntilDrawCallFinishes( ID3D11Device* device, ID3D11DeviceContext* deviceContext )
  {
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_EVENT;
    ID3D11Query* query = nullptr;
    auto createQueryResult = device->CreateQuery( &desc, &query );
    TAC_ASSERT( SUCCEEDED( createQueryResult ) );
    deviceContext->End( query );

    for( ;; )
    {
      HRESULT getDataResult = deviceContext->GetData( query, nullptr, 0, 0 );
      if( getDataResult == S_FALSE )
      {
        // not finished gpu commands executing
      }
      else if( getDataResult == S_OK )
      {
        break;
      }
      else
      {
        TAC_ASSERT_INVALID_CODE_PATH;
      }
    }

    query->Release();
  }
  // -----------------------------------------------------------------------------------------------


  static HashValue HashDrawCallSamplers(const DrawCallSamplers& drawCallSamplers )
  {
    Hasher hasher;

    // to differeniate {} from {0}
    hasher.Eat(drawCallSamplers.size());

    for( const SamplerStateHandle& sampler : drawCallSamplers )
      hasher.Eat( sampler.GetIndex() );

    return hasher;
  }

  struct TransientWchar
  {
    static const int N = 100;
    TransientWchar( const StringView str )
    {
      TAC_ASSERT( str.size() < N );
      WCHAR* resultIter = mBuffer;
      for( char c : str )
        *resultIter++ = ( WCHAR )c;
      *resultIter++ = 0;
    }

    const WCHAR* data() const { return mBuffer; }
  private:
    WCHAR mBuffer[ N ];
  };


  ConstantBufferHandle RendererDirectX11::FindCbufferOfName( const StringView& name )
  {
    for( int i = 0; i < kMaxConstantBuffers; ++i )
      if( ( StringView )mConstantBuffers[ i ].mName == name )
        return ConstantBufferHandle( i );
    return ConstantBufferHandle();
  }


  // -----------------------------------------------------------------------------------------------

  RendererDirectX11::~RendererDirectX11()
  {
    DXGIUninit();

    mDeviceContext.swap( {});
    mDevice.swap( {});

    if( mInfoQueueDEBUG )
    {
      mInfoQueueDEBUG->Release();
      mInfoQueueDEBUG = nullptr;
    }

    if( mUserAnnotationDEBUG )
    {
      mUserAnnotationDEBUG->Release();
      mUserAnnotationDEBUG = nullptr;
    }

    DXGIReportLiveObjects();
  }


  void RendererDirectX11::Init( Errors& errors )
  {
    const UINT createDeviceFlags = IsDebugMode ? D3D11_CREATE_DEVICE_DEBUG : 0;
    const Array featureLevels = { D3D_FEATURE_LEVEL_12_1 };

    PCom<ID3D11Device> device;
    PCom<ID3D11DeviceContext> deviceContext;

    TAC_CALL( DXGIInit( errors ));

    auto adapter = DXGIGetBestAdapter();

    TAC_DX11_CALL( D3D11CreateDevice(
                   (IDXGIAdapter*)adapter,
                   D3D_DRIVER_TYPE_UNKNOWN,
                   nullptr,
                   createDeviceFlags,
                   featureLevels.data(),
                   featureLevels.size(),
                   D3D11_SDK_VERSION,
                   device.CreateAddress(),
                   nullptr,
                   deviceContext.CreateAddress() ) );

    TAC_ASSERT( device );
    TAC_ASSERT( deviceContext );
    device.QueryInterface( mDevice );
    deviceContext.QueryInterface( mDeviceContext );
    TAC_ASSERT( mDevice );
    TAC_ASSERT( mDeviceContext );

    // If you're directx is crashing / throwing exception, don't forget to check
    // your output window, it likes to put error messages there
    if constexpr( IsDebugMode )
    {
      TAC_DX11_CALL( mDevice->QueryInterface( IID_PPV_ARGS( &mInfoQueueDEBUG ) ) );
      TAC_DX11_CALL( mDeviceContext->QueryInterface( IID_PPV_ARGS( &mUserAnnotationDEBUG ) ) );

      const D3D11_MESSAGE_SEVERITY breakSeverities[] =
      {
        D3D11_MESSAGE_SEVERITY_CORRUPTION,
        D3D11_MESSAGE_SEVERITY_ERROR,
        D3D11_MESSAGE_SEVERITY_WARNING,
#if 0
        D3D11_MESSAGE_SEVERITY_INFO,
        D3D11_MESSAGE_SEVERITY_MESSAGE
#endif
      };
      for( const D3D11_MESSAGE_SEVERITY severity : breakSeverities )
        mInfoQueueDEBUG->SetBreakOnSeverity( severity, TRUE );

    }





  }

  RendererDirectX11* RendererDirectX11::GetInstance()
  {
      return ( RendererDirectX11* )Instance;
  }

  void RendererDirectX11::RenderBegin( const Frame*, Errors& )
  {
    if( gVerbose )
      OS::OSDebugPrintLine("Render2::Begin");

    mBoundSRVs = {};
    mBoundConstantBuffers = {};

//       for( int iWindow = 0; iWindow < mWindowCount; ++iWindow )
//       {
//         const FramebufferHandle framebufferHandle = mWindows[ iWindow ];
//         TAC_ASSERT( framebufferHandle.IsValid() );
// 
//         Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];
//         ID3D11RenderTargetView* renderTargetView = framebuffer->mRenderTargetView;
//         ID3D11DepthStencilView* depthStencilView = framebuffer->mDepthStencilView;
// 
//         const UINT ClearFlags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
//         const FLOAT ClearDepth = 1.0f;
//         const UINT8 ClearStencil = 0;
//         mDeviceContext->ClearDepthStencilView( depthStencilView, ClearFlags, ClearDepth, ClearStencil );
// 
//         const FLOAT ClearGrey = 0.5f;
//         const FLOAT ClearColorRGBA[] = { ClearGrey, ClearGrey, ClearGrey,  1.0f };
//         mDeviceContext->ClearRenderTargetView( renderTargetView, ClearColorRGBA );
//       }

    for( int i = 0; i < kMaxFramebuffers; ++i )
      mBoundFramebuffersThisFrame[ i ] = false;

    mBoundBlendState = nullptr;
    mBoundDepthStencilState = nullptr;
    mBoundViewHandle = ViewHandle();
    //mIndexBuffer = nullptr;

  }

  static void DX11ShaderReloadFunction( const ShaderHandle shaderHandle,
                                        const ShaderNameStringView& shaderName,
                                        Errors& errors )
  {
    const int iProgram = shaderHandle.GetIndex();
    RendererDirectX11* renderer = RendererDirectX11::GetInstance();
    renderer->mPrograms[ iProgram ] = DX11LoadProgram( shaderName, errors );
  }

  void RendererDirectX11::RenderEnd( float dt, const Frame*, Errors& errors )
  {
    ShaderReloadHelperUpdate( dt, DX11ShaderReloadFunction, errors );

    if( gVerbose )
      OS::OSDebugPrintLine("Render2::End");
  }



  void RendererDirectX11::RenderDrawCallViewAndUAV( const Frame* frame,
                                                    const DrawCall* drawCall )
  {
    const bool isSameView = mBoundViewHandle == drawCall->mViewHandle;

    const bool isSameUAV = [ & ]() // this could be replaced with a uav hash
    {
      for( int i = 0; i < TAC_ARRAY_SIZE( DrawCallUAVs::mUAVTextures ); ++i )
        if( drawCall->mDrawCallUAVs.mUAVTextures[ i ] != mBoundDrawCallUAVs.mUAVTextures[ i ] )
          return false;

      for( int i = 0; i < TAC_ARRAY_SIZE( DrawCallUAVs::mUAVMagicBuffers ); ++i )
        if( drawCall->mDrawCallUAVs.mUAVMagicBuffers[ i ] != mBoundDrawCallUAVs.mUAVMagicBuffers[ i ] )
          return false;

      return false;
    }( );

    if( isSameUAV && isSameView )
      return;

    ID3D11DepthStencilView* dsv = nullptr;
    FixedVector< ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT > views = {};
    if( drawCall->mViewHandle.IsValid() )
    {
      const View* view = frame->mViews.FindView( drawCall->mViewHandle );
      const Framebuffer* framebuffer = &mFramebuffers[ ( int )view->mFrameBufferHandle ];


      // Should dsv be optional?
      // Should rtv be optional?

      dsv = framebuffer->mDepthStencilView;
      if( framebuffer->mRenderTargetView )
        views.push_back( framebuffer->mRenderTargetView );

      // if 1st use this frame clear it?

      if( !mBoundFramebuffersThisFrame[ ( int )view->mFrameBufferHandle ] )
      {
        mBoundFramebuffersThisFrame[ ( int )view->mFrameBufferHandle ] = true;

        if( dsv )
          mDeviceContext->ClearDepthStencilView( dsv,
                                                 D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                                 1.0f, // clear depth value
                                                 0 ); // clear stencil value

        if( framebuffer->mRenderTargetView && framebuffer->mClearEachFrame )
        {
          mDeviceContext->ClearRenderTargetView( framebuffer->mRenderTargetView,
                                                 framebuffer->mClearColorRGBA );
        }

        ID3D11ShaderResourceView* nullViews[ 16 ] = {};
        mDeviceContext->VSSetShaderResources( 0, 16, nullViews );
        mDeviceContext->PSSetShaderResources( 0, 16, nullViews );
        mDeviceContext->GSSetShaderResources( 0, 16, nullViews );
      }

      TAC_ASSERT( view->mViewportSet );
      const D3D11_VIEWPORT viewport
      {
        .TopLeftX = view->mViewport.mBottomLeftX,
        .TopLeftY = -view->mViewport.mBottomLeftY, // convert opengl to directx
        .Width = view->mViewport.mWidth,
        .Height = view->mViewport.mHeight,
        .MinDepth = view->mViewport.mMinDepth,
        .MaxDepth = view->mViewport.mMaxDepth,
      };
      mDeviceContext->RSSetViewports( 1, &viewport );

      // used if the rasterizer state ScissorEnable is TRUE
      TAC_ASSERT( view->mScissorSet );
      const D3D11_RECT scissor
      {
        .left = ( LONG )view->mScissorRect.mXMinRelUpperLeftCornerPixel,
        .top = ( LONG )view->mScissorRect.mYMinRelUpperLeftCornerPixel,
        .right = ( LONG )view->mScissorRect.mXMaxRelUpperLeftCornerPixel,
        .bottom = ( LONG )view->mScissorRect.mYMaxRelUpperLeftCornerPixel,
      };
      mDeviceContext->RSSetScissorRects( 1, &scissor );
    }


    if( drawCall->mDrawCallUAVs.HasValidHandle() )
    {
      FixedVector< ID3D11UnorderedAccessView*, 10 > uavs;

      // populate uavs
      for( int i = 0; i < 2; ++i )
      {
        MagicBufferHandle magicBuffer = drawCall->mDrawCallUAVs.mUAVMagicBuffers[ i ];
        TextureHandle texture = drawCall->mDrawCallUAVs.mUAVTextures[ i ];
        if( ID3D11UnorderedAccessView* uav =
            magicBuffer.IsValid() ? mMagicBuffers[ ( int )magicBuffer ].mUAV :
            texture.IsValid() ? mTextures[ ( int )texture ].mTextureUAV :
            nullptr )
        {
          uavs.resize( Max( i + 1, uavs.size() ) );
          uavs[ i ] = uav;
        }
      }

      mDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews( views.size(),
                                                                 views.data(),
                                                                 dsv,
                                                                 views.size(),
                                                                 uavs.size(),
                                                                 uavs.data(),
                                                                 nullptr );
    }
    else
    {
      mDeviceContext->OMSetRenderTargets( views.size(), views.data(), dsv );
    }

    mBoundViewHandle = drawCall->mViewHandle;
    mBoundDrawCallUAVs = drawCall->mDrawCallUAVs;
  }

  BoundCBufs BoundCBufs::ShaderCBufs( const ConstantBuffers& shaderCBs )
  {
    BoundCBufSlots mBoundConstantBuffers;
    int            mMaxUsedIndex = -1;
    int            mBoundCBufCount = 0;
    Hasher         mHash;

    RendererDirectX11* renderer = RendererDirectX11::GetInstance();
    const ConstantBuffer* pCBs = renderer->mConstantBuffers;

    const int shaderCBufCount = shaderCBs.size();
    for( int iSlot = 0; iSlot < shaderCBufCount; iSlot++ )
    {
      const ConstantBufferHandle hCBuf = shaderCBs[ iSlot ];
      TAC_ASSERT( hCBuf.IsValid() );

      const int iBuf = ( int )hCBuf;
      const ConstantBuffer& cBuf = pCBs[iBuf];

      mBoundConstantBuffers[ iSlot ] = cBuf.mBuffer;
      mMaxUsedIndex = Max( mMaxUsedIndex, iSlot );
      mHash.Eat( iSlot );
      mHash.Eat( iBuf );
      mBoundCBufCount++;
    }

    return BoundCBufs
    {
      .mBoundConstantBuffers = mBoundConstantBuffers,
      .mMaxUsedIndex = mMaxUsedIndex,
      .mBoundCBufCount = mBoundCBufCount,
      .mHash = mHash,
    };
  }


  void RendererDirectX11::RenderDrawCallShader( const DrawCall* drawCall )
  {
    const DX11Program* program = FindProgram( drawCall->mShaderHandle );
    if( !program )
      return;

    // program->mPixelShader and program->geometry shader allowed to be null
    // [ ] Q: Why pixel shader allowed to be null?
    TAC_ASSERT( program->mVertexShader );
    mDeviceContext->VSSetShader( program->mVertexShader, NULL, 0 );
    mDeviceContext->PSSetShader( program->mPixelShader, NULL, 0 );
    mDeviceContext->GSSetShader( program->mGeometryShader, NULL, 0 );

    const BoundCBufs shaderBoundCBufs = BoundCBufs::ShaderCBufs( program->mConstantBuffers );
    if( shaderBoundCBufs.mHash != mBoundConstantBuffers.mHash )
    {
      mBoundConstantBuffers = shaderBoundCBufs;

      using CBufSlotPointer = ID3D11Buffer**;

      const UINT StartSlot = 0;
      const UINT NumBuffers = mBoundConstantBuffers.mMaxUsedIndex + 1;
      const CBufSlotPointer ppConstantBuffers = mBoundConstantBuffers.mBoundConstantBuffers.data();

      mDeviceContext->PSSetConstantBuffers( StartSlot, NumBuffers, ppConstantBuffers );
      mDeviceContext->VSSetConstantBuffers( StartSlot, NumBuffers, ppConstantBuffers );
      mDeviceContext->GSSetConstantBuffers( StartSlot, NumBuffers, ppConstantBuffers );
    }
  }

  void RendererDirectX11::RenderDrawCallBlendState( const DrawCall* drawCall )
  {

    if( drawCall->mBlendStateHandle.IsValid()
        && mBoundBlendState != mBlendStates[ ( int )drawCall->mBlendStateHandle ] )
    {
      mBoundBlendState = mBlendStates[ ( int )drawCall->mBlendStateHandle ];
      TAC_ASSERT( mBoundBlendState );
      const FLOAT blendFactorRGBA[] = { 1.0f, 1.0f, 1.0f, 1.0f };
      const UINT sampleMask = 0xffffffff;
      mDeviceContext->OMSetBlendState( mBoundBlendState, blendFactorRGBA, sampleMask );
    }
  }

  void RendererDirectX11::RenderDrawCallDepthState( const DrawCall* drawCall )
  {
    if( drawCall->mDepthStateHandle.IsValid()
        && mBoundDepthStencilState != mDepthStencilStates[ ( int )drawCall->mDepthStateHandle ] )
    {
      mBoundDepthStencilState = mDepthStencilStates[ ( int )drawCall->mDepthStateHandle ];
      TAC_ASSERT( mBoundDepthStencilState );
      const UINT stencilRef = 0;
      mDeviceContext->OMSetDepthStencilState( mBoundDepthStencilState, stencilRef );
    }
  }

  void RendererDirectX11::RenderDrawCallIndexBuffer( const DrawCall* drawCall )
  {
    if( drawCall->mIndexBufferHandle != mBoundIndexBuffer )
    {
      mBoundIndexBuffer = drawCall->mIndexBufferHandle;
      if( drawCall->mIndexBufferHandle.IsValid() )
      {
        const IndexBuffer* indexBuffer = &mIndexBuffers[ ( int )drawCall->mIndexBufferHandle ];
        if( !indexBuffer->mBuffer )
          OS::OSDebugBreak();
        TAC_ASSERT( indexBuffer->mBuffer );
        const DXGI_FORMAT dxgiFormat = GetDXGIFormatTexture( indexBuffer->mFormat );
        const UINT byteOffset = 0; //  drawCall->mStartIndex * indexBuffer->mFormat.mPerElementByteCount;
        mDeviceContext->IASetIndexBuffer( indexBuffer->mBuffer,
                                          dxgiFormat,
                                          byteOffset );
      }
      else
      {
        mDeviceContext->IASetIndexBuffer( nullptr, DXGI_FORMAT_UNKNOWN, 0 );
      }
    }

  }

  void RendererDirectX11::RenderDrawCallVertexBuffer( const DrawCall* drawCall )
  {
    if( drawCall->mVertexBufferHandle == mBoundVertexBuffer )
      return;

    mBoundVertexBuffer = drawCall->mVertexBufferHandle;

    if( const VertexBuffer* vertexBuffer = FindVertexBuffer( drawCall->mVertexBufferHandle ) )
    {
      TAC_ASSERT( vertexBuffer->mBuffer );
      using PointerToBuffer = ID3D11Buffer*;
      const UINT startSlot = 0;
      const UINT NumBuffers = 1;
      const UINT Strides[ NumBuffers ] = { ( UINT )vertexBuffer->mStride };
      const UINT ByteOffsets[ NumBuffers ] = { 0 };
      const PointerToBuffer buffers[ NumBuffers ] = { vertexBuffer->mBuffer };
      mDeviceContext->IASetVertexBuffers( startSlot,
                                          NumBuffers,
                                          buffers,
                                          Strides,
                                          ByteOffsets );

    }
    else
    {
      UINT         StartSlot = 0;
      UINT         NumBuffers = 16;
      ID3D11Buffer *VertexBuffers[ 16 ] = {};
      UINT         Strides[ 16 ] = {};
      UINT         Offsets[ 16 ] = {};
      mDeviceContext->IASetVertexBuffers( StartSlot, NumBuffers, VertexBuffers, Strides, Offsets );
    }
  }

  void RendererDirectX11::RenderDrawCallRasterizerState( const DrawCall* drawCall )
  {
    if( drawCall->mRasterizerStateHandle.IsValid() )
    {
      ID3D11RasterizerState* rasterizerState = mRasterizerStates[ ( int )drawCall->mRasterizerStateHandle ];
      TAC_ASSERT( rasterizerState );
      mDeviceContext->RSSetState( rasterizerState );
    }

  }


  void RendererDirectX11::RenderDrawCallSamplerState( const DrawCall* drawCall )
  {
    const HashValue drawCallSamplerHash = HashDrawCallSamplers(drawCall->mSamplerStateHandle);
    if( mBoundSamplerHash != drawCallSamplerHash )
    {
      mBoundSamplerHash = drawCallSamplerHash;
      FixedVector< ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT > Samplers;
      for( const SamplerStateHandle& sampler : drawCall->mSamplerStateHandle )
      {
        ID3D11SamplerState* samplerState = mSamplerStates[ sampler.GetIndex() ];
        TAC_ASSERT( samplerState );
        Samplers.push_back( samplerState );
      }

      mDeviceContext->VSSetSamplers( 0, Samplers.size(), Samplers.data() );
      mDeviceContext->PSSetSamplers( 0, Samplers.size(), Samplers.data() );
    }
  }

  void RendererDirectX11::RenderDrawCallVertexFormat( const DrawCall* drawCall )
  {
    if( mBoundDrawCallVertexFormat == drawCall->mVertexFormatHandle )
      return;
    mBoundDrawCallVertexFormat = drawCall->mVertexFormatHandle;
    ID3D11InputLayout* inputLayout
      = drawCall->mVertexFormatHandle.IsValid()
      ? mInputLayouts[ ( int )drawCall->mVertexFormatHandle ]
      : nullptr;
    mDeviceContext->IASetInputLayout( inputLayout );
  }


  BoundSRVs BoundSRVs::DrawCallSRVs( const DrawCall* drawCall )
  {
    BoundSrvSlots mBoundShaderResourceViews;
    int           mMaxUsedIndex = -1;
    int           mBoundTextureCount = 0;
    Hasher        mHash;

    RendererDirectX11* renderer = RendererDirectX11::GetInstance();
    const Texture* mTextures = renderer->mTextures;


    const int n = drawCall->mDrawCallTextures.size();
    mHash.Eat( n );
    for( int iSlot = 0; iSlot < n; ++iSlot )
    {
      const TextureHandle hTex = drawCall->mDrawCallTextures[ iSlot ];
      const Texture* pTex = renderer->FindTexture( hTex );
      if( !pTex )
        continue;

      TAC_ASSERT_MSG( pTex->mTextureSRV, "Did you set the TexSpec::mBinding?" );

      mBoundShaderResourceViews[ iSlot ] = pTex->mTextureSRV;
      mBoundTextureCount++;
      mMaxUsedIndex = Max( mMaxUsedIndex, iSlot );
      mHash.Eat( iSlot );
      mHash.Eat( hTex.GetIndex() );
    }

    return BoundSRVs
    {
      .mBoundShaderResourceViews = mBoundShaderResourceViews,
      .mMaxUsedIndex = mMaxUsedIndex,
      .mBoundTextureCount = mBoundTextureCount,
      .mHash = mHash,
    };
  }




  void RendererDirectX11::RenderDrawCallTextures( const DrawCall* drawCall )
  {
    const BoundSRVs drawCallSRVs = BoundSRVs::DrawCallSRVs( drawCall );
    if( drawCallSRVs.mHash == mBoundSRVs.mHash )
      return;

    mBoundSRVs = drawCallSRVs;

    using PointerToSRVs = ID3D11ShaderResourceView**;

    const UINT StartSlot = 0;
    const UINT NumViews = mBoundSRVs.mMaxUsedIndex + 1;
    const PointerToSRVs ShaderResourceViews = mBoundSRVs.mBoundShaderResourceViews.data();

    mDeviceContext->VSSetShaderResources( StartSlot, NumViews, ShaderResourceViews );
    mDeviceContext->PSSetShaderResources( StartSlot, NumViews, ShaderResourceViews );
    mDeviceContext->GSSetShaderResources( StartSlot, NumViews, ShaderResourceViews );
  }

  void RendererDirectX11::RenderDrawCallPrimitiveTopology( const DrawCall* drawCall )
  {
    if( drawCall->mVertexCount == 0 &&
        drawCall->mIndexCount == 0 )
      return;

    const PrimitiveTopology newPrimitiveTopology
      = drawCall->mPrimitiveTopology == PrimitiveTopology::Unknown
      ? PrimitiveTopology::TriangleList
      : drawCall->mPrimitiveTopology;
    if( newPrimitiveTopology != mBoundPrimitiveTopology )
    {
      mBoundPrimitiveTopology = newPrimitiveTopology;
      const D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = GetPrimitiveTopology( drawCall->mPrimitiveTopology );
      mDeviceContext->IASetPrimitiveTopology( primitiveTopology );
    }
  }

  void RendererDirectX11::RenderDrawCallIssueDrawCommand( const DrawCall* drawCall )
  {

    // not quite convinced this assert should be here.
    // on one hand, it prevents u from forgetting to set index count. ( good )
    // on the other, it prevents u from setting start index/index count
    //   seperately from setting the index buffer ( bad? )
    TAC_ASSERT( !drawCall->mIndexBufferHandle.IsValid() || drawCall->mIndexCount );

    if( drawCall->mIndexCount )
    {
      const UINT IndexCount = drawCall->mIndexCount;
      const UINT StartIndexLocation = drawCall->mStartIndex; //  *indexBuffer->mFormat.CalculateTotalByteCount();
      // should this be multiplied by the sizeof vertex?
      const INT BaseVertexLocation = 0; // drawCall->mStartVertex;

      mDeviceContext->DrawIndexed( IndexCount, StartIndexLocation, BaseVertexLocation );
    }
    else if( drawCall->mVertexCount )
    {
      mDeviceContext->Draw( drawCall->mVertexCount, drawCall->mStartVertex );
    }
  }

  void RendererDirectX11::RenderDrawCall( const Frame* frame,
                                          const DrawCall* drawCall,
                                          Errors& errors )
  {
    RenderDrawCallViewAndUAV( frame, drawCall );
    RenderDrawCallShader( drawCall );
    RenderDrawCallBlendState( drawCall );
    RenderDrawCallDepthState( drawCall );
    RenderDrawCallIndexBuffer( drawCall );
    RenderDrawCallVertexBuffer( drawCall );
    RenderDrawCallRasterizerState( drawCall );
    RenderDrawCallSamplerState( drawCall );
    RenderDrawCallVertexFormat( drawCall );
    RenderDrawCallTextures( drawCall );
    RenderDrawCallPrimitiveTopology( drawCall );
    RenderDrawCallIssueDrawCommand( drawCall );
  }

  AssetPathStringView RendererDirectX11::GetShaderPath( const ShaderNameStringView& shaderName ) const
  {
    static String s;
    s = String() + "assets/hlsl/" + shaderName + ".hlsl";
    return s;
  }

  AssetPathStringView RendererDirectX11::GetShaderDir() const
  {
    return "assets/hlsl";
  }

  void RendererDirectX11::SwapBuffers()
  {
    if( gVerbose )
      OS::OSDebugPrintLine("SwapBuffers::Begin");

    for( int iWindow = 0; iWindow < mWindowCount; ++iWindow )
    {
      FramebufferHandle framebufferHandle = mWindows[ iWindow ];
      Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];
      if( !framebuffer->mSwapChain )
        continue;

      // Uhh..
      // https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
      // For flip presentation model swap chains that you create with the DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL value set,
      // a successful presentation unbinds back buffer 0 from the graphics pipeline,
      // except for when you pass the DXGI_PRESENT_DO_NOT_SEQUENCE flag in the Flags parameter.
      //
      // Unbinding it so dx doesnt give us a info message that it's unbinding it for us
      mDeviceContext->OMSetRenderTargets( 0, nullptr, nullptr );

        //ID3D11ShaderResourceView* nullViews[ 16 ] = {};
        //mDeviceContext->VSSetShaderResources( 0, 16, nullViews );
        //mDeviceContext->PSSetShaderResources( 0, 16, nullViews );
        //mDeviceContext->GSSetShaderResources( 0, 16, nullViews );
      framebuffer->mSwapChain->Present( 0, 0 );
    }

    if( gVerbose )
      OS::OSDebugPrintLine("SwapBuffers::End");
  }


  // Q: Should this function just return the clip space dimensions instead of A, B?
  OutProj RendererDirectX11::GetPerspectiveProjectionAB(InProj in)
  {
    const float f = in.mFar;
    const float n = in.mNear;
    TAC_ASSERT( f > n );

    // ( A, B ) maps ( -n, -f )vs to ( 0, 1 )ndc in directx
    // directx ndc : [ -1, 1 ][ -1, 1 ][  0, 1 ]
    // opengl  ndc : [ -1, 1 ][ -1, 1 ][ -1, 1 ]
    //
    // [ . 0 0  0 ] [ .    ] = [      .         ]         [          .            ]
    // [ 0 . 0  0 ] [ .    ] = [      .         ] /w_cs = [          .            ]
    // [ 0 0 A  B ] [ z_vs ] = [ z_vs * A + B   ]         [( z_vs * A + B )/-z_vs ]
    // [ 0 0 -1 0 ] [ 1    ] = [    -z_vs       ]         [          1            ]
    // ProjMtx      vs         cs                         ndc
    //
    //
    // z_ndc =    z_cs     /    w_cs       <-- ndc in terms of cs
    // z_ndc = ((z_vs)A+B) / ( -z_vs )     <-- ndc in terms of vs
    //                                         |
    // (0)ndc = (-n)vs                         |
    // (0)    = (( -n )A+B) / ( -(-n) ) <------+
    // B      = nA                             |
    //                                         |
    // (1)ndc = (-f)vs                         |
    // (1)    = (( -f )A+ B  ) / ( -(-f) ) <---+
    // 1      = (  -f  A+ B  ) /    f
    // 1      = (  -f  A+(nA)) /    f
    // f      =    -f  A+ nA
    // f      =  A(-f   + n )
    // A = f/(n-f)  <-- Solved for A
    // B = nf/(n-f) <-- Solved for B
    //

    const float a = f / ( n - f );
    const float b = ( n * f ) / ( n - f );

    // note that https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
    // uses -A and -B from ours, but their perspective matrix uses a 1 instead of our -1

    return OutProj
    {
      .mA = a,
      .mB = b,
    };
  }


  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------




  void RendererDirectX11::AddMagicBuffer( const CommandDataCreateMagicBuffer* commandDataCreateMagicBuffer,
                                          Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    D3D11_BUFFER_DESC desc = {};
    desc.BindFlags = GetBindFlags( commandDataCreateMagicBuffer->mBinding );
    desc.ByteWidth = commandDataCreateMagicBuffer->mByteCount;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = commandDataCreateMagicBuffer->mStride;
    desc.Usage = D3D11_USAGE_DEFAULT;

    // changed the api of this function, now callers need to set this.
    // ^ okay but like why
    //   i think its like what we were doing, was creating a resource that in one shader, was
    //   written to as a UAV, and in another shader,
    //   was read from as a SRV
    //   >> so i dont think its necessary
    TAC_ASSERT( desc.BindFlags & D3D11_BIND_SHADER_RESOURCE );

    MagicBuffer* magicBuffer = &mMagicBuffers[ ( int )commandDataCreateMagicBuffer->mMagicBufferHandle ];


    D3D11_SUBRESOURCE_DATA InitialData;
    InitialData.pSysMem = commandDataCreateMagicBuffer->mOptionalInitialBytes;
    InitialData.SysMemPitch = commandDataCreateMagicBuffer->mStride;
    InitialData.SysMemSlicePitch = commandDataCreateMagicBuffer->mByteCount;

    const D3D11_SUBRESOURCE_DATA *pInitialData
      = commandDataCreateMagicBuffer->mOptionalInitialBytes
      ? &InitialData
      : nullptr;

    TAC_DX11_CALL( mDevice->CreateBuffer( &desc, pInitialData, &magicBuffer->mBuffer ));
    SetDebugName( magicBuffer->mBuffer, commandDataCreateMagicBuffer->mStackFrame );

    // https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-object-structuredbuffer
    // The UAV format bound to this resource needs to be created with the DXGI_FORMAT_UNKNOWN format.
    const DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;

    const int NumElements = desc.ByteWidth / desc.StructureByteStride;

    if( desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS )
    {
      D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
      uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
      uavDesc.Format = Format;
      uavDesc.Buffer.FirstElement = 0; // index, not byte offset
      uavDesc.Buffer.Flags = 0;
      uavDesc.Buffer.NumElements = NumElements;
      TAC_DX11_CALL( mDevice->CreateUnorderedAccessView(
        magicBuffer->mBuffer,
        &uavDesc,
        &magicBuffer->mUAV ) );
      SetDebugName( magicBuffer->mUAV, commandDataCreateMagicBuffer->mStackFrame );
    }

    if( desc.BindFlags & D3D11_BIND_SHADER_RESOURCE )
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
      srvDesc.Format = Format;
      srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER; // D3D11_SRV_DIMENSION_BUFFEREX;
      srvDesc.Buffer.FirstElement = 0; // index, not byte offset
      srvDesc.Buffer.NumElements = NumElements;
      // srvDesc.BufferEx... = ;
      TAC_DX11_CALL( mDevice->CreateShaderResourceView( magicBuffer->mBuffer, &srvDesc, &magicBuffer->mSRV ));
      SetDebugName( magicBuffer->mSRV, commandDataCreateMagicBuffer->mStackFrame );
    }
  }

  void RendererDirectX11::AddVertexBuffer( const CommandDataCreateVertexBuffer* data,
                                           Errors& errors )
  {
    TAC_ASSERT( data->mStride );
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    TAC_ASSERT( !data->mOptionalInitialBytes || IsSubmitAllocated( data->mOptionalInitialBytes ) );

    const D3D11_BUFFER_DESC bd {
      .ByteWidth = (UINT)data->mByteCount,
      .Usage = GetUsage( data->mAccess ),
      .BindFlags = D3D11_BIND_VERTEX_BUFFER,
      .CPUAccessFlags = data->mAccess == Access::Dynamic ? D3D11_CPU_ACCESS_WRITE : (UINT)0,
    };
    const D3D11_SUBRESOURCE_DATA initData { .pSysMem = data->mOptionalInitialBytes, };
    const D3D11_SUBRESOURCE_DATA* pInitData = data->mOptionalInitialBytes ? &initData : nullptr;

    ID3D11Buffer* buffer;
    TAC_DX11_CALL( mDevice->CreateBuffer( &bd, pInitData, &buffer ));

    mVertexBuffers[ ( int )data->mVertexBufferHandle ] = VertexBuffer
    {
      .mBuffer = buffer,
      .mStride = ( UINT )data->mStride,
    };

    SetDebugName( buffer, data->mStackFrame );
  }

  void RendererDirectX11::AddVertexFormat( const CommandDataCreateVertexFormat* commandData,
                                           Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    VertexFormatHandle vertexFormatHandle = commandData->mVertexFormatHandle;

    // Let's say your shader doesn't require buffers.
    // So you go to create an input layout with no elements.
    // ID3D11Device::CreateInputLayout gives a error when passed a null pDesc because you
    // used a Vector.
    //
    // So you switch to using FixedVector, but then you get a warning that the input layout has
    // 0 elements.
    //
    // Turns out IASetInputLayout( NULL ) is just fine, so the Vector can stay.

    Vector< D3D11_INPUT_ELEMENT_DESC > inputElementDescs;

    for( int iVertexFormatData = 0;
         iVertexFormatData < commandData->mVertexDeclarations.size();
         ++iVertexFormatData )
    {
      const VertexDeclaration& curFormat = commandData->mVertexDeclarations[ iVertexFormatData ];

      D3D11_INPUT_ELEMENT_DESC curDX11Input = {};
      curDX11Input.Format = GetDXGIFormatTexture( curFormat.mTextureFormat );
      curDX11Input.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      curDX11Input.InstanceDataStepRate = 0;
      curDX11Input.InputSlot = 0;
      curDX11Input.SemanticName = GetSemanticName( curFormat.mAttribute );
      // MSDN:
      // A semantic index modifies a semantic, with an integer index number.
      // A semantic index is only needed in a case where there is more than
      // one element with the same semantic.
      curDX11Input.SemanticIndex;
      curDX11Input.AlignedByteOffset = curFormat.mAlignedByteOffset;
      inputElementDescs.push_back( curDX11Input );
    }
    ID3DBlob* inputSig = mPrograms[ ( int )commandData->mShaderHandle ].mInputSig;
    ID3D11InputLayout* inputLayout;
    TAC_DX11_CALL( mDevice->CreateInputLayout(
                   inputElementDescs.data(),
                   ( UINT )inputElementDescs.size(),
                   inputSig->GetBufferPointer(),
                   inputSig->GetBufferSize(),
                   &inputLayout ) );

    SetDebugName( inputLayout, commandData->mStackFrame );
    mInputLayouts[ ( int )vertexFormatHandle ] = inputLayout;
  }

  void RendererDirectX11::AddIndexBuffer( const CommandDataCreateIndexBuffer* data,
                                          Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    IndexBufferHandle index = data->mIndexBufferHandle;
    TAC_ASSERT( data->mFormat.mPerElementDataType == GraphicsType::uint );
    TAC_ASSERT( data->mFormat.mElementCount == 1 );
    TAC_ASSERT( data->mFormat.mPerElementByteCount == 2 ||
                data->mFormat.mPerElementByteCount == 4 );
    TAC_ASSERT( !data->mOptionalInitialBytes || IsSubmitAllocated( data->mOptionalInitialBytes ) );
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = data->mByteCount;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.Usage = GetUsage( data->mAccess );
    bd.CPUAccessFlags = data->mAccess == Access::Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data->mOptionalInitialBytes;
    ID3D11Buffer* buffer;
    TAC_DX11_CALL( mDevice->CreateBuffer(
                   &bd,
                   data->mOptionalInitialBytes ? &initData : nullptr,
                   &buffer ) );

    SetDebugName( buffer, data->mStackFrame );

    IndexBuffer* indexBuffer = &mIndexBuffers[ ( int )index ];
    indexBuffer->mFormat = data->mFormat;
    indexBuffer->mBuffer = buffer;
  }

  void RendererDirectX11::AddRasterizerState( const CommandDataCreateRasterizerState* commandData,
                                              Errors& errors )
  {
    const D3D11_RASTERIZER_DESC2 desc2
    {
      .FillMode = GetFillMode( commandData->mRasterizerState.mFillMode ),
      .CullMode = GetCullMode( commandData->mRasterizerState.mCullMode ),
      .FrontCounterClockwise = commandData->mRasterizerState.mFrontCounterClockwise,
      .DepthClipEnable = true,
      .ScissorEnable = commandData->mRasterizerState.mScissor,
      .MultisampleEnable = commandData->mRasterizerState.mMultisample,
      .ConservativeRaster
        = commandData->mRasterizerState.mConservativeRasterization
        ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON
        : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };

    ID3D11RasterizerState2* rasterizerState2;
    TAC_DX11_CALL( mDevice->CreateRasterizerState2( &desc2, &rasterizerState2 ) );
    SetDebugName( rasterizerState2, commandData->mStackFrame );
    mRasterizerStates[ ( int )commandData->mRasterizerStateHandle ] = rasterizerState2;
}

  void RendererDirectX11::AddSamplerState( const CommandDataCreateSamplerState* commandData,
                                           Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );

    const D3D11_SAMPLER_DESC desc =
    {
      .Filter = GetFilter( commandData->mSamplerState.mFilter ),
      .AddressU = GetAddressMode( commandData->mSamplerState.mU ),
      .AddressV = GetAddressMode( commandData->mSamplerState.mV ),
      .AddressW = GetAddressMode( commandData->mSamplerState.mW ),
      .ComparisonFunc = GetCompare( commandData->mSamplerState.mCompare ),
      .BorderColor = { 1, 0, 0, 1 },
    };

    ID3D11SamplerState* samplerStateDX11;
    TAC_DX11_CALL( mDevice->CreateSamplerState( &desc, &samplerStateDX11 ) );
    mSamplerStates[ ( int )commandData->mSamplerStateHandle ] = samplerStateDX11;
    SetDebugName( samplerStateDX11, commandData->mStackFrame );
  }

  void RendererDirectX11::AddShader( const CommandDataCreateShader* commandData, Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );

    TAC_CALL( DX11ShaderReloadFunction(
      commandData->mShaderHandle,
      commandData->mNameStringView,
      errors ) );

    TAC_CALL( ShaderReloadHelperAdd(
      commandData->mShaderHandle,
      commandData->mNameStringView,
      errors ) );
  }

  void RendererDirectX11::AddTexture( const CommandDataCreateTexture* oldData,
                                      Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );

   CommandDataCreateTexture newData = *oldData;
   const CommandDataCreateTexture* data = &newData;

    TexSpec& mTexSpec = newData.mTexSpec;
    if( mTexSpec.mImageBytes && !mTexSpec.mPitch )
    {
      mTexSpec.mPitch =
        mTexSpec.mImage.mWidth *
        mTexSpec.mImage.mFormat.CalculateTotalByteCount();
    }

    // D3D11_SUBRESOURCE_DATA structure
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476220(v=vs.85).aspx
    // You set SysMemPitch to the distance between any two adjacent pixels on different lines.
    // You set SysMemSlicePitch to the size of the entire 2D surface in bytes.
    D3D11_SUBRESOURCE_DATA subResources[ 6 ] = {};
    int                    subResourceCount = 0;

    if( data->mTexSpec.mImageBytes )
    {
      subResources[ subResourceCount++ ] = D3D11_SUBRESOURCE_DATA
      {
        .pSysMem = data->mTexSpec.mImageBytes,
        .SysMemPitch = ( UINT )data->mTexSpec.mPitch,
        .SysMemSlicePitch = ( UINT )data->mTexSpec.mPitch * data->mTexSpec.mImage.mHeight,
      };
    }

    for( const void* imageBytesCubemap : data->mTexSpec.mImageBytesCubemap )
    {
      if( !imageBytesCubemap )
        continue;

       subResources[ subResourceCount++ ] = D3D11_SUBRESOURCE_DATA
       {
         .pSysMem = imageBytesCubemap,
         .SysMemPitch = (UINT)data->mTexSpec.mPitch,
         .SysMemSlicePitch = (UINT) data->mTexSpec.mPitch * data->mTexSpec.mImage.mHeight,
       };
    }
    const bool isCubemap = subResourceCount == 6;
    D3D11_SUBRESOURCE_DATA* pInitialData = subResourceCount ? subResources : nullptr;

    const UINT MiscFlagsBinding = GetMiscFlags( data->mTexSpec.mBinding );
    const UINT MiscFlagsCubemap = isCubemap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

    // remove this assert if we ever use texture arrays of arbitary size
    TAC_ASSERT( subResourceCount == 0 || // empty texture
                subResourceCount == 1 || // single texture
                subResourceCount == 6 ); // cubemap texture

    const int dimension
      = ( data->mTexSpec.mImage.mWidth ? 1 : 0 )
      + ( data->mTexSpec.mImage.mHeight ? 1 : 0 )
      + ( data->mTexSpec.mImage.mDepth ? 1 : 0 );

    TAC_ASSERT( dimension );

    const DXGI_FORMAT Format = GetDXGIFormatTexture( data->mTexSpec.mImage.mFormat );
    const DXGI_FORMAT FormatTexture2D = [ & ]()
    {
      if( ( int )( data->mTexSpec.mBinding & Binding::DepthStencil ) )
        return GetDXGIFormatTextureTypeless( data->mTexSpec.mImage.mFormat.mPerElementByteCount );
      return Format;
    } ( );

    const DXGI_SAMPLE_DESC SampleDesc = { .Count = 1 };

    ID3D11Texture2D* texture2D = nullptr;
    if( dimension == 2 )
    {
      const D3D11_TEXTURE2D_DESC texDesc =
      {
        .Width = (UINT)data->mTexSpec.mImage.mWidth,
        .Height = (UINT)data->mTexSpec.mImage.mHeight,
        .MipLevels = 1,
        .ArraySize = (UINT)Max( 1, subResourceCount ),
        .Format = FormatTexture2D,
        .SampleDesc= SampleDesc,
        .Usage = GetUsage( data->mTexSpec.mAccess ),
        .BindFlags = GetBindFlags( data->mTexSpec.mBinding ),
        .CPUAccessFlags = GetCPUAccessFlags( data->mTexSpec.mCpuAccess ),
        .MiscFlags = MiscFlagsBinding | MiscFlagsCubemap,
      };
      TAC_DX11_CALL( mDevice->CreateTexture2D(
                     &texDesc,
                     pInitialData,
                     &texture2D ) );
      SetDebugName( texture2D, data->mStackFrame );
    }

    ID3D11Texture3D* texture3D = nullptr;
    if( dimension == 3 )
    {
      const D3D11_TEXTURE3D_DESC texDesc =
      {
        .Width = (UINT)data->mTexSpec.mImage.mWidth,
        .Height = (UINT)data->mTexSpec.mImage.mHeight,
        .Depth = (UINT)data->mTexSpec.mImage.mDepth,
        .MipLevels = 1,
        .Format = Format,
        .Usage = GetUsage( data->mTexSpec.mAccess ),
        .BindFlags = GetBindFlags( data->mTexSpec.mBinding ),
        .CPUAccessFlags = GetCPUAccessFlags( data->mTexSpec.mCpuAccess ),
        .MiscFlags = MiscFlagsBinding | MiscFlagsCubemap,
      };

      TAC_DX11_CALL( mDevice->CreateTexture3D( &texDesc, pInitialData, &texture3D ));
      SetDebugName( texture3D, data->mStackFrame );
    }

    ID3D11Resource* resource
      = texture2D ? ( ID3D11Resource* )texture2D
      : texture3D ? ( ID3D11Resource* )texture3D : nullptr;

    ID3D11RenderTargetView* rtv = nullptr;
    if( ( int )data->mTexSpec.mBinding & ( int )Binding::RenderTarget )
    {
      TAC_DX11_CALL( mDevice->CreateRenderTargetView( resource, nullptr, &rtv ) );
      SetDebugName( rtv, data->mStackFrame );
    }

    ID3D11UnorderedAccessView* uav = nullptr;
    if( ( int )data->mTexSpec.mBinding & ( int )Binding::UnorderedAccess )
    {
      D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
      uavDesc.Format = Format;

      if( dimension == 2 )
      {
        const D3D11_TEX2D_UAV Texture2D = {};

        uavDesc.Texture2D = Texture2D;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

      }
      else if( dimension == 3 )
      {
        const D3D11_TEX3D_UAV Texture3D =
        {
          .MipSlice = 0,
          .FirstWSlice = 0,
          .WSize = (UINT)data->mTexSpec.mImage.mDepth,
        };

        uavDesc.Texture3D = Texture3D;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
      }
      else
      {
        TAC_ASSERT_INVALID_CODE_PATH;
      }

      TAC_DX11_CALL( mDevice->CreateUnorderedAccessView( resource, &uavDesc, &uav ) );
      SetDebugName( uav, data->mStackFrame );
    }

    ID3D11ShaderResourceView* srv = nullptr;
    if( ( int )data->mTexSpec.mBinding & ( int )Binding::ShaderResource )
    {
      const D3D_SRV_DIMENSION srvDimension
        = isCubemap ? D3D11_SRV_DIMENSION_TEXTURECUBE
        : dimension == 2 ? D3D11_SRV_DIMENSION_TEXTURE2D
        : dimension == 3 ? D3D11_SRV_DIMENSION_TEXTURE3D
        : D3D_SRV_DIMENSION_UNKNOWN;

      if( srvDimension != D3D_SRV_DIMENSION_UNKNOWN )
      {
        const D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc =
        {
          .Format = Format,
          .ViewDimension = srvDimension,
          .Texture2D = D3D11_TEX2D_SRV{.MipLevels = 1 },
        };

        TAC_DX11_CALL( mDevice->CreateShaderResourceView( resource, &srvDesc, &srv ) );
        SetDebugName( srv, data->mStackFrame );
      }

      // why check if its a render target?
      if( ( int )data->mTexSpec.mBinding & ( int )Binding::RenderTarget )
        mDeviceContext->GenerateMips( srv );
    }

    ID3D11DepthStencilView* dsv = nullptr;
    if( ( int )data->mTexSpec.mBinding & ( int )Binding::DepthStencil )
    {
      D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
      depthStencilViewDesc.Format = GetDXGIFormatDepth( data->mTexSpec.mImage.mFormat.mPerElementByteCount );
      depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
      TAC_DX11_CALL( mDevice->CreateDepthStencilView(
                     resource,
                     &depthStencilViewDesc,
                     &dsv ) );
      SetDebugName( dsv, data->mStackFrame );
    }

    Texture* texture = &mTextures[ ( int )data->mTextureHandle ];
    texture->mTexture2D = texture2D;
    texture->mTexture3D = texture3D;
    texture->mTextureSRV = srv;
    texture->mTextureRTV = rtv;
    texture->mTextureUAV = uav;
    texture->mTextureDSV = dsv;
  }

  void RendererDirectX11::AddBlendState( const CommandDataCreateBlendState* commandData,
                                         Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    const BlendState* blendState = &commandData->mBlendState;

    const D3D11_RENDER_TARGET_BLEND_DESC blendDesc
    {
      .BlendEnable = true,
      .SrcBlend = GetBlend( blendState->mSrcRGB ),
      .DestBlend = GetBlend( blendState->mDstRGB ),
      .BlendOp = GetBlendOp( blendState->mBlendRGB ),
      .SrcBlendAlpha = GetBlend( blendState->mSrcA ),
      .DestBlendAlpha = GetBlend( blendState->mDstA ),
      .BlendOpAlpha = GetBlendOp( blendState->mBlendA ),
      .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
    };

    const D3D11_BLEND_DESC desc = { .RenderTarget = { blendDesc } };

    ID3D11BlendState* blendStateDX11;
    TAC_DX11_CALL( mDevice->CreateBlendState( &desc, &blendStateDX11 ) );
    SetDebugName( blendStateDX11, commandData->mStackFrame );

    mBlendStates[ commandData->mBlendStateHandle.GetIndex() ] = blendStateDX11;
  }

  void RendererDirectX11::AddConstantBuffer( const CommandDataCreateConstantBuffer* commandData,
                                             Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    TAC_ASSERT( commandData->mName );
    TAC_ASSERT( !FindCbufferOfName( commandData->mName ).IsValid() );

    const int iCB = commandData->mConstantBufferHandle.GetIndex();
    const D3D11_BUFFER_DESC bd =
    {
      .ByteWidth = (UINT)RoundUpToNearestMultiple( commandData->mByteCount, 16 ),
      .Usage = D3D11_USAGE_DYNAMIC, // i guess?
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE, // i guess?
    };

    ID3D11Buffer* cbufferhandle;
    TAC_DX11_CALL( mDevice->CreateBuffer( &bd, nullptr, &cbufferhandle ) );
    SetDebugName( cbufferhandle, commandData->mStackFrame );

    mConstantBuffers[ iCB ] = ConstantBuffer
    {
      .mBuffer = cbufferhandle,
      .mName = commandData->mName,
    };
  }

  void RendererDirectX11::AddDepthState( const CommandDataCreateDepthState* commandData,
                                         Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    const D3D11_DEPTH_WRITE_MASK DepthWriteMask
      = commandData->mDepthState.mDepthWrite
      ? D3D11_DEPTH_WRITE_MASK_ALL
      : D3D11_DEPTH_WRITE_MASK_ZERO;

    const D3D11_COMPARISON_FUNC DepthFunc = GetDepthFunc( commandData->mDepthState.mDepthFunc );

    const D3D11_DEPTH_STENCIL_DESC desc =
    {
      .DepthEnable = commandData->mDepthState.mDepthTest,
      .DepthWriteMask = DepthWriteMask,
      .DepthFunc = DepthFunc,
    };

    const DepthStateHandle hDepth = commandData->mDepthStateHandle;
    const int iDepth = hDepth.GetIndex();

    ID3D11DepthStencilState* depthStencilState;
    TAC_DX11_CALL( mDevice->CreateDepthStencilState( &desc, &depthStencilState ) );
    mDepthStencilStates[ iDepth ] = depthStencilState;
    SetDebugName( depthStencilState, commandData->mStackFrame );
  }

  void RendererDirectX11::AddFramebufferForRenderToTexture( const CommandDataCreateFramebuffer* data,
                                                            Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );

    TAC_ASSERT( !data->mFramebufferTextures.empty() );

    ID3D11DepthStencilView* dsv = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D*        depthTexture = nullptr;

    for( TextureHandle textureHandle : data->mFramebufferTextures )
    {
      TAC_ASSERT( textureHandle.IsValid() );
      Texture* texture = &mTextures[ ( int )textureHandle ];

      D3D11_TEXTURE2D_DESC desc;
      texture->mTexture2D->GetDesc( &desc );

#if 0
      const bool isDepthTexture =
        desc.Format == DXGI_FORMAT_D16_UNORM ||
        desc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
        desc.Format == DXGI_FORMAT_D32_FLOAT ||
        desc.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
      if( isDepthTexture )
      {
        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        depthStencilViewDesc.Format = desc.Format;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        TAC_DX11_CALL( mDevice->CreateDepthStencilView(
                       texture->mTexture2D,
                       &depthStencilViewDesc,
                       &dsv ) );
        SetDebugName( dsv, data->mStackFrame );

        depthTexture = texture->mTexture2D;
      }
  #else
      if( texture->mTextureDSV )
      {
        dsv = texture->mTextureDSV;
      }
  #endif
      else
      {
        rtv = texture->mTextureRTV;
      }
    }

    const String debugName = String()
      + data->mStackFrame.mFile + ":" + Tac::ToString( data->mStackFrame.mLine );

    Framebuffer* framebuffer = &mFramebuffers[ ( int )data->mFramebufferHandle ];
    framebuffer->mDebugName = debugName;
    framebuffer->mRenderTargetView = rtv;
    framebuffer->mDepthStencilView = dsv;
    framebuffer->mDepthTexture = depthTexture;
  }

  void RendererDirectX11::AddFramebufferForWindow( const CommandDataCreateFramebuffer* data,
                                                   Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );

    TAC_ASSERT( data->mFramebufferTextures.empty() );
    TAC_ASSERT( data->mNativeWindowHandle && data->mWidth && data->mHeight );

    const auto hwnd = ( HWND )data->mNativeWindowHandle;
    const int bufferCount = 4;
    const UINT width = data->mWidth;
    const UINT height = data->mHeight;

    const SwapChainCreateInfo scInfo
    {
      .mHwnd = hwnd,
      .mDevice = (IUnknown*)mDevice,
      .mBufferCount = bufferCount,
      .mWidth = data->mWidth,
      .mHeight = data->mWidth,
    };
    auto swapChain = TAC_CALL( DXGICreateSwapChain( scInfo, errors ));


    // this seems to be unused...
    //
    // DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    // swapChain->GetDesc( &swapChainDesc );

    ID3D11Texture2D* pBackBuffer = nullptr;
    TAC_DXGI_CALL( swapChain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) ) );
    TAC_RAISE_ERROR_IF( !pBackBuffer, "no buffer to resize" );
    TAC_ON_DESTRUCT(pBackBuffer->Release());

    ID3D11RenderTargetView* rtv = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr;
    TAC_DX11_CALL( mDevice->CreateRenderTargetView( pBackBuffer, rtvDesc, &rtv ) );
    SetDebugName( rtv, data->mStackFrame );

    // this seems to be unused...
    //
    // D3D11_RENDER_TARGET_VIEW_DESC createdDesc = {};
    // rtv->GetDesc( &createdDesc );

    const DXGI_SAMPLE_DESC SampleDesc =
    {
      .Count = 1,
      .Quality = 0,
    };

    const D3D11_TEXTURE2D_DESC texture2dDesc =
    {
      .Width = width,
      .Height = height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .SampleDesc = SampleDesc,
      .BindFlags = D3D11_BIND_DEPTH_STENCIL,
    };

    ID3D11Texture2D* texture;
    TAC_DX11_CALL( mDevice->CreateTexture2D( &texture2dDesc, nullptr, &texture ) );
    SetDebugName( texture, data->mStackFrame );

    const D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc =
    {
      .Format = texture2dDesc.Format,
      .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
    };

    ID3D11DepthStencilView* dsv;
    TAC_DX11_CALL( mDevice->CreateDepthStencilView( texture, &depthStencilViewDesc, &dsv ));
    SetDebugName( dsv, data->mStackFrame );


    const FramebufferHandle hFB = data->mFramebufferHandle;
    const int iFB = hFB.GetIndex();

    const String debugName = String()
      + data->mStackFrame.mFile + ":" + Tac::ToString( data->mStackFrame.mLine );
    mFramebuffers[iFB] = Framebuffer
    {
      .mBufferCount = bufferCount,
      .mSwapChain = Tac::move(swapChain),
      .mDepthStencilView = dsv,
      .mRenderTargetView = rtv,
      .mDepthTexture = texture,
      .mHwnd = hwnd,
      .mDebugName = debugName,
    };

    mWindows[ mWindowCount++ ] = hFB;
  }

  void RendererDirectX11::AddFramebuffer( const CommandDataCreateFramebuffer* data,
                                          Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );

    if( data->mNativeWindowHandle && data->mWidth && data->mHeight )
    {
      AddFramebufferForWindow( data, errors );
      return;
    }

    if( !data->mFramebufferTextures.empty() )
    {
      AddFramebufferForRenderToTexture( data, errors );
      return;
    }

    TAC_ASSERT_INVALID_CODE_PATH;
  }

  void RendererDirectX11::RemoveVertexBuffer( VertexBufferHandle vertexBufferHandle, Errors& )
  {
    VertexBuffer* vertexBuffer = &mVertexBuffers[ ( int )vertexBufferHandle ];
    TAC_RELEASE_IUNKNOWN( vertexBuffer->mBuffer );
    *vertexBuffer = VertexBuffer();
  }

  void RendererDirectX11::RemoveVertexFormat( VertexFormatHandle vertexFormatHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mInputLayouts[ ( int )vertexFormatHandle ] );
  }

  void RendererDirectX11::RemoveIndexBuffer( IndexBufferHandle indexBufferHandle, Errors& )
  {
    IndexBuffer* indexBuffer = &mIndexBuffers[ ( int )indexBufferHandle ];
    TAC_RELEASE_IUNKNOWN( indexBuffer->mBuffer );
    *indexBuffer = IndexBuffer();
  }

  void RendererDirectX11::RemoveRasterizerState( RasterizerStateHandle rasterizerStateHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mRasterizerStates[ ( int )rasterizerStateHandle ] );
  }

  void RendererDirectX11::RemoveSamplerState( SamplerStateHandle samplerStateHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mSamplerStates[ ( int )samplerStateHandle ] );
  }

  void RendererDirectX11::RemoveShader( const ShaderHandle shaderHandle, Errors& )
  {
    DX11Program* program = &mPrograms[ ( int )shaderHandle ];
    TAC_RELEASE_IUNKNOWN( program->mInputSig );
    TAC_RELEASE_IUNKNOWN( program->mVertexShader );
    TAC_RELEASE_IUNKNOWN( program->mPixelShader );
    TAC_RELEASE_IUNKNOWN( program->mGeometryShader );
    ShaderReloadHelperRemove( shaderHandle );
  }

  void RendererDirectX11::RemoveTexture( TextureHandle textureHandle, Errors& )
  {
    Texture* texture = &mTextures[ ( int )textureHandle ];
    TAC_RELEASE_IUNKNOWN( texture->mTexture2D );
    TAC_RELEASE_IUNKNOWN( texture->mTexture3D );
    TAC_RELEASE_IUNKNOWN( texture->mTextureRTV );
    TAC_RELEASE_IUNKNOWN( texture->mTextureSRV );
    *texture = {};
  }

  void RendererDirectX11::RemoveMagicBuffer( const MagicBufferHandle hBuf, Errors& )
  {
    FindMagicBuffer( hBuf )->clear();
  }

  void RendererDirectX11::RemoveFramebuffer( FramebufferHandle framebufferHandle, Errors& )
  {
    for( int i = 0; i < mWindowCount; ++i )
      if( mWindows[ i ] == framebufferHandle )
        mWindows[ i ] = mWindows[ --mWindowCount ];
    Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];

    //ReportLiveObjects();

    // Window framebuffers own their things
    if( framebuffer->mSwapChain )
    {
      TAC_RELEASE_IUNKNOWN( framebuffer->mDepthStencilView );
      TAC_RELEASE_IUNKNOWN( framebuffer->mDepthTexture );
      TAC_RELEASE_IUNKNOWN( framebuffer->mRenderTargetView );
    }

    *framebuffer = Framebuffer();
  }

  void RendererDirectX11::RemoveBlendState( BlendStateHandle blendStateHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mBlendStates[ ( int )blendStateHandle ] );
  }

  void RendererDirectX11::RemoveConstantBuffer( ConstantBufferHandle hBuf, Errors& )
  {

    FindConstantBuffer( hBuf )->clear();
  }

  void RendererDirectX11::RemoveDepthState( DepthStateHandle depthStateHandle, Errors& )
  {
    TAC_RELEASE_IUNKNOWN( mDepthStencilStates[ ( int )depthStateHandle ] );
  }

  void RendererDirectX11::UpdateTextureRegion( const CommandDataUpdateTextureRegion* commandData,
                                               Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    const TexUpdate* data = &commandData->mTexUpdate;
    TAC_ASSERT( IsSubmitAllocated( data->mSrcBytes ) );

    const UINT dstX = data->mDstX;
    const UINT dstY = data->mDstY;
    const UINT dstZ = 0;

    D3D11_BOX srcBox = {};
    srcBox.right = data->mSrc.mWidth;
    srcBox.bottom = data->mSrc.mHeight;
    srcBox.back = 1;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = data->mSrc.mWidth;
    texDesc.Height = data->mSrc.mHeight;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = GetDXGIFormatTexture( data->mSrc.mFormat );
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = 0;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = data->mSrcBytes;
    subResource.SysMemPitch = data->mPitch;
    subResource.SysMemSlicePitch = data->mPitch * data->mSrc.mHeight;

    ID3D11Resource* dstTex = mTextures[ ( int )commandData->mTextureHandle ].mTexture2D;
    TAC_ASSERT( dstTex );
    ID3D11Texture2D* srcTex;
    TAC_DX11_CALL( mDevice->CreateTexture2D( &texDesc, &subResource, &srcTex ));

    mDeviceContext->CopySubresourceRegion( dstTex,
                                           0, // dst subresource
                                           dstX,
                                           dstY,
                                           dstZ,
                                           srcTex,
                                           0, // src subresource,
                                           &srcBox );
    TAC_RELEASE_IUNKNOWN( srcTex );
  }

  void RendererDirectX11::UpdateVertexBuffer( const CommandDataUpdateVertexBuffer* commandData,
                                              Errors& errors )
  {
    ID3D11Buffer* buffer = mVertexBuffers[ ( int )commandData->mVertexBufferHandle ].mBuffer;
    UpdateBuffer( buffer, commandData->mBytes, commandData->mByteCount, errors );
  }

  void RendererDirectX11::UpdateConstantBuffer( const CommandDataUpdateConstantBuffer* commandData,
                                                Errors& errors )
  {
    const ConstantBuffer* constantBuffer = FindConstantBuffer( commandData->mConstantBufferHandle );
    TAC_ASSERT(constantBuffer);
    UpdateBuffer( constantBuffer->mBuffer,
                  commandData->mBytes,
                  commandData->mByteCount,
                  errors );

  }

  void RendererDirectX11::UpdateIndexBuffer( const CommandDataUpdateIndexBuffer* commandData,
                                             Errors& errors )
  {
    ID3D11Buffer* buffer = mIndexBuffers[ ( int )commandData->mIndexBufferHandle ].mBuffer;
    UpdateBuffer( buffer, commandData->mBytes, commandData->mByteCount, errors );
  }

  void RendererDirectX11::ResizeFramebuffer( const CommandDataResizeFramebuffer* data,
                                             Errors& errors )
  {
    Framebuffer* framebuffer = &mFramebuffers[ ( int )data->mFramebufferHandle ];
    TAC_ASSERT( framebuffer->mSwapChain );

    D3D11_TEXTURE2D_DESC depthTextureDesc;
    framebuffer->mDepthTexture->GetDesc( &depthTextureDesc );

    depthTextureDesc.Width = data->mWidth;
    depthTextureDesc.Height = data->mHeight;

    // Release outstanding back buffer references prior to calling IDXGISwapChain::ResizeBuffers
    TAC_RELEASE_IUNKNOWN( framebuffer->mRenderTargetView );
    TAC_RELEASE_IUNKNOWN( framebuffer->mDepthTexture );
    TAC_RELEASE_IUNKNOWN( framebuffer->mDepthStencilView );

    DXGI_SWAP_CHAIN_DESC desc;
    TAC_RAISE_ERROR_IF( FAILED( framebuffer->mSwapChain->GetDesc( &desc ) ),
                        "Failed to get swap chain desc" );

    framebuffer->mSwapChain->ResizeBuffers( framebuffer->mBufferCount,
                                            data->mWidth,
                                            data->mHeight,
                                            DXGI_FORMAT_UNKNOWN,
                                            desc.Flags );
    ID3D11Texture2D* pBackBuffer = nullptr;
    TAC_DXGI_CALL( framebuffer->mSwapChain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) ));
    TAC_RAISE_ERROR_IF( !pBackBuffer, "no buffer to resize" );

    const D3D11_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr;

    ID3D11RenderTargetView* rtv = nullptr;
    TAC_DX11_CALL( mDevice->CreateRenderTargetView( pBackBuffer, rtvDesc, &rtv ));

    TAC_RELEASE_IUNKNOWN( pBackBuffer );
    SetDebugName( rtv, framebuffer->mDebugName );

    ID3D11Texture2D* depthTexture;
    TAC_DX11_CALL( mDevice->CreateTexture2D( &depthTextureDesc, nullptr, &depthTexture ));
    SetDebugName( depthTexture, framebuffer->mDebugName );

    const D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc =
    {
      .Format = depthTextureDesc.Format,
      .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
    };

    ID3D11DepthStencilView* dsv;
    TAC_DX11_CALL( mDevice->CreateDepthStencilView( depthTexture, &depthStencilViewDesc, &dsv ));
    SetDebugName( dsv, framebuffer->mDebugName );

    framebuffer->mDepthStencilView = dsv;
    framebuffer->mDepthTexture = depthTexture;
    framebuffer->mRenderTargetView = rtv;
  }

  void RendererDirectX11::SetRenderObjectDebugName( const CommandDataSetRenderObjectDebugName* data,
                                                    Errors& errors )
  {
    if( Framebuffer* framebuffer = FindFramebuffer( data->mFramebufferHandle ) )
    {
      SetDebugName( framebuffer->mDepthStencilView, data->mName );
      SetDebugName( framebuffer->mRenderTargetView, data->mName );
      SetDebugName( framebuffer->mDepthTexture, data->mName );
      SetDebugName( (IDXGISwapChain4*)framebuffer->mSwapChain, data->mName );
      framebuffer->mDebugName = data->mName;
    }

    if( VertexBuffer* vb = FindVertexBuffer( data->mVertexBufferHandle ) )
      SetDebugName( vb->mBuffer, data->mName );

    if( IndexBuffer* ib = FindIndexBuffer( data->mIndexBufferHandle ) )
      SetDebugName( ib->mBuffer, data->mName );

    if( Texture* texture = FindTexture( data->mTextureHandle ))
    {
      SetDebugName( texture->mTexture2D, data->mName );
      SetDebugName( texture->mTexture3D, data->mName );
      SetDebugName( texture->mTextureSRV, data->mName );
      SetDebugName( texture->mTextureRTV, data->mName );
      SetDebugName( texture->mTextureUAV, data->mName );
    }

    if( data->mRasterizerStateHandle.IsValid() )
      SetDebugName( mRasterizerStates[ ( int )data->mRasterizerStateHandle ], data->mName );

    if( data->mBlendStateHandle.IsValid() )
      SetDebugName( mBlendStates[ ( int )data->mBlendStateHandle ], data->mName );

    if( data->mDepthStateHandle.IsValid() )
      SetDebugName( mDepthStencilStates[ ( int )data->mDepthStateHandle ], data->mName );

    if( data->mSamplerStateHandle.IsValid() )
      SetDebugName( mSamplerStates[ ( int )data->mSamplerStateHandle ], data->mName );

    if( data->mVertexFormatHandle.IsValid() )
      SetDebugName( mInputLayouts[ ( int )data->mVertexFormatHandle ], data->mName );

    if( ConstantBuffer* cb = FindConstantBuffer(data->mConstantBufferHandle ) ) 
      SetDebugName( cb->mBuffer, data->mName );

    if( MagicBuffer* magicBuffer = FindMagicBuffer(data->mMagicBufferHandle) )
    {
      SetDebugName( magicBuffer->mBuffer, data->mName );
      SetDebugName( magicBuffer->mSRV, data->mName );
      SetDebugName( magicBuffer->mUAV, data->mName );
    }
  }

  void RendererDirectX11::UpdateBuffer( ID3D11Buffer* buffer,
                                        const void* bytes,
                                        int byteCount,
                                        Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    TAC_ASSERT( IsSubmitAllocated( bytes ) );
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    TAC_DX11_CALL( mDeviceContext->Map( buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource ));
    MemCpy( mappedResource.pData, bytes, byteCount );
    mDeviceContext->Unmap( buffer, 0 );
  }

  // -----------------------------------------------------------------------------------------------

  DX11Program* RendererDirectX11::FindProgram( const ShaderHandle hShader )
  {
    return hShader.IsValid() ? &mPrograms[ hShader.GetIndex() ] : nullptr;
  }

  Framebuffer* RendererDirectX11::FindFramebuffer( const FramebufferHandle hFB ) 
  {
    return hFB.IsValid() ? &mFramebuffers[ hFB.GetIndex() ] : nullptr;
  }

  Texture* RendererDirectX11::FindTexture( const TextureHandle hTex )
  {
    return hTex.IsValid() ? &mTextures[ hTex.GetIndex() ] : nullptr;
  }

  IndexBuffer* RendererDirectX11::FindIndexBuffer( const IndexBufferHandle hBuf )
  {
    return hBuf.IsValid() ? &mIndexBuffers[ hBuf.GetIndex() ] : nullptr;
  }

  VertexBuffer* RendererDirectX11::FindVertexBuffer( const VertexBufferHandle hBuf )
  {
    return hBuf.IsValid() ? &mVertexBuffers[ hBuf.GetIndex() ] : nullptr;
  }

  MagicBuffer* RendererDirectX11::FindMagicBuffer( const MagicBufferHandle hBuf )
  {
    return hBuf.IsValid() ? &mMagicBuffers[ hBuf.GetIndex() ] : nullptr;
  }

  ConstantBuffer* RendererDirectX11::FindConstantBuffer( const ConstantBufferHandle hBuf)
  {
    return hBuf.IsValid() ? &mConstantBuffers[ hBuf.GetIndex() ] : nullptr;
  }

  // -----------------------------------------------------------------------------------------------

  void RendererDirectX11::DebugGroupBegin( StringView desc )
  {
    if( !mUserAnnotationDEBUG )
      return;
    TAC_ASSERT( !desc.empty() );
    const TransientWchar descWchar( desc );
    mUserAnnotationDEBUG->BeginEvent( descWchar.data() );
  }

  void RendererDirectX11::DebugMarker( StringView desc )
  {
    if( !mUserAnnotationDEBUG )
      return;
    TAC_ASSERT( !desc.empty() );
    const TransientWchar descWchar( desc );
    mUserAnnotationDEBUG->SetMarker( descWchar.data() );
  }

  void RendererDirectX11::DebugGroupEnd()
  {

    if( mUserAnnotationDEBUG )
      mUserAnnotationDEBUG->EndEvent();

  }

  // -----------------------------------------------------------------------------------------------

  void MagicBuffer::clear()
  {
    TAC_RELEASE_IUNKNOWN( mBuffer );
    TAC_RELEASE_IUNKNOWN( mUAV );
    TAC_RELEASE_IUNKNOWN( mSRV );
  }

  void MagicBuffer::SetDebugName( const StringView& name )
  {
      Render::SetDebugName( mBuffer, name );
      Render::SetDebugName( mSRV, name );
      Render::SetDebugName( mUAV, name );
  }

  void ConstantBuffer::clear()
  {
    mName.clear();
    TAC_RELEASE_IUNKNOWN( mBuffer );
  }

} // namespace Tac::Render
