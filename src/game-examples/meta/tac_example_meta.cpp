#include "tac_example_meta.h"
#include "src/common/meta/tac_meta.h"
#include "src/common/meta/tac_meta_fn.h"
#include "src/common/core/tac_error_handling.h"
//#include "src/common/core/tac_preprocessor.h" // C4100
#include "src/common/graphics/imgui/tac_imgui.h"

#include <sstream>

namespace Tac
{
  void ExampleMeta::Update( Errors& errors )
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

      const char* sep = "";
      for( int i = 0; i < fn->ArgCount(); ++i )
      {
        ss
          << sep
          << fn->ArgType( i )->GetName();
        sep = ", ";
      }

      ss
        << " )";

      std::string s = ss.str();
      const char* cstr = s.c_str();
      ImGuiText( cstr );
    }
  }
} // namespace Tac
