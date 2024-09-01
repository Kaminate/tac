#include "tac_component.h" // self-inc

#include "tac-ecs/component/tac_component_registry.h"

namespace Tac
{
  void Component::CopyFrom( const Component* component )
  {
    const ComponentInfo* entry{ GetEntry() };
    const MetaType::CopyParams copyParams
    {
      .mDst { this },
      .mSrc { component },
    };
    entry->mMetaType->Copy( copyParams );
    //const NetVars& netVars{ entry->mNetVars };

    //const int nVars{ netVars.size() };

    //for( int iVar{}; iVar < nVars; ++iVar )
    //{
    //  const NetVar& var{ mNetVars[ iVar ] };
    //  const MetaMember* metaMember{ var.mMetaMember };
    //  dynmc void* dst{ ( dynmc char* )dstComponent + metaMember->mOffset };
    //  const void* src{ ( const char* )srcComponent + metaMember->mOffset };

    //  var.CopyFrom( dst, src );
    //}
  }
}

