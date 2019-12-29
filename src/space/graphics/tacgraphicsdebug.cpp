#include "common/graphics/tacImGui.h"
#include "space/graphics/tacgraphics.h"
void TacGraphicsDebugImgui( TacSystem* system )
{
  auto graphics = ( TacGraphics* )system;
  TacImGuiText( "graphics stuff" );


  static TacString shaderReloadStatus;

  if( TacImGuiCollapsingHeader( "reload shaders" ) )
  {
    TacImGuiIndent();
    OnDestruct(TacImGuiUnindent());
    TacRenderer* renderer = TacRenderer::Instance;
    TacVector<TacShader*> shaders;
    renderer->GetShaders( shaders );

    bool reloadAllShaders = TacImGuiButton("reload all shaders");
    for( TacShader* shader : shaders )
    {
      if( TacImGuiButton( "reload shader " + shader->mName ) || reloadAllShaders )
      {
        TacErrors shaderReloadErrors;
        renderer->ReloadShader( shader, shaderReloadErrors );
        if(shaderReloadErrors.size())
          shaderReloadStatus = shaderReloadErrors.ToString();
        else
          shaderReloadStatus = "reloaded shader " + shader->mName;
      }
    }

    if(TacImGuiButton("clear status"))
      shaderReloadStatus.clear();
    TacImGuiText( shaderReloadStatus );
  }
}
