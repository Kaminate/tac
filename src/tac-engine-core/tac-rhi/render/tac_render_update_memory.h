#pragma once


namespace Tac::Render
{
  struct UpdateMemory
  {
    virtual ~UpdateMemory() = default;
    virtual const void* GetBytes() const = 0;
    virtual int         GetByteCount() const = 0;
  };

} // namespace Tac::Render

