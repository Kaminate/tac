#pragma once

#include "tac-std-lib/tac_core.h"
#include "tac-rhi/renderer/tac_renderer.h"

namespace Tac
{
  typedef Mesh MeshLoadFunction( const AssetPathStringView&,
                                 const int iModel,
                                 const Render::VertexDeclarations&,
                                 Errors& );

  using MeshFileExt = StringView;

  void              ModelLoadFunctionRegister( MeshLoadFunction*, MeshFileExt );
  MeshLoadFunction* ModelLoadFunctionFind( MeshFileExt );
}

