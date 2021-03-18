#pragma once

#include "src/common/tacErrorHandling.h"
#include "src/common/string/tacString.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/containers/tacVector.h"

namespace Tac
{
  // make this allocate from frame memory?
  typedef Vector< char > TemporaryMemory;
  TemporaryMemory        TemporaryMemoryFromFile( StringView path, Errors& );
  TemporaryMemory        TemporaryMemoryFromBytes( const void* bytes, int byteCount );
  String                 FileToString( StringView path, Errors& );
  void                   WriteToFile( StringView path, void* bytes, int byteCount, Errors& );
}

