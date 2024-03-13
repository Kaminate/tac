#include "tac_render_handles.h" // self-inc

#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-rhi/render/tac_render_context_data.h"
#include "tac-rhi/render/tac_render_backend_cmd_data.h"

#include "tac-std-lib/string/tac_string_view.h"

namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------

  // ViewHandle2 
  void ViewHandle2::SetViewport( Viewport vp )
  {
    mViewData->mViewport = vp;
    mViewData->mIsViewportSet = true;
  }
  void ViewHandle2::SetFramebuffer() {}
  void ViewHandle2::SetScissorRect( ScissorRect sr )
  {
    mViewData->mScissorRect = sr;
    mViewData->mIsScissorSet = true;
  }
  void ViewHandle2::SetViewData( ViewData* viewData ) { mViewData = viewData; }
  const ViewData* ViewHandle2::GetViewData() const { return mViewData; }
  // -----------------------------------------------------------------------------------------------

  // ContextHandle 

  void ContextHandle::Draw()
  {
    mCmdList->Draw();
  }

  void ContextHandle::SetRenderObjectName( RenderHandle h, StringView s )
  {
    const SetRenderObjectNameParams params
    {
      .mHandle = h,
      .mName = s,
    };

    BackendCmdSerializer( mData ).PushSetRenderObjectNameParams( params );
  }

  void ContextHandle::UpdateDynamicBuffer( DynamicBufferHandle2 handle,
                                           int byteOffset,
                                           SmartPtr< UpdateMemory > mem )
  {
    const DynBufUpdateParams params
    {
      .mHandle = handle,
      .mByteOffset = byteOffset,
      .mUpdateMemory = mem,
    };

    BackendCmdSerializer( mData ).PushDynBufUpdateParams( params );
  }

  DynamicBufferHandle2 ContextHandle::CreateDynamicBuffer( int byteCount,
                                                           const StackFrame& stackFrame )
  {
    const DynBufCreateParams params
    {
      .mHandle = AllocRenderHandleT< DynamicBufferHandle2 >(),
      .mByteCount = byteCount,
      .mStackFrame = stackFrame,
    };

    BackendCmdSerializer( mData ).PushDynBufCreateParams( params );
    return params.mHandle;
  }

} // namespace Tac::Render

// -------------------------------------------------------------------------------------------------

namespace Tac
{
}


