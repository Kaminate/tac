#include "tac-ecs/system/tac_system.h"

#include "tac-std-lib/containers/tac_fixed_vector.h"


namespace Tac
{
  static FixedVector< SystemInfo, 10 > sSystemInfos;

  auto SystemInfo::Register() -> dynmc SystemInfo*
  {
    const int index{ sSystemInfos.size() };
    sSystemInfos.resize( index + 1 );
    return &sSystemInfos[ index ];
  }

  auto SystemInfo::Find( StringView name ) -> const SystemInfo*
  {
    for( const SystemInfo& systemInfo : sSystemInfos )
      if( StringView( systemInfo.mName ) == name )
        return &systemInfo;
    return nullptr;
  }

  auto SystemInfo::GetIndex() const -> int { return int( this - sSystemInfos.data() ); }

  auto SystemInfo::Iterate::begin() const -> const SystemInfo*{ return sSystemInfos.begin(); }

  auto SystemInfo::Iterate::end() const -> const SystemInfo* { return sSystemInfos.end(); }

} // namespace Tac
