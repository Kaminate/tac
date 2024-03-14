#pragma once


namespace Tac { struct StringView; struct Example; }
namespace Tac
{

  bool ExampleIndexValid( int );
  void ExampleRegistryPopulate();
  int GetExampleIndex( const StringView& );
  int GetExampleCount();
  const char* GetExampleName(int);
  Example* CreateExample(int);
  
} // namespace Tac
