#pragma once

#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-engine-core/window/tac_window_handle.h"

#include <d3d12.h> // D3D12...

namespace Tac { struct Errors; }

namespace Tac
{
  WindowHandle DX12ExampleCreateWindow( const StringView, Errors& );
}

namespace Tac::Render
{

  struct DX12ExampleDebugLayer
  {
    void Init( Errors& );
    bool IsEnabled() const;

  private:
    bool                mDebugLayerEnabled {};
    PCom< ID3D12Debug > mDebug;
  };

  struct DX12ExampleDevice
  {
    // Responsible for creating the device
    void Init( const DX12ExampleDebugLayer&, Errors& );

    PCom< ID3D12Device >              mDevice;
    PCom< ID3D12DebugDevice >         mDebugDevice;
  };

  struct DX12ExampleInfoQueue
  {
    void Init( const DX12ExampleDebugLayer&, ID3D12Device*, Errors& );
    PCom< ID3D12InfoQueue >            mInfoQueue;
  };

  struct ClipSpacePosition3
  {
    explicit ClipSpacePosition3( v3 v ) : mValue( v ) {}
    explicit ClipSpacePosition3(float x, float y, float z) : mValue{ x,y,z } {}
    v3 mValue;
  };

  struct LinearColor3
  {
    explicit LinearColor3( v3 v ) : mValue( v ) {}
    explicit LinearColor3( float x, float y, float z ) : mValue{ x, y, z } {}
    v3 mValue;
  };

  struct TextureCoordinate2
  {
    explicit TextureCoordinate2( float u, float v ) : mValue{ u, v } {}
    v2 mValue;
  };

  bool DX12SupportsRayTracing( ID3D12Device*, Errors& );

} // namespace Tac::Render

