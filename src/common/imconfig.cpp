#include "imconfig.h"
#include "imgui.h"

namespace ImGui
{
  bool InputText( const char* label, TacString& str )
  {
    const size_t bufSize = 1024;
    char buf[ bufSize ];
    strcpy_s( buf, str.c_str() );
    bool result = ImGui::InputText( label, buf, bufSize );
    if( result )
      str = buf;
    return result;
  }
  void Text( const TacString& s )
  {
    Text( s.c_str() );
  }
  bool Button( const TacString& s )
  {
    return Button( s.c_str() );
  }
  bool Checkbox( const TacString& s, bool* b )
  {
    return ImGui::Checkbox( s.c_str(), b );
  }
  bool DragDouble( const TacString& s, double* d )
  {
    float f = ( float )*d;
    bool result = ImGui::DragFloat( s.c_str(), &f );
    if( result )
      *d = ( double )f;
    return result;
  }
}

