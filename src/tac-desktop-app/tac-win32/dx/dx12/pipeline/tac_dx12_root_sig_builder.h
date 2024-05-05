#pragma once

#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/containers/tac_vector.h"
//#include "tac-std-lib/containers/tac_fixed_vector.h"

#include <d3d12.h> // ID3D12...

namespace Tac { struct Errors; }

namespace Tac::Render { struct D3D12ProgramBinding; }
namespace Tac::Render
{

  struct DX12RootSigBuilder
  {
    struct Location
    {
      int mRegister;
      int mSpace;
    };

    DX12RootSigBuilder( ID3D12Device* );

    void AddRootDescriptor( D3D12_ROOT_PARAMETER_TYPE, Location );

    //void AddRootDescriptor( D3D12_ROOT_PARAMETER_TYPE,
    //                       D3D12_SHADER_VISIBILITY,
    //                       D3D12_ROOT_DESCRIPTOR1 );

    //void AddConstantBuffer( Location );

    void AddUnboundedArray( D3D12_DESCRIPTOR_RANGE_TYPE, Location );
    void AddBoundedArray( D3D12_DESCRIPTOR_RANGE_TYPE, int, Location );

    // Add a descriptor table that associated with a single descriptor range
    // ( This must be called for an unbounded array )
    //void AddRootDescriptorTable( D3D12_SHADER_VISIBILITY ,
    //                             D3D12_DESCRIPTOR_RANGE1 );
    

    // Add a descriptor table associated with several descriptor ranges
    // ( for example, a SRV range, CBV range, and UAV range )
    //void AddRootDescriptorTable( D3D12_SHADER_VISIBILITY ,
    //                             Span< D3D12_DESCRIPTOR_RANGE1 > );
    
    void AddBindings( const D3D12ProgramBinding* , int );
    void SetInputLayoutEnabled( bool );

    PCom< ID3D12RootSignature > Build( Errors& errors );
    

  private:

    D3D12_DESCRIPTOR_RANGE1* AddRange( int n = 1 );
    void AddArrayInternal( D3D12_DESCRIPTOR_RANGE_TYPE, UINT, Location );

    Vector< D3D12_ROOT_PARAMETER1 >   mRootParams;
    Vector< D3D12_DESCRIPTOR_RANGE1 > mRanges;
    Vector< int >                     mRangeOffsets;
    ID3D12Device*                     mDevice;
    bool                              mHasInputLayout{};
  };
}
