#pragma once

#include "tac-std-lib/dataprocess/tac_serialization.h" // NetVars
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-ecs/tac_space.h"

namespace Tac
{
#if 0
  //  Used for serializing components to/from disk
  //
  // commented out because im not sure what Element::Path should be.
  // this is Json, not SettingsNode, i cant mix in "foo.bar[9]" since Json will treat the whole
  // thing as a key
  //
  // ^ uncommented to replace Json with SettingsNode
  //
  struct ComponentSettings
  {
    struct Element
    {
      Json GetValue( const Component* );
      void SetValue( Json, Component* );
      void Save( SettingsNode, const Component* );
      void Load( SettingsNode, Component* );

      String   mPath       {};
      JsonType mType       {};
      int      mByteOffset {};
    };

    void Register( Element );
    void Save( SettingsNode, const Component* );
    void Load( SettingsNode, Component* );
    bool Empty() const;

    Vector< Element > mElements;
  };
#endif


  struct ComponentInfo
  {
    using  ComponentCreateFn       = Component* (*) ( World* );
    using  ComponentDestroyFn      = void       (*) ( World*, Component* );
    using  ComponentDebugImguiFn   = void       (*) ( Component* );
    using  ComponentSaveFn         = void       (*) ( Json&, Component* );
    using  ComponentLoadFn         = void       (*) ( Json&, Component* );
    using  ComponentSettingsSaveFn = void       (*) ( SettingsNode, Component* );
    using  ComponentSettingsLoadFn = void       (*) ( SettingsNode, Component* );

    struct Iterate
    {
      ComponentInfo* begin();
      ComponentInfo* end();
    };

    static ComponentInfo*  Register();
    static ComponentInfo*  Find( const char* );

    int                    GetIndex() const;

    //                     Used for
    //                     - debugging network bits
    //                     - prefab serialization
    const char*            mName         {};

    //                     Used to create components at runtime
    //                     ( from prefabs, or hardcode, or in editor, or whenever )
    //                     Component* ( *mCreateFn )( World* ) = nullptr;
    ComponentCreateFn       mCreateFn     {};
    ComponentDestroyFn      mDestroyFn    {};
    ComponentDebugImguiFn   mDebugImguiFn {};
    //ComponentSaveFn         mSaveFn       {};
    //ComponentLoadFn         mLoadFn       {};
    ComponentSettingsSaveFn mSettingsSaveFn       {};
    ComponentSettingsLoadFn mSettingsLoadFn       {};


    //                     Used for serializing components over the network
    //NetVars                mNetVars;

    NetVarRegistration       mNetVarRegistration {};
    //const MetaCompositeType* mMetaType           {};
    const MetaType* mMetaType           {};

    //ComponentSettings      mComponentSettings;
  };


  // Index of a registered component in the ComponentInfo
  //enum ComponentRegistryIndex : int {};

  //struct ComponentRegistryIndexes
  //{
  //  struct Iterator
  //  {
  //    ComponentRegistryIndex operator*() { return ComponentRegistryIndex{ mIndex }; }
  //    void operator++() { mIndex++; }
  //    int mIndex;
  //  };
  //  Iterator begin() { return {}; }
  //  Iterator end() { return { ComponentRegistry_GetComponentCount() }; }
  //};


  //int                     ComponentRegistry_GetComponentCount();
  //ComponentInfo* ComponentRegistry_GetComponentAtIndex( int );

}

