#pragma once

namespace Tac
{
  struct Example;


  bool ExampleIndexValid( int );
  void ExampleRegistryPopulate();
  int GetExampleIndex( StringView );
  int GetExampleCount();
  const char* GetExampleName(int);
  Example* CreateExample(int);
  
  
} // namespace Tac
