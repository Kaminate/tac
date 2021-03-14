// Some helper functions
// ( non-templated, those would go in preprocessor )

#pragma once

#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"

namespace Tac
{
  //
  // Filesystem utils
  //
  struct SplitFilepath
  {
    // "/usr/bin/man" --> { "/user/bin", "man" }
    // "c:\windows\winhelp.exe" --> { "c:\windows", "winhelp.exe" }
    SplitFilepath( StringView );
    String mFilename;
    String mDirectory;
  };

  void   SaveToFile( StringView path, void* bytes, int byteCount, Errors& );
  bool   IsOfExt( StringView str, StringView ext );
  bool   FileExist( StringView str );

  // "foo.txt" --> "foo"
  String StripExt( StringView );
  String StripLeadingSlashes( StringView path );

  //
  // String-manipulation
  //
  String Join( const Vector< String >& lines, StringView separator );
  String Join( const String* lines, int lineCount, StringView separator );
  //String SeparateNewline( const Vector< String >& lines );
  //String SeparateSpace( const Vector< String >& lines );
  bool   StartsWith( StringView str, StringView prefix );
  bool   EndsWith( StringView str, StringView suffix );
  String ToLower( StringView str );
  String FormatPercentage( float number_between_0_and_1 );
  String FormatPercentage( float curr, float maxi );

}
