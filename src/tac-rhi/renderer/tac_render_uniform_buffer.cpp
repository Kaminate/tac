#include "tac_render_uniform_buffer.h" // self-inc

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-rhi/renderer/command/tac_render_command_data.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/error/tac_stack_frame.h"

namespace Tac::Render
{

  static Vector< String >     pushed_groups;
  static int                  pushed_group_count; // reduce dyn string allocation

  // -----------------------------------------------------------------------------------------------

  UniformBufferHeader::UniformBufferHeader( const UniformBufferEntryType type,
                                            const StackFrame& stackFrame )
  {
    mType = type;
    mStackFrame = stackFrame;
  }

  void                   UniformBuffer::PushHeader( const UniformBufferHeader& header )
  {
    PushData( &header, sizeof( header ) );
  }

  void                   UniformBuffer::PushData( const void* bytes, const int byteCount )
  {
    MemCpy( mBytes + mByteCount, bytes, byteCount );
    mByteCount += byteCount;
  }

  void                   UniformBuffer::PushString( const StringView& s )
  {
    PushNumber( s.size() );
    PushData( s.c_str(), s.size() );
    PushData( "", 1 );
  }

  void                   UniformBuffer::PushNumber( const int i )
  {
    PushData( &i, sizeof( i ) );
  }

  void                   UniformBuffer::PushPointer( const void* p )
  {
    PushData( &p, sizeof( p ) );
  }

  int                    UniformBuffer::size() const
  {
    return mByteCount;
  }

  void*                  UniformBuffer::data() const
  {
    return ( void* )mBytes;
  }

  void                   UniformBuffer::clear()
  {
    mByteCount = 0;
  }


  UniformBuffer::Iterator::Iterator( const UniformBuffer* uniformBuffer,
                                     const int iBegin,
                                     const int iEnd )
  {
    const char* uniformBufferData = ( char* )uniformBuffer->data();
    mCur = uniformBufferData + iBegin;
    mEnd = uniformBufferData + iEnd;
  }

  UniformBufferHeader    UniformBuffer::Iterator::PopHeader()
  {
    return *( UniformBufferHeader* )PopData( sizeof( UniformBufferHeader ) );
  }

  void*                  UniformBuffer::Iterator::PopData( const int byteCount )
  {
    auto result = ( void* )mCur;
    mCur += byteCount;
    return result;
  }

  int                    UniformBuffer::Iterator::PopNumber()
  {
    return *( int* )PopData( sizeof( int ) );
  }

  const void*            UniformBuffer::Iterator::PopPointer()
  {
    return *( const void** )PopData( sizeof( void* ) );
  }

  StringView             UniformBuffer::Iterator::PopString()
  {
    const int len = PopNumber();
    const char* str = ( const char* )PopData( len );
    PopData( 1 );
    return StringView( str, len );
  }

  // -----------------------------------------------------------------------------------------------

  static void ExecuteDebugGroupBegin( UniformBuffer::Iterator& iter )
  {
    const StringView desc = iter.PopString();
    pushed_groups.resize( pushed_group_count + 1 );
    pushed_groups[ pushed_group_count ] = desc;
    pushed_group_count++;
    Renderer::Instance->DebugGroupBegin( desc );
  }

  static void ExecuteDebugMarker( UniformBuffer::Iterator& iter )
  {
    const StringView desc = iter.PopString();
    Renderer::Instance->DebugMarker( desc );
  }

  static void ExecuteDebugGroupEnd()
  {
    Renderer::Instance->DebugGroupEnd();
    pushed_groups[ --pushed_group_count ].clear();
  }

  static void ExecuteUpdateConstantBuffer( const UniformBufferHeader& header,
                                           UniformBuffer::Iterator& iter,
                                           Errors& errors )
  {
    const ConstantBufferHandle constantBufferHandle = { iter.PopNumber() };
    const int byteCount = iter.PopNumber();
    const void* bytes = iter.PopPointer();
    const CommandDataUpdateConstantBuffer commandData
    {
      .mStackFrame = header.mStackFrame,
      .mConstantBufferHandle = constantBufferHandle,
      .mBytes = bytes,
      .mByteCount = byteCount,
    };
    Renderer::Instance->UpdateConstantBuffer( &commandData, errors );
  }

  // -----------------------------------------------------------------------------------------------

  void ExecuteUniformCommands( const UniformBuffer* uniformBuffer,
                                      const int iUniformBegin,
                                      const int iUniformEnd,
                                      Errors& errors )
  {
    UniformBuffer::Iterator iter( uniformBuffer, iUniformBegin, iUniformEnd );
    while( iter.mCur < iter.mEnd )
    {
      const UniformBufferHeader header = iter.PopHeader();
      TAC_ASSERT( header.mCorruption == UniformBufferHeader().mCorruption );
      switch( header.mType )
      {
        case UniformBufferEntryType::DebugGroupBegin:
        {
          ExecuteDebugGroupBegin( iter );
        } break;
        case UniformBufferEntryType::DebugMarker:
        {
          ExecuteDebugMarker( iter );
        } break;
        case UniformBufferEntryType::DebugGroupEnd:
        {
          ExecuteDebugGroupEnd();
        } break;
        case UniformBufferEntryType::UpdateConstantBuffer:
        {
          TAC_CALL( ExecuteUpdateConstantBuffer( header, iter, errors ));
        } break;
        default: TAC_ASSERT_INVALID_CASE( header.mType ); return;
      }
    }
  }

} // namespace Tac::Render

