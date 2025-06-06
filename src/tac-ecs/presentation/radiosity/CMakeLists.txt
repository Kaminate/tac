target_sources( TacECS
  PUBLIC
  "tac_radiosity_bake_presentation.h"

  # I tried to name this tac_radiosity_bake.h, but either cmake or visual studio would fuck up
  # the include directories. weird!
  "tac_radiosity_baker.h"

  PRIVATE
  "tac_radiosity_bake_presentation.cpp"
  "tac_radiosity_baker.cpp"

)
