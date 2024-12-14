#include "tac_example_meta.h" // self-inc

#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/meta/tac_meta_fn.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <sstream>
#endif

namespace Tac
{
  void ExampleMeta::Update(  Errors& errors )
  {
    mShouldRunTests |=  ImGuiButton( "Run Unit Tests" );
    if( mShouldRunTests )
    {
      RunMetaUnitTestSuite();
      mShouldRunTests = false;
    }

    for( MetaFn* fn : MetaFn::Range() )
    {
      std::stringstream ss;
      ss
        << fn->RetType()->GetName()
        << " "
        << fn->Name()
        << "( ";

      const char* sep { "" };
      for( int i{}; i < fn->ArgCount(); ++i )
      {
        ss
          << sep
          << fn->ArgType( i )->GetName();
        sep = ", ";
      }

      ss
        << " )";

      std::string s { ss.str() };
      const char* cstr { s.c_str() };
      ImGuiText( cstr );
    }
  }
} // namespace Tac
