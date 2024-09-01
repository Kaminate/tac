#pragma once
#include "tac-std-lib/containers/tac_list.h"
#include "tac-ecs/component/tac_component.h"
namespace Tac
{
  struct Components
  {
    using Container = List< Component* >;
    using ConstIter = Container::ConstIterator;

    void        Add( Component* );
    void        Clear();
    Component*  Remove( const ComponentInfo* );
    ConstIter   begin() const;
    ConstIter   end() const;

  private:
    Container   mComponents;
  };
}
