#include "tac_win32_redirect_stream_buf.h" // self-inc

#include "tac-win32/tac_win32.h" // OutputDebugStringA

#if TAC_SHOULD_IMPORT_STD
  import std;
#else
  #include <iostream>
#endif

//import std; // #include <iostream>

static struct : public std::streambuf
{
  // xsputn() outputs strings of size n
  std::streamsize xsputn( const char* s, std::streamsize n ) override
  {
    OutputDebugStringA( s );
    return n;
  }

  // overflow() outputs characters such as '\n'
  int overflow( int c ) override
  {
    const char s[] = { ( char )c, '\0' };
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

