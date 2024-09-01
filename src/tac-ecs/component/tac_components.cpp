#include "tac_components.h" // self-inc

namespace Tac
{

  void                      Components::Add( Component* component )
  {
    mComponents.push_back( component );
  }

  void                      Components::Clear()
  {
    mComponents.clear();
  }

  Components::ConstIter     Components::begin() const { return mComponents.begin(); }

  Components::ConstIter     Components::end() const   { return mComponents.end(); }

  Component*                Components::Remove( const ComponentRegistryEntry* entry )
  {
    for( auto it { mComponents.begin() }; it != mComponents.end(); ++it )
    {
      Component* component { *it };
      if( component->GetEntry() == entry )
      {
        mComponents.erase( it );
        return component;
      }
    }

    return nullptr;
  }

}
