#include "tac-examples/tac_examples.h"
#include "tac-examples/tac_examples_state_machine.h"
#include "tac-examples/tac_examples_registry.h"
#include "tac-std-lib/memory/tac_memory.h"

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
      TAC_CALL( sCurrExample->Update( errors ));
    }
  }


} // namespace Tac
