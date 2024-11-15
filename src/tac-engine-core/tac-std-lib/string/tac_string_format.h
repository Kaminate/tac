// I think this file should only be included by tac_string.cpp
// because its slow
#pragma once

#if 0



#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

#if TAC_SHOULD_IMPORT_STD()
  import std; // size_t, formatted_size, forward, format_to_n, format_string, formatter, ...
#else
  #include <cstdint>
#endif

namespace Tac
{

  template < typename ... Args >
  StringView FrameMemoryFormat( std::format_string< Args ... > fmt_str, Args&& ... args )
  {
    const std::size_t n = std::formatted_size( fmt_str, std::forward<Args>( args )... );
    char* bytes{ ( char* )FrameMemoryAllocate( ( int )n + 1 ) };
    std::format_to_n( bytes, n, fmt_str, std::forward<Args>( args )... );
    bytes[ n ] = '\0';
    return StringView( bytes, (int)n );
  }

  template < typename ... Args >
  String FormatString( std::format_string< Args ... > fmt_str, Args&& ... args )
  {
    const std::size_t n = std::formatted_size( fmt_str, std::forward<Args>( args )... );
    String s;
    s.resize( ( int )n );
    std::format_to_n( &s.front(), n, fmt_str, std::forward<Args>( args )... );
    return s;
  }

  template< typename ... Args >
  ShortFixedString va( std::format_string< Args ... > fmt_str, Args&& ... args )
  {
    ShortFixedString fs;
    const FixedStringData fsd = fs.GetFSD();

    const std::size_t n = std::formatted_size( fmt_str, std::forward<Args>( args )... );
    TAC_ASSERT( n < fsd.mCapacity );
    std::format_to_n( fsd.mBuf, n, fmt_str, std::forward<Args>( args )... );
    *fsd.mSize = ( int )n;
    fsd.mBuf[ n ] = '\0';
    return fs;
  }
} // namespace Tac

// Used to pass a StringView into va() or FrameMemoryFormat
template <>
struct std::formatter<Tac::StringView> : public std::formatter<std::string_view>
{
  auto format( const Tac::StringView& obj, std::format_context& ctx ) const
  {
    const std::string_view sv( obj.data(), obj.size() );
    return std::formatter<std::string_view>::format( sv, ctx );
  }
};

#endif
