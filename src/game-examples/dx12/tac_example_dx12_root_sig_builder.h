#pragma once

#include "src/shell/windows/tac_win32_com_ptr.h"
#include "src/common/containers/tac_span.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/containers/tac_fixed_vector.h"

#include <d3d12.h> // ID3D12...

namespace Tac { struct Errors; }

namespace Tac::Render
{

  struct RootSignatureBuilder
  {
    RootSignatureBuilder( ID3D12Device* device ) ;

    void AddConstantBuffer( D3D12_SHADER_VISIBILITY,
                            D3D12_ROOT_DESCRIPTOR1 );

    void AddRootDescriptorTable( D3D12_SHADER_VISIBILITY ,
                                 D3D12_DESCRIPTOR_RANGE1 );
    

    void AddRootDescriptorTable( D3D12_SHADER_VISIBILITY ,
                                 Span< D3D12_DESCRIPTOR_RANGE1 > );
    

    PCom< ID3D12RootSignature > Build( Errors& errors );
    

  private:
    Vector< D3D12_ROOT_PARAMETER1 > mRootParams;

    // why not just make this a vector
    FixedVector< D3D12_DESCRIPTOR_RANGE1, 100 > mRanges;

    ID3D12Device* mDevice;
  };
}
