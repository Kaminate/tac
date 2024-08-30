#include "tac_component_registry.h" // self-inc

#include "tac-std-lib/string/tac_string.h"

namespace Tac
{
  // I wonder if these can be out of sync between different builds of the exe
  // or between server/clients
  // maybe it should be sorted by entry name or something?

  static FixedVector< ComponentRegistryEntry, 10 > mEntries;

  ComponentRegistryEntry* ComponentRegistryIterator::begin() { return mEntries.begin(); }
  ComponentRegistryEntry* ComponentRegistryIterator::end() { return mEntries.end(); }

  int                    ComponentRegistryEntry::GetIndex() const
  {
    return ( int )( this - mEntries.data() );
  }

  ComponentRegistryEntry* ComponentRegistry_RegisterComponent()
  {
    mEntries.push_back( ComponentRegistryEntry() );
    return &mEntries.back();
  }

  ComponentRegistryEntry* ComponentRegistry_FindComponentByName( const char* name )
  {
    for( ComponentRegistryEntry& entry : mEntries )
      if( !StrCmp( entry.mName, name ) )
        return &entry;
    return nullptr;
  }

  //int                     ComponentRegistry_GetComponentCount() { return mEntries.size(); }

  //ComponentRegistryEntry* ComponentRegistry_GetComponentAtIndex( int i ) { return &mEntries[ i ]; }


#if 1

  Json ComponentSettings::Element::GetValue( const Component* component )
  {
    const void* pVal{ ( const char* )component + mByteOffset };

    switch( mType )
    {
    case JsonType::Bool:
    {
      const bool b{ *( const bool* )pVal };
      return Json( b );
    } break;

    case JsonType::Number:
    {
      const JsonNumber n{ *( const float* )pVal };
      return Json( n );
    } break;

    }

    TAC_ASSERT_INVALID_CODE_PATH;
    return {};
  }

  void ComponentSettings::Element::Save( SettingsNode parent, const Component* component )
  {
    SettingsNode child{ parent.GetChild( mPath ) };
    const Json value{ GetValue( component ) };
    child.SetValue( value );
  }


  void ComponentSettings::Element::SetValue( Json value, Component* component )
  {
      void* pVal{ ( char* )component + mByteOffset };

      switch( mType )
      {
      case JsonType::Bool: { *( bool* )pVal = value; } break;
      case JsonType::Number: { *( float* )pVal = ( float )( JsonNumber )value; } break;
      default: TAC_ASSERT_INVALID_CASE( mType );
      }
  }

  void ComponentSettings::Element::Load( SettingsNode parent, Component* component )
  {
    SettingsNode child{ parent.GetChild( mPath ) };
    const Json value{ child.GetValue() };
    SetValue( value, component );
  }

  void ComponentSettings::Register( Element e )
  {
    TAC_ASSERT(
      e.mType == JsonType::Number ||
      e.mType == JsonType::Bool
    );
    mElements.push_back( e );
  }

  void ComponentSettings::Save( SettingsNode parent, const Component* component )
  {
    for( Element& element : mElements )
      element.Save( parent, component );
  }

  void ComponentSettings::Load( SettingsNode node, Component* component )
  {
    for( Element& element : mElements )
      element.Load( node, component );
  }

  bool ComponentSettings::Empty() const { return mElements.empty(); }
#endif

} // namespace Tac

