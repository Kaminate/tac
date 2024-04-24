#include "tac_render.h" // self-inc

#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"

#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-rhi/render/tac_render_backend.h"
#include "tac-rhi/renderer/command/tac_render_command_type.h"
#include "tac-rhi/render/tac_render_handle.h"
#include "tac-rhi/render/tac_render_context_data.h"
#include "tac-rhi/render/tac_render_backend.h"
#include "tac-rhi/render/tac_render_backend_cmd_data.h"

//#if !TAC_TEMPORARILY_DISABLED()
//#include "tac-win32/dx/dx12/tac_dx12_context_manager.h"
//#endif

import std; // mutex

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  static int sMaxGPUFrameCount;

  //struct CommandBuffer
  //{
  //  void Allocate( void* bytes, int byteCount )
  //  {
  //    TAC_SCOPE_GUARD( std::lock_guard, m );
  //    int i = mData.size();
  //    mData.resize( i + byteCount );
  //    MemCpy( &mData[ i ], bytes, byteCount );
  //  }

  //private:
  //  std::mutex m;
  //  Vector< char > mData;
  //};

  //static CommandBuffer sCmdBufFrameBegin;
  //static CommandBuffer sCmdBufFrameEnd;

  //thread_local CommandRecorder sCommandRecorder;

  //void CommandRecorder::CreateIB( const BufferParams& params )
  //{
  //  sCmdBufFrameBegin.Allocate( ( void* )&params, sizeof( BufferParams ) );
  //}


  static void ExecuteContext( ContextHandle& h, Errors& errors )
  {
    BackendCmdDeserializer it( h.mData );
    IBackend* backend = IBackend::Get();
    while( it.IsValid() )
    {

      const CommandType2 commandType { it.PopCommandType() };
      TAC_ASSERT_INDEX( commandType, CommandType2::kCount );

      switch( commandType )
      {
      case CommandType2::kCreateDynamicBuffer:
      {
        const DynBufCreateParams params { it.PopDynBufCreateParams() };
        TAC_CALL( backend->CreateDynamicBuffer2( params, errors ) );
      } break;

      case CommandType2::kUpdateDynamicBuffer:
      {
        const DynBufUpdateParams params { it.PopDynBufUpdateParams() };
        backend->UpdateDynamicBuffer2( params );
      } break;

      case CommandType2::kSetName:
      {
        const SetRenderObjectNameParams params { it.PopSetRenderObjectNameParams() };
        backend->SetRenderObjectName( params );
      } break;

      default: TAC_ASSERT_INVALID_CASE( commandType );
      }
    }
  }



} // namespace Tac::Render

namespace Tac
{
  void Render::Init2( const InitParams params, Errors& errors )
  {
    sMaxGPUFrameCount = params.mMaxGPUFrameCount;

    IBackend* backend = IBackend::Get();
    backend->Init( errors );
  }

  int Render::GetMaxGPUFrameCount() { return sMaxGPUFrameCount; }

  Render::ContextHandle     Render::CreateContext( Errors& errors )
  {
    auto ch = AllocRenderHandleT< ContextHandle >();

    IBackend* backend = IBackend::Get();

    SmartPtr< ICommandList > cmdList = backend->GetCommandList( ch, errors );
    SmartPtr< BackendCmdData > data = TAC_NEW BackendCmdData; 

    static Vector< SmartPtr< BackendCmdData > > sBackendCmdDatas;
    static Vector< SmartPtr< ICommandList > > sCmdLists;
    const int i = ch.GetHandleIndex();
    static int n;
    {
      static std::mutex sMutex;
      TAC_SCOPE_GUARD( std::lock_guard, sMutex );
      if( !( i < n ) )
      {
        n = i + 1;
        sBackendCmdDatas.resize( n );
        sCmdLists.resize( n );
      }
    }

    sBackendCmdDatas[ i ] = data;
    sCmdLists[ i ] = cmdList;

    ch.mData = data.Get();
    ch.mCmdList = cmdList.Get();

    return ch;
  }

  Render::ViewHandle2       Render::CreateView2()
  {
    auto ch = AllocRenderHandleT< ViewHandle2 >();

    static Vector< ViewData > sViewData;
    const int i = ch.GetHandleIndex();
    static int n;
    {
      static std::mutex sMutex;
      TAC_SCOPE_GUARD( std::lock_guard, sMutex );
      if( !( i < n ) )
      {
        n = i + 1;
        sViewData.resize( n );
      }
    }

    ViewData* data = &sViewData[ i ];
    ch.SetViewData( data );
    return ch;
  }

  void Render::ExecuteCommands( Span< ContextHandle > contexts, Errors& errors )
  {
    for( ContextHandle& h : contexts )
    {
      TAC_CALL( ExecuteContext( h, errors ) );
    }
  }

#if 0
  void Render::FrameEnd()
  {
    // execute cmd lists, swap buffers
    OS::OSDebugBreak();
  }
#endif


} // namespace Tac

