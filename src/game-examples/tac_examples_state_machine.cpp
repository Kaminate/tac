#if 1
#include "src/game-examples/tac_examples.h"
#include "src/game-examples/tac_examples_state_machine.h"
#include "src/game-examples/tac_examples_registry.h"
#include "src/common/memory/tac_memory.h"
#include "src/common/core/tac_error_handling.h"

namespace Tac
{
  static int                       sExampleIndexCurr = -1;
  static int                       sExampleIndexNext = -1;
  static Example* sCurrExample;

  Example* GetCurrExample()
  {
    return sCurrExample;
  }


  void SetNextExample( int i )
  {
    i += GetExampleCount();
    i %= GetExampleCount();
    sExampleIndexNext = i;
  }

  void ExampleStateMachineUnint()
  {
    TAC_DELETE sCurrExample;
  }

  int GetCurrExampleIndex()
  {
    return sExampleIndexCurr;
  }

  void ExampleStateMachineUpdate(Errors& errors)
  {
    if( ExampleIndexValid( sExampleIndexNext ) && sExampleIndexNext != sExampleIndexCurr )
    {
      if( sCurrExample )
        TAC_DELETE sCurrExample;

      sExampleIndexCurr = sExampleIndexNext;

      sCurrExample = CreateExample(sExampleIndexCurr);
      sCurrExample->mName = GetExampleName(sExampleIndexCurr);
    }

    if( sCurrExample )
    {
      sCurrExample->Update( errors );
      TAC_HANDLE_ERROR( errors);
    }
  }


} // namespace Tac
#endif
