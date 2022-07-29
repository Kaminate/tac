#include "tac_example_meta.h"
#include "src/common/meta/tac_meta.h"
#include "src/common/tac_error_handling.h"
//#include "src/common/tac_preprocessor.h" // C4100
#include "src/common/graphics/imgui/tac_imgui.h"


namespace Tac
{
  void ExampleMeta::Init( Errors& errors )
  {
      RunMetaUnitTestSuite();
  }

  void ExampleMeta::Update( Errors& errors )
  {
    if( ImGuiButton( "Run Unit Tests" ) )
    {
      RunMetaUnitTestSuite();
    }
  }

  void ExampleMeta::Uninit( Errors& errors )
  {

  }

  const char* ExampleMeta::GetName() const
  {
    return "Meta";
  }

} // namespace Tac
