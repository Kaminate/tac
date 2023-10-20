#include "tac_example_meta.h"
#include "src/common/meta/tac_meta.h"
#include "src/common/core/tac_error_handling.h"
//#include "src/common/core/tac_preprocessor.h" // C4100
#include "src/common/graphics/imgui/tac_imgui.h"


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
  }
} // namespace Tac
