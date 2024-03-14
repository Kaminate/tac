#pragma once

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/memory/tac_smart_ptr.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-rhi/render/tac_render_update_memory.h"
#include "tac-rhi/render/tac_render_command_type.h"
#include "tac-rhi/render/tac_render_handle.h"
#include "tac-rhi/render/tac_render_backend_cmd_data.h"

namespace Tac::Render
{
  struct BackendCmdData
  {
    Vector< char >                     mCommandByteData;
    Vector< CommandType2 >             mCommands;
    Vector< SmartPtr< UpdateMemory > > mUpdateMemories;
    Vector< RenderHandle >             mRenderHandles;
  };

  struct BackendCmdSerializer
  {
    BackendCmdSerializer( BackendCmdData* );
    void     PushDynBufCreateParams( const DynBufCreateParams& );
    void     PushDynBufUpdateParams( const DynBufUpdateParams& );
    void     PushSetRenderObjectNameParams( const SetRenderObjectNameParams& );

  private:
    void     PushCommandType( CommandType2 );
    void     PushPointer( const void* );
    void     PushInt( int );
    void     PushBytes( const void*, int );
    void     PushString( StringView );
    void     PushStackFrame( const StackFrame& );
    void     PushUpdateMemory( SmartPtr< UpdateMemory > );
    void     PushRenderHandle( const RenderHandle& );

    BackendCmdData* mData;
  };

  struct BackendCmdDeserializer
  {
    BackendCmdDeserializer( BackendCmdData* );
    bool                      IsValid() const;
    CommandType2              PopCommandType();
    DynBufCreateParams        PopDynBufCreateParams();
    DynBufUpdateParams        PopDynBufUpdateParams();
    SetRenderObjectNameParams PopSetRenderObjectNameParams();

  private:

    int                       PopInt();
    RenderHandle              PopRenderHandle();
    SmartPtr< UpdateMemory >  PopUpdateMemory();
    StackFrame                PopStackFrame();
    StringView                PopString();
    const void*               PopPointer();
    const void*               PopBytes( int );

    int                       mHandleIdx = 0;
    int                       mCmdTypeIdx = 0;
    int                       mCmdByteIdx = 0;
    int                       mUpdateMemIdx = 0;
    BackendCmdData*           mData;
  };



} // namespace Tac::Render

