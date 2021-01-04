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
      default: TAC_ASSERT_INVALID_CASE( attribType ); return nullptr;
    }
  }

  v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated )
  {
    return {
      colorAlphaUnassociated.x * colorAlphaUnassociated.w,
      colorAlphaUnassociated.y * colorAlphaUnassociated.w,
      colorAlphaUnassociated.z * colorAlphaUnassociated.w,
      colorAlphaUnassociated.w };
  }

  ScissorRect::ScissorRect( float w, float h )
  {
    mXMaxRelUpperLeftCornerPixel = w;
    mYMaxRelUpperLeftCornerPixel = h;
  }
  ScissorRect::ScissorRect( int w, int h )
  {
    mXMaxRelUpperLeftCornerPixel = ( float )w;
    mYMaxRelUpperLeftCornerPixel = ( float )h;
  }

  Viewport::Viewport( int w, int h ) { mWidth = ( float )w; mHeight = ( float )h; }
  Viewport::Viewport( float w, float h ) { mWidth = w; mHeight = h; }

  int Format::CalculateTotalByteCount() const
  {
    return mElementCount * mPerElementByteCount;
  }

  const float sizeInMagicUISpaceUnits = 1024.0f;

  static Vector< RendererFactory* >& GetRendererFactories()
  {
    static Vector< RendererFactory* > result;
    return result;
  }

  RendererFactory* RendererFactoriesFind( StringView name )
  {
    for( RendererFactory* factory : RendererRegistry() )
      if( factory->mRendererName == name )
        return factory;
    return nullptr;
  }

  void RendererFactoriesRegister( RendererFactory* rendererFactory )
  {
    GetRendererFactories().push_back( rendererFactory );
  }

  RendererFactory** RendererRegistry::begin() { return GetRendererFactories().begin(); }
  RendererFactory** RendererRegistry::end() { return GetRendererFactories().end(); }

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

