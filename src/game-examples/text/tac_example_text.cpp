#include "tac_example_text.h" // self-inc

#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/shell/tac_shell_timestep.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"
#include "src/common/graphics/tac_font.h"

namespace Tac
{
  static const char* quickbrownfox = "The quick brown fox jumps over the lazy dog";
  static const bool sShowRedrawDialog;

  static void Test( float fontSize, const char* str = quickbrownfox )
  {
    ImGuiPushFontSize( fontSize );
    ImGuiText( str);
    ImGuiPopFontSize();
  }


  void ExampleText::Update( Errors& errors )
  {
    if( sShowRedrawDialog )
    {
      static bool setForceRedraw;
      if( ImGuiCheckbox( "Force redraw", &setForceRedraw ) )
        FontApi::SetForceRedraw( setForceRedraw );
    }


    float fontSize = 10.0f;
    for( int i = 0; i < 6; ++i, fontSize *=  2 )
      Test( fontSize);
  }



} // namespace Tac
