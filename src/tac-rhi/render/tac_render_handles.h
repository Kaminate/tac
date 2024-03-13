#pragma once

#include "tac-std-lib/memory/tac_smart_ptr.h"

#include "tac-rhi/identifier/tac_handle.h"
#include "tac-rhi/render/tac_render_update_memory.h"
#include "tac-rhi/render/tac_render_handle.h"
#include "tac-rhi/render/tac_render_handle_mgr.h"
#include "tac-rhi/render/tac_render_command_list.h"
#include "tac-rhi/renderer/tac_renderer.h" // ScissorRect

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------
  struct BackendCmdData;

  // -----------------------------------------------------------------------------------------------

  struct ViewData
  {
    // use Optional<>? instead?

    ScissorRect mScissorRect;
    bool        mIsScissorSet = false;

    Viewport mViewport;
    bool     mIsViewportSet = false;

    // ... framebuffer
  };

  struct FrameBufHandle2 : public RenderHandle
  {
  };

  struct ViewHandle2 : public RenderHandle
  {
    void SetViewport( Viewport );
    void SetFramebuffer();
    void SetScissorRect( ScissorRect );
    void SetViewData( ViewData* );
    const ViewData* GetViewData() const;
  private:
    ViewData* mViewData = nullptr;
  };

  struct DynamicBufferHandle2 : public RenderHandle
  {
  };

  struct ContextHandle : public RenderHandle
  {
    DynamicBufferHandle2 CreateDynamicBuffer( int byteCount, const StackFrame& );
    void UpdateDynamicBuffer( DynamicBufferHandle2, int byteOffset, SmartPtr< UpdateMemory > );
    void Draw();
    void SetRenderObjectName( RenderHandle, StringView );

    BackendCmdData* mData;
    ICommandList* mCmdList;
  };

  // -----------------------------------------------------------------------------------------------

  template< typename T > HandleType GetHandleType() { TAC_ASSERT_INVALID_CODE_PATH; return HandleType::kCount; }
  template<> inline HandleType GetHandleType< ViewHandle2 >()          { return HandleType::kView; }
  template<> inline HandleType GetHandleType< DynamicBufferHandle2 >() { return HandleType::kDynamicBuffer; }
  template<> inline HandleType GetHandleType< ContextHandle >()        { return HandleType::kContext; }

  template< typename T > T AllocRenderHandleT()
  {
    const HandleType type = GetHandleType< T >();
    const Handle h = RenderHandleAlloc( type );
    T t;
    auto renderHandle = static_cast< RenderHandle* >( &t );
    //renderHandle->SetType( type );
    *renderHandle = RenderHandle( type, h );
    return t;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render

