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
  SplitFilepath( const String& );
  String mFilename;
  String mDirectory;
};
void SaveToFile( const String& path, void* bytes, int byteCount, Errors& errors );
bool IsOfExt( const String& str, const String& ext );
bool FileExist( const String& str );
String StripExt( const String& path );
String StripLeadingSlashes( const String& path );


//
// String-manipulation
//
String SeparateStrings( const Vector< String>& lines, const String& separator );
String SeparateNewline( const Vector< String>& lines );
String SeparateSpace( const Vector< String>& lines );
bool StartsWith( const String& str, const String& prefix );
bool EndsWith( const String& str, const String& suffix );
String ToLower( const String& str );
String FormatPercentage( float number_between_0_and_1 );
String FormatPercentage( float curr, float maxi );

}
