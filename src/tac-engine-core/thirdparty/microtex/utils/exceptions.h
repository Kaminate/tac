
#pragma once

#include <cassert>
#include <string>
#include <iostream>

inline void MicroTexError( const std::string& s )
{
  std::cout << s << std::endl;
  assert(false);
}

#define MICROTEX_ERROR(x)     do{ MicroTexError(x); return; } while( false )
#define MICROTEX_ERROR_RET(x) do{ MicroTexError(x); return {}; } while( false )
