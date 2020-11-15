#pragma once

namespace Tac
{
  struct Entity;
  struct Errors;

  struct CreationPropertyWindow
  {
    CreationPropertyWindow();
    ~CreationPropertyWindow();
    static CreationPropertyWindow* Instance;
    void                           Init( Errors& );
    void                           Update( Errors& );
    void                           RecursiveEntityHierarchyElement( Entity* );
    DesktopWindowHandle            mDesktopWindowHandle;
  };

  const char* const gPropertyWindowName = "PropertyWindow";

}
