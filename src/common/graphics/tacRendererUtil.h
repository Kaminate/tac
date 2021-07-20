// This file is for things that you may want to use after including tacRenderer.h
#pragma once

//#include "src/common/graphics/tacRenderer.h"
//#include "src/common/math/tacVector4.h"
#include "src/common/math/tacMatrix4.h"
//#include "src/common/tacPreprocessor.h"

namespace Tac
{
  // this should really be called like per camera data
  struct DefaultCBufferPerFrame
  {
    static const char* name_view()        { return "View";        };
    static const char* name_proj()        { return "Projection";  };
    static const char* name_far()         { return "far";         };
    static const char* name_near()        { return "near";        };
    static const char* name_gbuffersize() { return "gbufferSize"; };
    static const int   shaderRegister = 0;
    m4                 mView;
    m4                 mProjection;
    float              mFar;
    float              mNear;
    v2                 mGbufferSize;
    float              mSecModTau;
  };

  struct DefaultCBufferPerObject
  {
    static const char* name_world()       { return "World";       };
    static const char* name_color()       { return "Color";       };
    static const int   shaderRegister = 1;
    m4                 World;
    v4                 Color;
  };

  // maybe this should be in renderer idk
  v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated );
}

