#pragma once

#include "tac-rhi/identifier/tac_handle.h"

#include "tac_render_handle_type.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  struct RenderHandle
  {
    RenderHandle() = default;
    RenderHandle( HandleType, Handle );
    RenderHandle( RenderHandle&& ) noexcept;
    RenderHandle( const RenderHandle& );
    ~RenderHandle();
    void Clear();
    int  GetHandleIndex() const;
    HandleType GetHandleType() const;
    void Assign( const RenderHandle& );
    bool IsValid() const;

    void operator = ( RenderHandle&& ) noexcept;
    void operator = ( const RenderHandle& );

  private:
    void Inc() const;
    void Dec() const;

    HandleType     mType = HandleType::kCount;
    mutable int*   mReferenceCounter = nullptr;
    mutable Handle mHandle;
  };

} // namespace
