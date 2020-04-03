#include "common/graphics/tacRenderer.h"
#include "common/tacPreprocessor.h"
#include "common/tacAlgorithm.h"
#include "common/tacShell.h"
#include "common/tacOS.h"

const char* TacGetSemanticName( TacAttribute attribType )
{
  switch( attribType )
  {
  case TacAttribute::Position: return "POSITION";
  case TacAttribute::Normal: return "NORMAL";
  case TacAttribute::Texcoord: return "TEXCOORD";
  case TacAttribute::Color: return "COLOR";
  case TacAttribute::BoneIndex: return "BONEINDEX";
  case TacAttribute::BoneWeight: return "BONEWEIGHT";
  case TacAttribute::Coeffs: return "COEFFS";
    TacInvalidDefaultCase( attribType );
  }
  return nullptr;
}

TacString TacToString( TacRendererType rendererType )
{
  switch( rendererType )
  {
  case TacRendererType::Vulkan: return "Vulkan";
  case TacRendererType::OpenGL4: return "OpenGL4";
  case TacRendererType::DirectX12: return "DirectX12";
    TacInvalidDefaultCase( rendererType );
  }
  TacInvalidCodePath;
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


int TacFormat::CalculateTotalByteCount() const
{
  return mElementCount * mPerElementByteCount;
}


TacRenderer* TacRenderer::Instance = nullptr;
TacRenderer::TacRenderer()
{
  Instance = this;
}

TacRenderer::~TacRenderer()
{
  for( TacRendererResource* rendererResource : mRendererResources )
  {
    TacString errorMessage =
      "Resource leaked: " +
      rendererResource->mName +
      " created at " +
      rendererResource->mStackFrame.ToString();
    TacOS::Instance->DebugAssert( errorMessage, TAC_STACK_FRAME );
  }
}

void TacRenderer::DebugImgui()
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
  //  TacVector< TacShader*>shaders;
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
  //  TacVector< TacTexture* > textures;
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
void TacRenderer::GetUIDimensions(
  TacTexture* texture,
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


void TacRendererFactory::CreateRendererOuter( TacRenderer** renderer )
{
  CreateRenderer( renderer );
  ( *renderer )->mName = mRendererName;
}

TacRendererRegistry& TacRendererRegistry::Instance()
{
  // This variable must be inside this function or else the
  // renderers will add themselves too early or something
  // and then be stomped with an empty registry
  static TacRendererRegistry TacRendererRegistryInstance;
  return TacRendererRegistryInstance;
}
void TacRenderer::RemoveRendererResource( TacRendererResource* rendererResource )
{
  if( !rendererResource )
    return;
  bool found = TacContains( mRendererResources, rendererResource );
  TacAssert( found );
  mRendererResources.erase( rendererResource );
  rendererResource->mActive = false;
  delete rendererResource;
}

void TacDrawCall2::CopyUniformSource( const void* bytes, int byteCount )
{
  mUniformSrcc.resize(byteCount);
  TacMemCpy(mUniformSrcc.data(), bytes, byteCount );

  }
