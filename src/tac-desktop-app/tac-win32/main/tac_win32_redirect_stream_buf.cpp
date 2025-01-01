#include "tac_win32_redirect_stream_buf.h" // self-inc

#include "tac-win32/tac_win32.h" // OutputDebugStringA


#if TAC_SHOULD_IMPORT_STD
  import std;
#else
  #include <iostream>
#endif

// This struct is used to redirect calls to std::cout, std::cerr, and std::clog to the Visual
// Studio's Output window, "show output from: Debug"
//
// note:
//   1. xsputn() outputs strings of size n
//   2. overflow() outputs single characters such as '\n'
//   3. No need for LogApi::LogMessagePrint() because it is called by OSDebugPrint()
//      ( OSDebugPrint() also calls std::cout, which is intercepted by this struct )
static struct : public std::streambuf
{
  std::streamsize xsputn( const char* s, std::streamsize n ) override
  {
    OutputDebugStringA( s );
    return n;
  }

  int overflow( int c ) override
  {
    const char s[]{ ( char )c, '\0' };
    OutputDebugStringA( s );
    return c;
  }
} sStreamBuf;

// Redirect stdout to output window
void Tac::RedirectStreamBuf()
{
  std::cout.rdbuf( &sStreamBuf );
  std::cerr.rdbuf( &sStreamBuf );
  std::clog.rdbuf( &sStreamBuf );
}

