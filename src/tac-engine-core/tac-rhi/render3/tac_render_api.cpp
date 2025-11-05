#include "tac_render_api.h" //self-inc
#include "tac_render_backend.h"
#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac
{
#if 0
  constexpr Render::BindingMask::operator Binding() const { return mBinding; }
  constexpr Render::BindingMask::operator bool() const { return mBinding != Binding::None; }
  constexpr auto Render::operator | ( Binding lhs, Binding rhs ) -> BindingMask { return { ( Binding )( ( int )lhs | ( int )rhs ) }; }
  constexpr auto Render::operator & ( Binding lhs, Binding rhs ) -> BindingMask { return { ( Binding )( ( int )lhs & ( int )rhs ) }; }
#endif

  int Render::GetTexFmtSize( const TexFmt fmt )
  {
    switch( fmt )
    {
    case TexFmt::kRGBA16F:                   return 8;
    case TexFmt::kR8_unorm:                  return 1;
    case TexFmt::kR16_uint:                  return 2;
    case TexFmt::kR32_uint:                  return 4;
    case TexFmt::kRGBA8_unorm:               return 4;
    case TexFmt::kRGBA8_unorm_srgb:          return 4;
    default: TAC_ASSERT_INVALID_CASE( fmt ); return 0;
    }
  }
}

namespace Tac::Render
{
  static int              sMaxGPUFrameCount; 
  static FileSys::Path    sShaderOutputPath; 
  static IDevice*         sDevice;
  static u64              sCurrentRenderFrameIndex;

   // -----------------------------------------------------------------------------------------------

  static auto FromFloats( int n ) -> VertexAttributeFormat
  {
    return VertexAttributeFormat::FromElements( FormatElement::GetFloat(), n );
  }

  // -----------------------------------------------------------------------------------------------

  IContext::Scope::Scope( IContext* context )           { mContext = context; }
  IContext::Scope::~Scope()                             { if( mContext ) mContext->Retire(); }
  IContext* IContext::Scope::GetContext()               { return mContext; }
#if 0
  /*oper*/      IContext::Scope::operator IContext* ()      { return mContext; }
#endif
  IContext* IContext::Scope::operator ->()              { return mContext; }

  // -----------------------------------------------------------------------------------------------

  IBindlessArray::IBindlessArray( Params params )
    : mHandleType{ params.mHandleType }
    , mBinding{ params.mBinding }
  {
  }
  IBindlessArray::Binding::Binding( int index ) : mIndex{ index } {}
  bool IBindlessArray::Binding::IsValid() const         { return mIndex != Binding{}.mIndex; }
  auto IBindlessArray::Binding::GetIndex() const -> int { return mIndex; }

  // -----------------------------------------------------------------------------------------------

  void RenderApi::Init( InitParams params, Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    sMaxGPUFrameCount = params.mMaxGPUFrameCount;
    sShaderOutputPath = params.mShaderOutputPath;
  }
  void RenderApi::Uninit()
  {
    // ...
  }
  auto RenderApi::GetMaxGPUFrameCount() -> int           { return sMaxGPUFrameCount; }
  auto RenderApi::GetShaderOutputPath() -> FileSys::Path { return sShaderOutputPath; }
  auto RenderApi::GetRenderDevice() -> IDevice*          { return sDevice; }
  void RenderApi::SetRenderDevice( IDevice* device )     { sDevice = device; }
  void RenderApi::BeginRenderFrame( Errors& errors )
  {
    TAC_CALL( sDevice->BeginRenderFrame( errors ) );
  }
  void RenderApi::EndRenderFrame( Errors& errors )
  {
    TAC_CALL( sDevice->EndRenderFrame( errors ) );
    sCurrentRenderFrameIndex++;
  }
  auto RenderApi::GetCurrentRenderFrameIndex() -> u64    { return sCurrentRenderFrameIndex; }


  // -----------------------------------------------------------------------------------------------

  auto FormatElement::GetFloat() -> FormatElement
  {
    return FormatElement
    {
      .mPerElementByteCount { sizeof( float ) },
      .mPerElementDataType  { GraphicsType::real },
    };
  };

  // -----------------------------------------------------------------------------------------------

  auto VertexAttributeFormat::FromElements( FormatElement element, int n ) -> VertexAttributeFormat
  {
    return
    {
      .mElementCount        { n },
      .mPerElementByteCount { element.mPerElementByteCount },
      .mPerElementDataType  { element.mPerElementDataType },
    };
  }
  auto VertexAttributeFormat::CalculateTotalByteCount() const-> int 
  {
    return mElementCount * mPerElementByteCount;
  }
  auto VertexAttributeFormat::GetFloat() -> VertexAttributeFormat   { return FromFloats( 1 ); }
  auto VertexAttributeFormat::GetVector2() -> VertexAttributeFormat { return FromFloats( 2 ); }
  auto VertexAttributeFormat::GetVector3() -> VertexAttributeFormat { return FromFloats( 3 ); }
  auto VertexAttributeFormat::GetVector4() -> VertexAttributeFormat { return FromFloats( 4 ); }

  // -----------------------------------------------------------------------------------------------

  auto VertexDeclarations::CalculateStride() const-> int 
  {
    int maxStride {};
    for( const VertexDeclaration& decl : *this )
    {
      const int curStride { decl.mAlignedByteOffset + decl.mFormat.CalculateTotalByteCount() };
      maxStride = Max( maxStride, curStride );
    }

    return maxStride;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render
