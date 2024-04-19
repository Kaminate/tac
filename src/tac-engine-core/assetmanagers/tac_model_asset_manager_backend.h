#pragma once

#include "tac-std-lib/string/tac_string_view.h"
//#include "tac-rhi/render3/tac_render_api.h"

namespace Tac { struct Mesh; struct Errors; struct AssetPathStringView; }
namespace Tac::Render { struct VertexDeclarations; }

namespace Tac
{
  using MeshLoadFunction = Mesh (*)( const AssetPathStringView&,
                                 const int iModel,
                                 const Render::VertexDeclarations&,
                                 Errors& );

  using MeshFileExt = StringView;

  void             ModelLoadFunctionRegister( MeshLoadFunction, MeshFileExt );
  MeshLoadFunction ModelLoadFunctionFind( MeshFileExt );
}

