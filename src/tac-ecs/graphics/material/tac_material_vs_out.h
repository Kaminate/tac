#pragma once

#include "tac-std-lib/meta/tac_meta_type.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/dataprocess/tac_json.h"

namespace Tac
{
  // Describes, at a "material" level, the variables the vertex shader's output struct will contain
  // Used when generating the shader program
  struct MaterialVSOut
  {
    struct Variable
    {
      const MetaType* mMetaType     {};
      String          mName         {};
      String          mSemantic     {};
    };

    static MaterialVSOut FromJson( const Json* );
    static Json          ToJson( const MaterialVSOut& );

    Vector< Variable > mVariables;
  };


} // namespace Tac

