#include "tac_render_command_buffer.h" // self-inc

#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/string/tac_string.h"

namespace Tac::Render
{
  const StringView cheep( "end" ); // canary sound

  // -----------------------------------------------------------------------------------------------

  bool CommandBufferIterator::IsValid()
  {
    return bufferPos < bufferEnd;
  }

  void* CommandBufferIterator::PopBytes( int n )
  {
    if( bufferPos + n > bufferEnd )
      return nullptr;
    auto result = ( void* )bufferPos;
    bufferPos += n;
    return result;
  }

  void CommandBufferIterator::PopCanary()
  {
    const StringView canary( ( const char* )PopBytes( cheep.size() ), cheep.size() );
    TAC_ASSERT( canary == cheep );
  }

  // -----------------------------------------------------------------------------------------------

  void        CommandBuffer::Push( const void* bytes,
                                   const int byteCount )
  {
    const int bufferSize = mBuffer.size();
    mBuffer.resize( mBuffer.size() + byteCount );
    MemCpy( mBuffer.data() + bufferSize, bytes, byteCount );
  }

  void        CommandBuffer::PushCommand( const CommandType type,
                                          const void* bytes,
                                          const int byteCount )
  {
    Push( &type, sizeof( CommandType ) );
    Push( bytes, byteCount );
    Push( cheep.data(), cheep.size() );
  }

  void        CommandBuffer::Resize( int newSize ) { mBuffer.resize( newSize ); }

  void        CommandBuffer::Clear() { mBuffer.clear(); }

  const char* CommandBuffer::Data() const { return mBuffer.data(); }

  int         CommandBuffer::Size() const { return mBuffer.size(); }

  CommandBufferIterator CommandBuffer::GetIterator()
  {
    return CommandBufferIterator
    {
      .bufferBegin = Data(),
      .bufferEnd = Data() + Size(),
      .bufferPos = Data()
    };
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render
