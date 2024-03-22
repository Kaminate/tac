#pragma once

#include "tac-std-lib/memory/tac_smart_ptr.h"

#include "tac-rhi/render/tac_render_update_memory.h"
#include "tac-rhi/render/tac_render_command_list.h"
#include "tac-rhi/renderer/tac_renderer.h" // ScissorRect

namespace Tac::Render
{
  enum class HandleType
  {
    kView,
    kDynamicBuffer,
    kContext,
    kCount,
  };

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

  struct RenderHandle
  {
    int mIndex = -1;
  };

  struct FrameBufHandle2 : public RenderHandle
  {
  };

  struct ViewHandle2 : public RenderHandle
  {
    void SetViewport( Viewport );
    void SetFramebuffer();
    void SetScissorRect( ScissorRect );
    ViewData* GetViewData();
  };

  struct DynamicBufferHandle2 : public RenderHandle
  {
  };

  struct ContextHandle : public RenderHandle
  {
    DynamicBufferHandle2 CreateDynamicBuffer( int byteCount, const StackFrame& );

    // this is just a MemCpy, no cmdlist needed, no SmartPtr<UpdateMemory> needed
    void UpdateDynamicBuffer( DynamicBufferHandle2, int byteOffset, SmartPtr< UpdateMemory > );


    // but.. updating a static buffer needs
    //
    // m_commandList->CopyBufferRegion and
    // m_commandList->ResourceBarrier
    //
    // so SmartPtr<UpdateMemory> is needed here
#if 0
    void UpdateStaticBuffer( SmartPtr<UpdateMemory> ) {};
#endif

    void Draw();
    void SetRenderObjectName( RenderHandle, StringView );

    BackendCmdData* mData = nullptr;

    // RSSetViewport
    // IASetVertexBuffers
    // CopyTextureRegion
    ICommandList* mCmdList = nullptr;
  };

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render

