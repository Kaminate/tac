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

  static VertexAttributeFormat FromFloats( int n )
  {
    return VertexAttributeFormat::FromElements( FormatElement::GetFloat(), n );
  }

  // -----------------------------------------------------------------------------------------------

  ctor      IContext::Scope::Scope( IContext* context ) { mContext = context; }
  dtor      IContext::Scope::~Scope()                   { if( mContext ) mContext->Retire(); }
  IContext* IContext::Scope::GetContext()               { return mContext; }
#if 0
  /*oper*/      IContext::Scope::operator IContext* ()      { return mContext; }
#endif
  IContext* IContext::Scope::operator ->()              { return mContext; }

  // -----------------------------------------------------------------------------------------------

  ctor IBindlessArray::IBindlessArray( Params params )
    : mHandleType{ params.mHandleType }
    , mBinding{ params.mBinding }
  {
  }
  ctor IBindlessArray::Binding::Binding( int index ) : mIndex{ index } {}
  bool IBindlessArray::Binding::IsValid() const  { return mIndex != Binding{}.mIndex; }
  int  IBindlessArray::Binding::GetIndex() const { return mIndex; }

  // -----------------------------------------------------------------------------------------------

  //ctor IHandle::IHandle( int i ) : mIndex( i ) {}
  //int  IHandle::GetIndex() const               { TAC_ASSERT( IsValid() ); return mIndex; }
  //bool IHandle::IsValid() const                { return mIndex != -1; }

  // -----------------------------------------------------------------------------------------------

  void             RenderApi::Init( InitParams params, Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
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
  void             RenderApi::BeginRenderFrame( Errors& errors )
  {
    TAC_CALL( sDevice->BeginRenderFrame( errors ) );
  }
  void             RenderApi::EndRenderFrame( Errors& errors )
  {
    TAC_CALL( sDevice->EndRenderFrame( errors ) );
    sCurrentRenderFrameIndex++;
  }
  u64              RenderApi::GetCurrentRenderFrameIndex()       { return sCurrentRenderFrameIndex; }


  // -----------------------------------------------------------------------------------------------

  FormatElement FormatElement::GetFloat()
  {
    return FormatElement
    {
      .mPerElementByteCount { sizeof( float ) },
      .mPerElementDataType  { GraphicsType::real },
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

  int VertexDeclarations::CalculateStride() const
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
