#include "tac_dx12_program_bind_desc.h" // self-inc


namespace Tac::Render
{


  // For visual examples of what is/isn't a descriptor table, see:
  //   https://learn.microsoft.com/en-us/windows/win32/direct3d12/example-root-signatures
  bool D3D12ProgramBindDesc::BindsAsDescriptorTable() const
  {
    // There exist certain types of root UAVs and root SRVs, but we will not use them.
    return mBindCount != 1
      || mType.IsSampler()
      || mType.IsTexture()
      || mType.IsUAV()
      || mType.IsSRV();
  }


  // -----------------------------------------------------------------------------------------------

  D3D12ProgramBindDescs::D3D12ProgramBindDescs( const DXCReflInfo::BindDescs& descs )
  {
    const int n{ descs.size() };
    reserve( n );
    for( int i{}; i < n; ++i )
    {
      const D3D12_SHADER_INPUT_BIND_DESC& info { descs[ i ] };
      const D3D12ProgramBindType type( info );
      const D3D12ProgramBindDesc binding
      {
        .mType          { type },
        .mName          { info.Name },
        .mBindCount     { ( int )info.BindCount },
        .mBindRegister  { ( int )info.BindPoint },
        .mRegisterSpace { ( int )info.Space },
      };

      push_back( binding );
    }
  }


} // namespace Tac::Render
