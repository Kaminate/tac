// Some helper functions
// ( non-templated, those would go in preprocessor )

#pragma once

#include "tacString.h"
#include "tacErrorHandling.h"
#include "containers/tacVector.h"
//#include "tacPreprocessor.h"

//#include <set>


//
// Filesystem utils
//
struct TacSplitFilepath
{
  // "/usr/bin/man" --> { "/user/bin", "man" }
  // "c:\windows\winhelp.exe" --> { "c:\windows", "winhelp.exe" }
  TacSplitFilepath( const TacString& );
  TacString mFilename;
  TacString mDirectory;
};
void TacSaveToFile( const TacString& path, void* bytes, int byteCount, TacErrors& errors );
bool TacIsOfExt( const TacString& str, const TacString& ext );
bool TacFileExist( const TacString& str );


//
// String-manipulation
//
TacString TacSeparateStrings( const TacVector< TacString>& lines, const TacString& separator );
TacString TacSeparateNewline( const TacVector< TacString>& lines );
TacString TacSeparateSpace( const TacVector< TacString>& lines );
bool TacEndsWith( const TacString& str, const TacString& suffix );
TacString TacToLower( const TacString& str );
TacString TacFormatPercentage( float number_between_0_and_1 );
TacString TacFormatPercentage( float curr, float maxi );

