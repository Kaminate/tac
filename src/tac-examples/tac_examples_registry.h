#pragma once

namespace Tac { struct StringView; struct Example; }
namespace Tac
{
  bool ExampleIndexValid( int );
  void ExampleRegistryPopulate();
  auto GetExampleIndex( const StringView ) -> int;
  auto GetExampleCount() -> int;
  auto GetExampleName(int) -> const char*;
  auto CreateExample(int) -> Example*;
  
} // namespace Tac
