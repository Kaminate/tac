#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacShell.h"
#include "src/common/tacOS.h"
#include "src/common/containers/tacFixedVector.h"

namespace Tac
{
  namespace Render
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
        default: TAC_CRITICAL_ERROR_INVALID_CASE( attribType ); return nullptr;
      }
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

    static auto& GetRendererFactories()
    {
      static FixedVector< RendererFactory, 10 > result;
      return result;
    }

    RendererFactory* RendererRegistry::begin() { return GetRendererFactories().begin(); }
    RendererFactory* RendererRegistry::end() { return GetRendererFactories().end(); }

    RendererFactory* RendererFactoriesFind( StringView name )
    {
      for( RendererFactory& factory : GetRendererFactories() )
        if( !StrCmp( factory.mRendererName, name.c_str() ) )
          return &factory;
      return nullptr;
    }

    void RendererFactoriesRegister( RendererFactory rendererFactory )
    {
      GetRendererFactories().push_back( rendererFactory );
    }

    ShaderSource ShaderSource::FromPath( const char* path )
    {
      ShaderSource result;
      result.mType = ShaderSource::Type::kPath;
      result.mStr = path;
      return result;
    }

    ShaderSource ShaderSource::FromStr( const char* str )
    {
      ShaderSource result;
      result.mType = kStr;
      result.mStr = str;
      return result;

    }
  } // namespace Render

} // namespace Tac
