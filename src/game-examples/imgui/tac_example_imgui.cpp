#include "tac_example_imgui.h" // self-inc

#include "src/common/core/tac_error_handling.h"
#include "src/common/graphics/imgui/tac_imgui.h"

#define FMT "%3i "

namespace Tac
{
  void ExampleImgui::Update( Errors& errors )
  {
    ImGuiText( "r   g   b   a" );
    for( int i = 0; i < (int)ImGuiCol::Count; ++i )
    {
      const ImGuiCol col = ( ImGuiCol )i;
      const v4& col4 = ImGuiGetColor( col );
      const UIStyle& style = ImGuiGetStyle();
      const v2 size( style.fontSize, style.fontSize );
      const int iTex = ( int )Render::TextureHandle();

      ImGuiTextf( FMT FMT FMT FMT,
                  int( 255 * col4[ 0 ] ),
                  int( 255 * col4[ 1 ] ),
                  int( 255 * col4[ 2 ] ),
                  int( 255 * col4[ 3 ] ) );
      ImGuiSameLine();
      ImGuiImage( iTex, size, col4 );
      ImGuiSameLine();
      ImGuiText(ImGuiGetColName( col ));
    }

    if( ImGuiCollapsingHeader( "DragInts" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      static int i1 = 1;
      static int i2[] = { 1, 2 };
      static int i3[] = { 1, 2, 3 };
      static int i4[] = { 1, 2, 3, 4 };
      ImGuiDragInt( "int1", &i1 );
      ImGuiDragInt2( "int2", i2 );
      ImGuiDragInt3( "int3", i3 );
      ImGuiDragInt4( "int4", i4 );
    }

    if( ImGuiCollapsingHeader( "DragFloats" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;

      static float f1 = 1;
      static float f2[] = { 1, 2 };
      static float f3[] = { 1, 2, 3 };
      static float f4[] = { 1, 2, 3, 4 };
      ImGuiDragFloat( "f1", &f1 );
      ImGuiDragFloat2( "f2", f2 );
      ImGuiDragFloat3( "f3", f3 );
      ImGuiDragFloat4( "f4", f4 );
    }

  }
} // namespace Tac
