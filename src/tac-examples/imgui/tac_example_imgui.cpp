#include "tac_example_imgui.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"


namespace Tac
{
  static String FormatColor( const v4& col4 )
  {
      String s;
      for( int i{}; i < 4; ++i )
      {
        int col { int( 255 * col4[ i ] ) };
        String s2 { ToString( col ) };
        while( s2.size() < 3 )
          s2 += " ";
        s += s2;
      }
      return s;
  }

  void ExampleImgui::Update(UpdateParams,  Errors& errors )
  {
    ImGuiText( "r   g   b   a" );
    for( int i{}; i < ( int )ImGuiCol::Count; ++i )
    {
      const ImGuiCol col { ( ImGuiCol )i };
      const v4& col4 { ImGuiGetColor( col ) };
      const UIStyle& style { ImGuiGetStyle() };
      const v2 size( style.fontSize, style.fontSize );
      const int iTex{ Render::TextureHandle::sNull };

      ImGuiText( FormatColor( col4 ) );
      ImGuiSameLine();
      ImGuiImage( iTex, size, col4 );
      ImGuiSameLine();
      ImGuiText( ImGuiGetColName( col ) );
    }

    if( ImGuiCollapsingHeader( "DragInts" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      static int i1    { 1 };
      static int i2[]  { 1, 2 };
      static int i3[]  { 1, 2, 3 };
      static int i4[]  { 1, 2, 3, 4 };
      ImGuiDragInt( "int1", &i1 );
      ImGuiDragInt2( "int2", i2 );
      ImGuiDragInt3( "int3", i3 );
      ImGuiDragInt4( "int4", i4 );
    }

    if( ImGuiCollapsingHeader( "DragFloats" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;

      static float f1    { 1 };
      static float f2[]  { 1, 2 };
      static float f3[]  { 1, 2, 3 };
      static float f4[]  { 1, 2, 3, 4 };
      ImGuiDragFloat( "f1", &f1 );
      ImGuiDragFloat2( "f2", f2 );
      ImGuiDragFloat3( "f3", f3 );
      ImGuiDragFloat4( "f4", f4 );
    }

  }
} // namespace Tac
