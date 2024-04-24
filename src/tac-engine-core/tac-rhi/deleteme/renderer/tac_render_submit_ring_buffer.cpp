#include "tac_render_submit_ring_buffer.h" // self-inc

#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac::Render
{

  void*                 SubmitRingBuffer::SubmitAlloc( const int byteCount )
  {
    if( !byteCount )
      return nullptr;

    const int beginPos { mPos + byteCount > sCapacity ? 0 : mPos };
    mPos = beginPos + byteCount;
    return mBytes + beginPos;
  }

  const void*           SubmitRingBuffer::SubmitAlloc( const void* bytes, const int byteCount )
  {
    if( !bytes )
      return nullptr;

    if( IsSubmitAllocated( bytes ) )
      return bytes;

    void* dst { SubmitAlloc( byteCount ) };
    MemCpy( dst, bytes, byteCount );
    return dst;
  }

  StringView            SubmitRingBuffer::SubmitAlloc( const StringView& stringView )
  {
    const int n { stringView.size() };
    if( !n )
      return {};

    if( IsSubmitAllocated( stringView.data() ) )
      return stringView;

    auto resultData { ( char* )SubmitAlloc( n + 1 ) };
    MemCpy( resultData, stringView.c_str(), n );
    resultData[ n ] = '\0';
    return StringView( resultData, n );
  }

  bool                  SubmitRingBuffer::IsSubmitAllocated( const void* data ) const
  {
    const bool result { data >= mBytes && data < mBytes + sCapacity };
    return result;
  }

  void                  SubmitRingBuffer::DebugPrintSubmitAllocInfo() const
  {

    OS::OSDebugPrintLine( String()
                          + "gSubmitRingBuffer::sCapacity: " + Tac::ToString( sCapacity ) );

    OS::OSDebugPrintLine( String()
                          + "gSubmitRingBuffer.mBytes: " + Tac::ToString( ( void* )mBytes ) );

    OS::OSDebugPrintLine( String()
                          + "gSubmitRingBuffer.mPos: " + Tac::ToString( mPos ) );
  }

} // namespace Tac::Render

