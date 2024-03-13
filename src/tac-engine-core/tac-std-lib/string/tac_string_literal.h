#pragma once

namespace Tac
{

  // -----------------------------------------------------------------------------------------------
  
  // String literals can be copied by value
  // mStr pointing to a c string literal using the honor system
  //
  // I like the idea of a StringLiteral class, the idea being that you KNOW that you can store
  // just a pointer and it's lifetime is forever.
  // However, the variable that we eventually store that StringLiteral into can probably also be
  // used to store strings loaded from disk. Therefore the StringLiteral turns into a String at
  // the end of the day so it might as well have been a StringView.
  struct StringLiteral
  {
    StringLiteral( const char* str = "" );
    int size() const;
    const char* begin() const;
    const char* end() const;
    const char* data() const;
    const char* c_str() const;
  private:
    const char* mStr;
  };


} // namespace Tac

