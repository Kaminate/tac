#include "tac_example_text.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"

namespace Tac
{
  static const char* quickbrownfox { "The quick brown fox jumps over the lazy dog" };

  static void Test( float fontSize, const char* str = quickbrownfox )
  {
    ImGuiPushFontSize( fontSize );
    ImGuiText( str);
    ImGuiPopFontSize();
  }


  void ExampleText::Update( Errors& )
  {
    float fontSize { 10.0f };
    for( int i{}; i < 6; ++i, fontSize *= 2 )
      Test( fontSize);
  }



} // namespace Tac
