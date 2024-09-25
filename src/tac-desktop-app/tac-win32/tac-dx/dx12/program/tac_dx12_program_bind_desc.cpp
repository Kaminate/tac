#include "tac_dx12_program_bind_desc.h" // self-inc


namespace Tac::Render
{


  bool D3D12ProgramBindDesc::BindsAsDescriptorTable() const
  {
    // Samplers and textures can only be set through descriptor tables ( not root descriptors )
    return mBindCount != 1
      || mType.IsSampler()
      || mType.IsUAV() // typed uav cannot be root descriptor
      || mType.IsTexture();
  }


  // -----------------------------------------------------------------------------------------------

  D3D12ProgramBindDescs::D3D12ProgramBindDescs( const D3D12_SHADER_INPUT_BIND_DESC* descs, int n )
  {
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
