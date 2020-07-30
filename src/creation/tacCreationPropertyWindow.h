#pragma once

#include "src/common/tacString.h"
//#include "src/common/tacDesktopWindow.h"

namespace Tac
{
  struct Creation;
  /*struct DesktopWindow*/;
  struct Entity;
  struct Errors;
  struct Shell;
  struct UI2DDrawData;
  struct UIHierarchyNode;
  struct UIRoot;



  struct CreationPropertyWindow
  {
    static CreationPropertyWindow* Instance;
    CreationPropertyWindow();
    ~CreationPropertyWindow();
    void Init( Errors& errors );
    void Update( Errors& errors );
    void RecursiveEntityHierarchyElement( Entity* );

    DesktopWindowHandle mDesktopWindowHandle;
    UI2DDrawData* mUI2DDrawData = nullptr;

    //UIHierarchyNode* mHierarchyList = nullptr;
    //UIHierarchyNode* mHierarchyPane = nullptr;
    //UIHierarchyNode* mInspector = nullptr;
  };

  const String gPropertyWindowName = "PropertyWindow";

}
