#include "tac_dx12_helper.h" // self-inc

#include <D3d9.h> // D3DERR_INVALIDCALL

namespace Tac
{
  const char* Render::DX12_HRESULT_ToString( const HRESULT hr )
  {
    switch( hr )
    {

    //// https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d11-graphics-reference-returnvalues
    //  case D3D11_ERROR_FILE_NOT_FOUND: return "";
    //  case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "";
    //  case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "";
    //  case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD: return "";

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/d3d12-graphics-reference-returnvalues
    case D3D12_ERROR_ADAPTER_NOT_FOUND:       return "D3D12_ERROR_ADAPTER_NOT_FOUND - The specified cached PSO was created on a different adapter and cannot be reused on the current adapter.";
    case D3D12_ERROR_DRIVER_VERSION_MISMATCH: return "D3D12_ERROR_DRIVER_VERSION_MISMATCH - The specified cached PSO was created on a different driver version and cannot be reused on the current adapter.";
    case DXGI_ERROR_INVALID_CALL:             return "DXGI_ERROR_INVALID_CALL - The method call is invalid.For example, a method's parameter may not be a valid pointer.";
    case DXGI_ERROR_WAS_STILL_DRAWING:        return "DXGI_ERROR_WAS_STILL_DRAWING - The previous blit operation that is transferring information to or from this surface is incomplete.";
    case E_FAIL:                              return "E_FAIL - Attempted to create a device with the debug layer enabled and the layer is not installed.";
    case E_INVALIDARG:                        return "E_INVALIDARG - An invalid parameter was passed to the returning function.";
    case E_OUTOFMEMORY:                       return "E_OUTOFMEMORY - Direct3D could not allocate sufficient memory to complete the call.";
    case E_NOTIMPL:                           return "E_NOTIMPL - The method call isn't implemented with the passed parameter combination.";
    case S_FALSE:                             return "S_FALSE - Alternate success value, indicating a successful but nonstandard completion( the precise meaning depends on context ).";
    case S_OK:                                return "S_OK - No error occurred.";

      // to handle the return value of D3DCompile
#define CASE( err ) case err: return #err;
    CASE( D3DERR_WRONGTEXTUREFORMAT );
    CASE( D3DERR_UNSUPPORTEDCOLOROPERATION );
    CASE( D3DERR_UNSUPPORTEDCOLORARG );
    CASE( D3DERR_UNSUPPORTEDALPHAOPERATION );
    CASE( D3DERR_UNSUPPORTEDALPHAARG );
    CASE( D3DERR_TOOMANYOPERATIONS );
    CASE( D3DERR_CONFLICTINGTEXTUREFILTER );
    CASE( D3DERR_UNSUPPORTEDFACTORVALUE );
    CASE( D3DERR_CONFLICTINGRENDERSTATE );
    CASE( D3DERR_UNSUPPORTEDTEXTUREFILTER );
    CASE( D3DERR_CONFLICTINGTEXTUREPALETTE );
    CASE( D3DERR_DRIVERINTERNALERROR );
    CASE( D3DERR_NOTFOUND );
    CASE( D3DERR_MOREDATA );
    CASE( D3DERR_DEVICELOST );
    CASE( D3DERR_DEVICENOTRESET );
    CASE( D3DERR_NOTAVAILABLE );
    CASE( D3DERR_OUTOFVIDEOMEMORY );
    CASE( D3DERR_INVALIDDEVICE );
    CASE( D3DERR_INVALIDCALL );
    CASE( D3DERR_DRIVERINVALIDCALL );
    CASE( D3DERR_WASSTILLDRAWING );
    CASE( D3DOK_NOAUTOGEN );

    default: TAC_ASSERT_INVALID_CASE( hr ); return "???";
    }
  }

  //static WCHAR* ToWStr( StringView sv )
  //{
  //  const int n = 100;
  //  TAC_ASSERT( sv.size() < n );
  //  static WCHAR buf[ n ];
  //  int i{};
  //  for( char c : sv )
  //    buf[ i++ ] = c;
  //  buf[ i ] = '\0';
  //  return buf;
  //}

  String Render::DX12CallAux( const char* fn, const HRESULT hr )
  {
    const String hrStr { DX12_HRESULT_ToString( hr ) };
    return String() + fn + " failed with " + hrStr;
  }

  void Render::DX12SetName( ID3D12Object* obj, StringView sv )
  {
    obj->SetPrivateData( WKPDID_D3DDebugObjectName, ( UINT )sv.size(), sv.data() );
  }

  void Render::DX12SetName( ID3D12Object* obj, DX12Name name )
  {
    if( !name.mName.empty() )
    {
      DX12SetName( obj, name.mName );
      return;
    }

    if( name.mStackFrame.IsValid() )
    {
      DX12SetName( obj, String()
                   + name.mStackFrame.GetFile()
                   + ":"
                   + Tac::ToString( name.mStackFrame.GetLine() ) );
      return;
    }

    TAC_ASSERT( !name.mResourceType.empty() );
    TAC_ASSERT( name.mResourceIndex != -1 );
    DX12SetName( obj, String() +
                 name.mResourceType + " " + Tac::ToString( name.mResourceIndex ) );
  }

} // namespace Tac::Render
