#pragma once

#include "src/common/tac_common.h"

namespace Tac
{
  struct Example;

  bool ExampleIndexValid( int );
  void ExampleRegistryPopulate();
  int GetExampleIndex( const StringView& );
  int GetExampleCount();
  const char* GetExampleName(int);
  Example* CreateExample(int);
  
} // namespace Tac
