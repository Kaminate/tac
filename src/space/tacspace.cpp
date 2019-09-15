#include "space/tacspace.h"
#include "space/graphics/tacgraphics.h"
#include "space/physics/tacphysics.h"


void TacSpaceInit()
{
  TacGraphics::TacSpaceInitGraphics();
  TacPhysics::TacSpaceInitPhysics();

  for( TacSystemRegistryEntry* entry : TacSystemRegistry::Instance()->mEntries )
  {
    TacAssert( entry->mName.size() );
  }

  for( TacComponentRegistryEntry* entry : TacComponentRegistry::Instance()->mEntries )
  {
    TacAssert( entry->mName.size() );
  }
}
