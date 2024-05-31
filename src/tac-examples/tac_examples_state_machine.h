#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct SimKeyboardApi;
  struct SimWindowApi;

  Example* GetCurrExample();
  void     SetNextExample( int );
  void     ExampleStateMachineUnint();
  int      GetCurrExampleIndex();
  void     ExampleStateMachineUpdate( Example::UpdateParams, Errors& );
} // namespace Tac
