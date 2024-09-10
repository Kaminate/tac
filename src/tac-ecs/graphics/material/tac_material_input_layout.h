#pragma once

#include "tac-std-lib/meta/tac_meta_type.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/dataprocess/tac_json.h"

namespace Tac
{

  struct MaterialInputLayout
  {
    struct Variable
    {
      const MetaType* mMetaType     {};
      String          mName         {};
      String          mSemantic     {};
    };

    static MaterialInputLayout FromJson( const Json* );
    static Json                ToJson( const MaterialInputLayout& );

    Vector< Variable > mVariables;
  };


} // namespace Tac

