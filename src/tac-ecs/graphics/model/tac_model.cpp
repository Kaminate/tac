#include "tac_model.h" // self-inc

#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static int sRegistryIndex;

	static Component* CreateModelComponent( World* world )
	{
		return GetGraphics( world )->CreateModelComponent();
	}

	static void       DestroyModelComponent( World* world, Component* component )
	{
		GetGraphics( world )->DestroyModelComponent( ( Model* )component );
	}

	static void       SaveModelComponent( Json& modelJson, Component* component )
	{
		auto model { ( Model* )component };
		Json colorRGBJson;
		colorRGBJson[ "r" ].SetNumber( model->mColorRGB[ 0 ] );
		colorRGBJson[ "g" ].SetNumber( model->mColorRGB[ 1 ] );
		colorRGBJson[ "b" ].SetNumber( model->mColorRGB[ 2 ] );
		modelJson[ "mModelPath" ].SetString( model->mModelPath.c_str() );
    modelJson[ "mModelIndex" ].SetNumber( model->mModelIndex );
    //modelJson[ "mTryingNewThing" ].SetNumber( model->mTryingNewThing );
		modelJson[ "mColorRGB" ].DeepCopy( &colorRGBJson );
	}

	static void       LoadModelComponent( Json& modelJson, Component* component )
	{
		auto model { ( Model* )component };
		Json& colorRGBJson { modelJson[ "mColorRGB" ] };
    model->mModelIndex = ( int )modelJson[ "mModelIndex" ].mNumber;
		model->mModelPath = modelJson[ "mModelPath" ].mString;
		model->mColorRGB.x = ( float )colorRGBJson[ "r" ].mNumber;
		model->mColorRGB.y = ( float )colorRGBJson[ "g" ].mNumber;
		model->mColorRGB.z = ( float )colorRGBJson[ "b" ].mNumber;
	}


	const Model*                   Model::GetModel( const Entity* entity )
	{
    const ComponentRegistryEntry* entry = ComponentRegistry_GetComponentAtIndex( sRegistryIndex );
		return ( Model* )entity->GetComponent( entry );
	}

	Model*                         Model::GetModel( Entity* entity )
	{
    const ComponentRegistryEntry* entry = ComponentRegistry_GetComponentAtIndex( sRegistryIndex );
		return ( Model* )entity->GetComponent( entry );
	}

	const ComponentRegistryEntry*  Model::GetEntry() const
	{
    return ComponentRegistry_GetComponentAtIndex( sRegistryIndex );
	}

	void ModelDebugImgui( Model* );

  static void DebugImguiFn( Component* component )
  {
    ModelDebugImgui( (Model*) component );
  }

	void RegisterModelComponent()
	{
    ComponentRegistryEntry* entry { ComponentRegistry_RegisterComponent() };
    sRegistryIndex = entry->GetIndex(); 
    *entry = ComponentRegistryEntry
    {
      .mName { "Model" },
      //.mNetworkBits = ComponentModelBits,
      .mCreateFn { CreateModelComponent },
      .mDestroyFn { DestroyModelComponent },
      .mDebugImguiFn { DebugImguiFn },
      .mSaveFn { SaveModelComponent },
      .mLoadFn { LoadModelComponent },
    };
	}

}

