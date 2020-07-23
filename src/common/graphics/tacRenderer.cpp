
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


  //Renderer::~Renderer()
  //{
    //for( RendererResource* rendererResource : mRendererResources )
    //{
    //  String errorMessage =
    //    "Resource leaked: " +
    //    rendererResource->mName +
    //    " created at " +
    //    rendererResource->mFrame.ToString();
    //  OS::DebugAssert( errorMessage, TAC_STACK_FRAME );
    //}
  //}

  //void Renderer::DebugImgui()
  //{
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
  //}

  const float sizeInMagicUISpaceUnits = 1024.0f;

  // Make this static?
  //void Renderer::GetUIDimensions(
  //  Texture* texture,
  //  float* width,
  //  float* height )
  //{
  //  float aspect = texture->GetAspect();
  //  float scaleWidth = 1;
  //  float scaleHeight = 1;
  //  if( aspect > 1 )
  //    scaleWidth = aspect;
  //  else
  //    scaleHeight = 1.0f / aspect;
  //  *width = scaleWidth * sizeInMagicUISpaceUnits;
  //  *height = scaleHeight * sizeInMagicUISpaceUnits;
  //}


  //void Renderer::RemoveRendererResource( RendererResource* rendererResource )
  //{
  //  if( !rendererResource )
  //    return;
  //  bool found = Contains( mRendererResources, rendererResource );
  //  TAC_ASSERT( found );
  //  mRendererResources.erase( rendererResource );
  //  rendererResource->mActive = false;
  //  delete rendererResource;
  //}

  //void DrawCall2::CopyUniformSource( const void* bytes, int byteCount )
  //{
  //  mUniformSrcc.resize( byteCount );
  //  MemCpy( mUniformSrcc.data(), bytes, byteCount );
  //}

  namespace Render
  {
    ConstantBuffers::ConstantBuffers( ConstantBufferHandle constantBufferHandle )
    {
      AddConstantBuffer( constantBufferHandle );

    }

    ConstantBuffers::ConstantBuffers( ConstantBufferHandle a,
                                      ConstantBufferHandle b )
    {
      AddConstantBuffer( a );
      AddConstantBuffer( b );
    }

    ConstantBuffers::ConstantBuffers( ConstantBufferHandle* constantBufferHandles, int n )
    {
      while( n-- )
        AddConstantBuffer( *constantBufferHandles++ );

    }

    void ConstantBuffers::AddConstantBuffer( ConstantBufferHandle handle )
    {
      mConstantBuffers[ mConstantBufferCount++ ] = handle;
    }

    void VertexDeclarations::AddVertexDeclaration( VertexDeclaration v )
    {
      mVertexFormatDatas[ mVertexFormatDataCount++ ] = v;
    }

    VertexDeclarations::VertexDeclarations( VertexDeclaration a )
    {
      AddVertexDeclaration( a );
    }

    VertexDeclarations::VertexDeclarations( VertexDeclaration a, VertexDeclaration b )
    {
      AddVertexDeclaration( a );
      AddVertexDeclaration( b );
    }
    DrawCallTextures::DrawCallTextures( TextureHandle a )
    {

      AddTexture( a );
    }
    DrawCallTextures::DrawCallTextures( TextureHandle a, TextureHandle b )
    {

      AddTexture( a );
      AddTexture( b );
    }
    void DrawCallTextures::AddTexture( TextureHandle v )
    {

      mTextures[ mTextureCount++ ] = v;

    }
    const TextureHandle* DrawCallTextures::begin() const
    {
      return mTextures;
    }
    const TextureHandle* DrawCallTextures::end() const
    {
      return mTextures + mTextureCount;
    }

    TextureHandle DrawCallTextures::operator[]( int i ) const
    {
      return mTextures[ i ];
    }

    ShaderSource ShaderSource::FromPath( StringView path )
    {
      ShaderSource result;
      result.mShaderPath = path;
      return result;
    }

    ShaderSource ShaderSource::FromStr( StringView str )
    {
      ShaderSource result;
      result.mShaderStr = str;
      return result;

    }
  }
}

