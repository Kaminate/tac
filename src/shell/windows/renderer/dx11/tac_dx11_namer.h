// 

#pragma once

#include "src/common/tac_common.h"
#include <d3d11_3.h> // ID3D11DeviceChild

namespace Tac::Render
{
  String ID3D11DeviceChildGetName( ID3D11DeviceChild* );
  void ID3D11DeviceChildSetName( ID3D11DeviceChild* , const StringView& );

  // -----------------------------------------------------------------------------------------------

  void SetDebugNameAux( IDXGIObject*, const StringView&, const StringView& );
  void SetDebugNameAux( ID3D11DeviceChild* directXObject, const StringView&, const StringView& );

  // -----------------------------------------------------------------------------------------------

  template< typename T > inline const char* GetShortName() = delete;
  template< typename T> inline void SetDebugName( T* t, const StringView& name )
  {
    if( !t )
      return;

    const char* suffix = GetShortName<T>();
    SetDebugNameAux( t, name, suffix );
  }

  // -----------------------------------------------------------------------------------------------

#define Name(T, str) template<> inline const char* GetShortName<T>() { return str; }
  Name( ID3D11BlendState,          "bs" );
  Name( ID3D11Buffer,              "buf" );
  Name( ID3D11ComputeShader,       "cs" );
  Name( ID3D11DepthStencilState,   "dss" );
  Name( ID3D11DepthStencilView,    "dsv" );
  Name( ID3D11GeometryShader,      "gs" );
  Name( ID3D11InputLayout,         "il" );
  Name( ID3D11PixelShader,         "ps" );
  Name( ID3D11RasterizerState,     "rs" );
  Name( ID3D11RasterizerState2,    "rs2" );
  Name( ID3D11RenderTargetView,    "rtv" );
  Name( ID3D11SamplerState,        "ss" );
  Name( ID3D11ShaderResourceView,  "srv" );
  Name( ID3D11Texture2D,           "2d" );
  Name( ID3D11Texture3D,           "3d" );
  Name( ID3D11UnorderedAccessView, "uav" );
  Name( ID3D11VertexShader,        "vs" );
  Name( IDXGISwapChain,            "sc" );
#undef Name




} // namespace Tac::Render

