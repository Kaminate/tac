#include "tac_render_api.h" //self-inc
#include "tac_render_backend.h"
#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac
{
  // still needed, even if Binding is not an enum class
  Render::Binding Render::operator | ( Binding lhs, Binding rhs ) { return Binding{ ( int )lhs | ( int )rhs }; }
  Render::Binding Render::operator & ( Binding lhs, Binding rhs ) { return Binding{ ( int )lhs & ( int )rhs }; }

  int Render::GetTexFmtSize( const TexFmt fmt )
  {
    switch( fmt )
    {
    case TexFmt::kRGBA16F: return 8;
    case TexFmt::kR8_unorm: return 1;
    case TexFmt::kR16_uint: return 2;
    case TexFmt::kR32_uint: return 4;
    case TexFmt::kRGBA8_unorm: return 4;
    case TexFmt::kRGBA8_unorm_srgb: return 4;
    default: TAC_ASSERT_INVALID_CASE( fmt ); return 0;
    }
  }
}

namespace Tac::Render
{
  static int              sMaxGPUFrameCount; 
  static FileSys::Path    sShaderOutputPath; 
  static IDevice*         sDevice;

  // -----------------------------------------------------------------------------------------------

  static VertexAttributeFormat FromFloats( int n )
  {
    return VertexAttributeFormat::FromElements( FormatElement::GetFloat(), n );
  }

  // -----------------------------------------------------------------------------------------------

  IContext::Scope::Scope( IContext* context ) { mContext = context; }
  IContext::Scope::~Scope()                   { if( mContext ) mContext->Retire(); }
  IContext* IContext::Scope::operator ->()    { return mContext; }

  // -----------------------------------------------------------------------------------------------

  //ctor IHandle::IHandle( int i ) : mIndex( i ) {}
  //int  IHandle::GetIndex() const               { TAC_ASSERT( IsValid() ); return mIndex; }
  //bool IHandle::IsValid() const                { return mIndex != -1; }

  // -----------------------------------------------------------------------------------------------

  void             RenderApi::Init( InitParams params, Errors& errors )
  {
    sMaxGPUFrameCount = params.mMaxGPUFrameCount;
    sShaderOutputPath = params.mShaderOutputPath;
  }
  void             RenderApi::Uninit()
  {
    // ...
  }
  int              RenderApi::GetMaxGPUFrameCount()              { return sMaxGPUFrameCount; }
  FileSys::Path    RenderApi::GetShaderOutputPath()              { return sShaderOutputPath; }
  IDevice*         RenderApi::GetRenderDevice()                  { return sDevice; }
  void             RenderApi::SetRenderDevice( IDevice* device ) { sDevice = device; }

  // -----------------------------------------------------------------------------------------------

  FormatElement FormatElement::GetFloat()
  {
    return FormatElement
    {
      .mPerElementByteCount { sizeof( float ) },
      .mPerElementDataType { GraphicsType::real },
    };
  };

  // -----------------------------------------------------------------------------------------------

  VertexAttributeFormat VertexAttributeFormat::FromElements( FormatElement element, int n )
  {
    return
    {
      .mElementCount        { n },
      .mPerElementByteCount { element.mPerElementByteCount },
      .mPerElementDataType  { element.mPerElementDataType },
    };
  }
  int                   VertexAttributeFormat::CalculateTotalByteCount() const
  {
    return mElementCount * mPerElementByteCount;
  }
  VertexAttributeFormat VertexAttributeFormat::GetFloat()   { return FromFloats( 1 ); }
  VertexAttributeFormat VertexAttributeFormat::GetVector2() { return FromFloats( 2 ); }
  VertexAttributeFormat VertexAttributeFormat::GetVector3() { return FromFloats( 3 ); }
  VertexAttributeFormat VertexAttributeFormat::GetVector4() { return FromFloats( 4 ); }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render
