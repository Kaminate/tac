#pragma once

#include "tac-std-lib/memory/tac_smart_ptr.h"
#include "tac-rhi/render/tac_render_handle.h"
#include "tac-rhi/render/tac_render_command_list.h"
#include "tac-rhi/render/tac_render_backend_cmd_data.h"

namespace Tac
{
  struct Errors;
  struct StackFrame;
}

namespace Tac::Render
{

  struct IBackend
  {
    using Cmds = SmartPtr< ICommandList >;

    static void       Set( IBackend* );
    static IBackend*  Get();

    virtual void Init( Errors& ) {};
    virtual void CreateDynamicBuffer2( const DynBufCreateParams&, Errors& ) {};
    virtual void UpdateDynamicBuffer2( const DynBufUpdateParams& ) {};
    virtual void SetRenderObjectName( const SetRenderObjectNameParams& ) {};

    virtual Cmds GetCommandList( ContextHandle, Errors& ) { return {}; }
  };


} // namespace Tac::Render
