#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  Example* GetCurrExample();
  void     SetNextExample( int );
  void     ExampleStateMachineUnint();
  int      GetCurrExampleIndex();
  void     ExampleStateMachineUpdate( Errors& );
} // namespace Tac
