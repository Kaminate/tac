#pragma once

#include "tac-rhi/renderer/command/tac_render_command_type.h"
//#include "tac-std-lib/tac_core.h"

namespace Tac { struct Errors; }
namespace Tac::Render
{
  struct Renderer;
  struct CommandBufferIterator;

  struct CommandHandlerBase
  {
    virtual void Invoke( Renderer*, CommandBufferIterator*, Errors& ) const = 0;
    const char* mName = nullptr; // <-- mName is only used for debug log
  };

  struct CommandHandlers
  {
    CommandHandlers();

    void Add( CommandType, const CommandHandlerBase* );
    void Invoke( Renderer*,
                 CommandType,
                 CommandBufferIterator*,
                 Errors& ) const;

    const CommandHandlerBase* mCommandHandlers[ ( int )CommandType::Count ];
  };
} // namespace Tac::Render
