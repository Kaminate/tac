#pragma once

#include "tac-rhi/render/tac_render_handles.h"
#include "tac-rhi/render/tac_render_update_memory.h"

#include "tac-std-lib/error/tac_stack_frame.h"
#include "tac-std-lib/memory/tac_smart_ptr.h"
#include "tac-std-lib/string/tac_string.h"

namespace Tac::Render
{
  struct DynBufCreateParams
  {
    DynamicBufferHandle2     mHandle;
    int                      mByteCount;
    StackFrame               mStackFrame;
  };

  struct DynBufUpdateParams
  {
    DynamicBufferHandle2     mHandle;
    int                      mByteOffset;
    SmartPtr< UpdateMemory > mUpdateMemory;
  };

  struct SetRenderObjectNameParams
  {
    RenderHandle             mHandle;
    StringView               mName;
  };

} // namespace Tac::Render
