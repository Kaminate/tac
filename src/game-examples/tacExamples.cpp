#include "src/game-examples/tacExamples.h"
#include "src/shell/tacDesktopApp.h"
#include "src/common/tacErrorHandling.h"

namespace Tac
{

  static Examples gExamples;

  static void   ExamplesInitCallback( Errors& errors )   { gExamples.Init( errors ); }
  static void   ExamplesUninitCallback( Errors& errors ) { gExamples.Uninit( errors ); }
  static void   ExamplesUpdateCallback( Errors& errors ) { gExamples.Update( errors ); }

  void                ExecutableStartupInfo::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mAppName = "Examples";
    mProjectInit = ExamplesInitCallback;
    mProjectUpdate = ExamplesUpdateCallback;
    mProjectUninit = ExamplesUninitCallback;
  }
  void Examples::Init( const Errors& ) {}
  void Examples::Update( const Errors& ) {}
  void Examples::Uninit( const Errors& ) {}
} // namespace Tac
