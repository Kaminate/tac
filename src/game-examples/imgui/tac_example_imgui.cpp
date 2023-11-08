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
  }
} // namespace Tac
