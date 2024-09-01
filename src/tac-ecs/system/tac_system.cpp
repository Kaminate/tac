#include "tac-ecs/system/tac_system.h"

#include "tac-std-lib/containers/tac_fixed_vector.h"


namespace Tac
{
  static FixedVector< SystemInfo, 10 > sSystemInfos;

  dynmc SystemInfo* SystemInfo::Register()
  {
    const int index{ sSystemInfos.size() };
    sSystemInfos.resize( index + 1 );
    return &sSystemInfos[ index ];
  }

  const SystemInfo* SystemInfo::Find( StringView name )
  {
    for( const SystemInfo& systemInfo : sSystemInfos )
      if( systemInfo.mName == name )
        return &systemInfo;
    return nullptr;
  }

  int               SystemInfo::GetIndex() const { return int( this - sSystemInfos.data() ); }

  const SystemInfo* SystemInfo::Iterate::begin() const { return sSystemInfos.begin(); }

  const SystemInfo* SystemInfo::Iterate::end() const   { return sSystemInfos.end(); }

} // namespace Tac
