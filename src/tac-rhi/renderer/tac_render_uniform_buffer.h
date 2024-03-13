#pragma once

//#include "tac-rhi/renderer/tac_renderer.h"
//#include "tac-std-lib/containers/tac_fixed_vector.h"
//#include "tac-std-lib/containers/tac_vector.h"
//#include "tac-std-lib/string/tac_string.h"

#include "tac-std-lib/error/tac_stack_frame.h"

namespace Tac {  struct StringView; struct Errors; }

namespace Tac::Render
{

  enum class UniformBufferEntryType
  {
    Unknown = 0,
    DebugGroupBegin,
    DebugMarker,
    DebugGroupEnd,
    UpdateConstantBuffer,
  };

  struct UniformBufferHeader
  {
    UniformBufferHeader() = default;
    UniformBufferHeader( UniformBufferEntryType, const StackFrame& );
    UniformBufferEntryType mType = UniformBufferEntryType::Unknown;
    int                    mCorruption = 0xd34db33f;
    StackFrame             mStackFrame;
  };

  struct UniformBuffer
  {
    void                     PushHeader( const UniformBufferHeader& );
    void                     PushData( const void*, int );
    void                     PushString( const StringView& );
    void                     PushNumber( int );
    void                     PushPointer( const void* );

    int                      size() const;
    void*                    data() const;
    void                     clear();


    struct Iterator
    {
      Iterator( const UniformBuffer*, int iBegin, int iEnd );
      UniformBufferHeader    PopHeader();
      void*                  PopData( int );
      int                    PopNumber();
      const void*            PopPointer();
      StringView             PopString();
      const char*            mCur;
      const char*            mEnd;
    };

  private:
    static const int         kByteCapacity = 256 * 1024;
    char                     mBytes[ kByteCapacity ] = {};
    int                      mByteCount = 0;
  };

  void ExecuteUniformCommands( const UniformBuffer*,
                               const int iUniformBegin,
                               const int iUniformEnd,
                               Errors& );
} // namespace Tac::Render
