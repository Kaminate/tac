
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacShell.h"
#include "src/common/tacOS.h"
namespace Tac
{

  const char* GetSemanticName( Attribute attribType )
  {
    switch( attribType )
    {
      case Attribute::Position: return "POSITION";
      case Attribute::Normal: return "NORMAL";
      case Attribute::Texcoord: return "TEXCOORD";
      case Attribute::Color: return "COLOR";
      case Attribute::BoneIndex: return "BONEINDEX";
      case Attribute::BoneWeight: return "BONEWEIGHT";
      case Attribute::Coeffs: return "COEFFS";
        TAC_INVALID_DEFAULT_CASE( attribType );
    }
    return nullptr;
  }

  String ToString( Renderer::Type rendererType )
  {
    switch( rendererType )
    {
      case Renderer::Type::Vulkan: return "Vulkan";
      case Renderer::Type::OpenGL4: return "OpenGL4";
      case Renderer::Type::DirectX12: return "DirectX12";
        TAC_INVALID_DEFAULT_CASE( rendererType );
    }
    TAC_INVALID_CODE_PATH;
    return "";
  }
  v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated )
  {
    return {
      colorAlphaUnassociated.x * colorAlphaUnassociated.w,
      colorAlphaUnassociated.y * colorAlphaUnassociated.w,
      colorAlphaUnassociated.z * colorAlphaUnassociated.w,
      colorAlphaUnassociated.w };
  }


  int Format::CalculateTotalByteCount() const
  {
    return mElementCount * mPerElementByteCount;
  }


  Renderer* Renderer::Instance = nullptr;

  Renderer::Renderer()
  {
    Instance = this;
  }

  Renderer::~Renderer()
  {
    for( RendererResource* rendererResource : mRendererResources )
    {
      String errorMessage =
        "Resource leaked: " +
        rendererResource->mName +
        " created at " +
        rendererResource->mFrame.ToString();
      OS::Instance->DebugAssert( errorMessage, TAC_STACK_FRAME );
    }
  }

  void Renderer::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( "Renderer" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //if( ImGui::CollapsingHeader( "Shaders" ) )
    //{
    //  ImGui::Indent();
    //  OnDestruct( ImGui::Unindent() );
    //  auto reloadAll = ImGui::Button( "Reload all shaders" );
    //  Vector< Shader*>shaders;
    //  GetShaders( shaders );
    //  for( auto shader : shaders )
    //  {
    //    auto reloadShader = reloadAll || ImGui::Button( shader->mName.c_str() );
    //    if( !reloadShader )
    //      continue;
    //    ReloadShader( shader, mShaderReloadErrors );
    //  }
    //  ImGui::Text( mShaderReloadErrors.mMessage );
    //}

    //if( ImGui::CollapsingHeader( "Textures" ) )
    //{
    //  ImGui::Indent();
    //  OnDestruct( ImGui::Unindent() );
    //  Vector< Texture* > textures;
    //  GetTextures( textures );
    //  for( auto texture : textures )
    //  {
    //    ImGui::PushID( texture );
    //    OnDestruct( ImGui::PopID() );
    //    if( ImGui::CollapsingHeader( texture->mName.c_str() ) )
    //    {
    //      ImGui::Indent();
    //      OnDestruct( ImGui::Unindent() );
    //      ImGui::Text( "Width: %ipx", texture->myImage.mWidth );
    //      ImGui::Text( "Height: %ipx", texture->myImage.mHeight );
    //      float w = 0.9f * ImGui::GetContentRegionAvailWidth();
    //      float h = w / texture->GetAspect();
    //      auto id = texture->GetImguiTextureID();
    //      auto size = ImVec2( w, h );
    //      ImGui::Image( id, size );
    //    }
    //  }
    //}
  }

  const float sizeInMagicUISpaceUnits = 1024.0f;

  // Make this static?
  void Renderer::GetUIDimensions(
    Texture* texture,
    float* width,
    float* height )
  {
    float aspect = texture->GetAspect();
    float scaleWidth = 1;
    float scaleHeight = 1;
    if( aspect > 1 )
      scaleWidth = aspect;
    else
      scaleHeight = 1.0f / aspect;
    *width = scaleWidth * sizeInMagicUISpaceUnits;
    *height = scaleHeight * sizeInMagicUISpaceUnits;
  }


  void RendererFactory::CreateRendererOuter()
  {
    CreateRenderer();
    Renderer::Instance->mName = mRendererName;
  }

  RendererRegistry& RendererRegistry::Instance()
  {
    // This variable must be inside this function or else the
    // renderers will add themselves too early or something
    // and then be stomped with an empty registry
    static RendererRegistry RendererRegistryInstance;
    return RendererRegistryInstance;
  }

  RendererFactory* RendererRegistry::FindFactory( StringView name )
  {
    for( RendererFactory* factory : mFactories )
      if( factory->mRendererName == name )
        return factory;
    return nullptr;
  }

  void Renderer::RemoveRendererResource( RendererResource* rendererResource )
  {
    if( !rendererResource )
      return;
    bool found = Contains( mRendererResources, rendererResource );
    TAC_ASSERT( found );
    mRendererResources.erase( rendererResource );
    rendererResource->mActive = false;
    delete rendererResource;
  }

  void DrawCall2::CopyUniformSource( const void* bytes, int byteCount )
  {
    mUniformSrcc.resize( byteCount );
    MemCpy( mUniformSrcc.data(), bytes, byteCount );

  }

}

