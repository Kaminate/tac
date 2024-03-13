#include "tac_string_literal.h" // self-inc

namespace Tac
{
  StringLiteral::StringLiteral( const char* str  ) : mStr( str ) {}
  const char* StringLiteral::begin() const { return mStr; }
  const char* StringLiteral::end() const   { return mStr + size(); }
  int         StringLiteral::size() const  { return StrLen( mStr ); }
  const char* StringLiteral::data() const  { return mStr; }
  const char* StringLiteral::c_str() const { return mStr; }
} // namespace Tac

