#include "tac_render_api.h" //self-inc
#include "tac_render_backend.h"
#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac
{
}

namespace Tac::Render
{
  static int              sMaxGPUFrameCount; 
  static Filesystem::Path sShaderOutputPath; 
  static IDevice*         sDevice;

  // -----------------------------------------------------------------------------------------------

  Binding operator | ( Binding lhs, Binding rhs ) { return ( Binding )( ( int )lhs | ( int )rhs ); }
  Binding operator & ( Binding lhs, Binding rhs ) { return ( Binding )( ( int )lhs & ( int )rhs ); }

  // -----------------------------------------------------------------------------------------------

  IContext::Scope::Scope( IContext* context ) { mContext = context; }
  IContext::Scope::~Scope()                   { mContext->Retire(); }
  IContext* IContext::Scope::operator ->()    { return mContext; }

  // -----------------------------------------------------------------------------------------------

  IHandle::IHandle( int i ) : mIndex( i ) {}
  int IHandle::GetIndex() const { TAC_ASSERT( IsValid() ); return mIndex; }
  bool IHandle::IsValid() const { return mIndex != -1; }

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
  int              RenderApi::GetMaxGPUFrameCount() { return sMaxGPUFrameCount; }
  Filesystem::Path RenderApi::GetShaderOutputPath() { return sShaderOutputPath; }
  IDevice*         RenderApi::GetRenderDevice()     { return sDevice; }

  // -----------------------------------------------------------------------------------------------


  Format Format::FromElements( FormatElement element, int n )
  {
    return
    {
      .mElementCount = n,
      .mPerElementByteCount = element.mPerElementByteCount,
      .mPerElementDataType = element.mPerElementDataType,
    };
  }

  int    Format::CalculateTotalByteCount() const
  {
    return mElementCount * mPerElementByteCount;
  }

  const FormatElement FormatElement::sFloat =
  {
      .mPerElementByteCount = sizeof( float ),
      .mPerElementDataType = GraphicsType::real,
  };
  const Format Format::sfloat = Format::FromElements( FormatElement::sFloat, 1 );
  const Format Format::sv2 = Format::FromElements( FormatElement::sFloat, 2 );
  const Format Format::sv3 = Format::FromElements( FormatElement::sFloat, 3 );
  const Format Format::sv4 = Format::FromElements( FormatElement::sFloat, 4 );


  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render
