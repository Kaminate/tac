#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/string/tacString.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/common/tacPreprocessor.h"

namespace Tac
{
  void GraphicsDebugImgui( System* system )
  {
    TAC_UNUSED_PARAMETER( system );

    //auto graphics = ( Graphics* )system;
    ImGuiText( "graphics stuff" );
    //static String shaderReloadStatus;

    //if( ImGuiCollapsingHeader( "reload shaders" ) )
    //{
    //  ImGuiIndent();
    //  TAC_ON_DESTRUCT(ImGuiUnindent());
    //  Vector<Shader*> shaders;
    //  Renderer::Instance->GetShaders( shaders );

    //  bool reloadAllShaders = ImGuiButton("reload all shaders");
    //  for( Shader* shader : shaders )
    //  {
    //    if( ImGuiButton( "reload shader " + shader->mName ) || reloadAllShaders )
    //    {
    //      Errors shaderReloadErrors;
    //      Renderer::Instance->ReloadShader( shader, shaderReloadErrors );
    //      if(shaderReloadErrors)
    //        shaderReloadStatus = shaderReloadErrors.ToString();
    //      else
    //        shaderReloadStatus = "reloaded shader " + shader->mName;
    //    }
    //  }

    //  if(ImGuiButton("clear status"))
    //    shaderReloadStatus.clear();
    //  ImGuiText( shaderReloadStatus );
    //}
  }

}

