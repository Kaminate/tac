#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h" // self-inc


namespace Tac::Render
{
  const char* DX12_HRESULT_ToString( const HRESULT hr )
  {
    switch( hr )
    {
    case D3D12_ERROR_ADAPTER_NOT_FOUND: return "D3D12_ERROR_ADAPTER_NOT_FOUND - The specified cached PSO was created on a different adapter and cannot be reused on the current adapter.";
    case D3D12_ERROR_DRIVER_VERSION_MISMATCH: return "D3D12_ERROR_DRIVER_VERSION_MISMATCH - The specified cached PSO was created on a different driver version and cannot be reused on the current adapter.";
    case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL - The method call is invalid.For example, a method's parameter may not be a valid pointer.";
    case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING - The previous blit operation that is transferring information to or from this surface is incomplete.";
    case E_FAIL: return "E_FAIL - Attempted to create a device with the debug layer enabled and the layer is not installed.";
    case E_INVALIDARG: return "E_INVALIDARG - An invalid parameter was passed to the returning function.";
    case E_OUTOFMEMORY: return "E_OUTOFMEMORY - Direct3D could not allocate sufficient memory to complete the call.";
    case E_NOTIMPL: return "E_NOTIMPL - The method call isn't implemented with the passed parameter combination.";
    case S_FALSE: return "S_FALSE - Alternate success value, indicating a successful but nonstandard completion( the precise meaning depends on context ).";
    case S_OK: return "S_OK - No error occurred.";
    default: TAC_ASSERT_INVALID_CASE( hr ); return "???";
    }
  }

  static WCHAR* ToWStr( StringView sv )
  {
    const int n = 100;
    TAC_ASSERT( sv.size() < n );
    static WCHAR buf[ n ];
    int i = 0;
    for( char c : sv )
      buf[ i++ ] = c;
    buf[ i ] = '\0';
    return buf;
  }

  void DX12CallAux( const char* fn, const HRESULT hr, Errors& errors )
  {
    String msg = fn;
    msg += " failed with ";
    msg += DX12_HRESULT_ToString( hr );

    errors.Append( msg );
  }

  void DX12SetName( ID3D12Object* obj, StringView sv )
  {
    const HRESULT hr = obj->SetName( ToWStr( sv ) );
    TAC_ASSERT( hr == S_OK );
  }

} // namespace Tac::Render
