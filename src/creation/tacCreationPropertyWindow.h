#pragma once

#include "src/common/tacString.h"

namespace Tac
{
  struct Entity;
  struct Errors;

  struct CreationPropertyWindow
  {
    static CreationPropertyWindow* Instance;
    CreationPropertyWindow();
    ~CreationPropertyWindow();
    void Init( Errors& errors );
    void Update( Errors& errors );
    void RecursiveEntityHierarchyElement( Entity* );

    DesktopWindowHandle mDesktopWindowHandle;
  };

  const String gPropertyWindowName = "PropertyWindow";

}
