#include "src/common/tacTime.h"
#include "src/common/tacUtility.h"
#include "src/common/containers/tacVector.h"

namespace Tac
{


  Timepoint GetCurrentTime()
  {
    return Clock::now();
  }

  float TimepointSubtractSeconds( const Timepoint a, const Timepoint b )
  {
    return ( float )( a - b ).count() / ( float )1e9;
  }
  float TimepointSubtractMiliseconds( const Timepoint a, const Timepoint b )
  {
    return ( a - b ).count() / 1000000.0f;
  }
  float SecondsSince( const Timepoint a )
  {
    return TimepointSubtractSeconds( GetCurrentTime(), a );
  }
}
