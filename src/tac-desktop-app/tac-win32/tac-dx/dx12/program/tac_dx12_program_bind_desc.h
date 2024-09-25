#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-dx/dx12/program/tac_dx12_program_bind_type.h"

#include <d3d12shader.h> // D3D12_SHADER_INPUT_BIND_DESC

namespace Tac::Render
{

  // Describes bind points in a shader program where one or more resources can be bound
  struct D3D12ProgramBindDesc
  {
    bool BindsAsDescriptorTable() const;

    D3D12ProgramBindType mType          {};
    String               mName          {};
    int                  mBindCount     { -1 }; // A value of 0 represents an unbounded array
    int                  mBindRegister  { -1 };
    int                  mRegisterSpace { -1 };
  };

  struct D3D12ProgramBindDescs : public Vector< D3D12ProgramBindDesc >
  {
    D3D12ProgramBindDescs() = default;
    D3D12ProgramBindDescs( const D3D12_SHADER_INPUT_BIND_DESC*, int );
  };

} // namespace Tac::Render
