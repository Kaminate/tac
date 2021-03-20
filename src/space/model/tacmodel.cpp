
#include "src/space/model/tacModel.h"
#include "src/space/tacEntity.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/tacOS.h"
#include "src/common/tacUtility.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacJson.h"

namespace Tac
{
  static ComponentRegistryEntry* sComponentRegistryEntry;

	const Model* Model::GetModel( const Entity* entity )
	{
		return ( Model* )entity->GetComponent( sComponentRegistryEntry );
	}

	Model* Model::GetModel( Entity* entity )
	{
		return ( Model* )entity->GetComponent( sComponentRegistryEntry );
	}

	const ComponentRegistryEntry* Model::GetEntry() const
	{
    return sComponentRegistryEntry;
	}

  //const ComponentRegistryEntry* Model::GetEntryStatic()
  //{
  //  return sComponentRegistryEntry;
  //}

	static Component* CreateModelComponent( World* world )
	{
		return GetGraphics( world )->CreateModelComponent();
	}

	static void DestroyModelComponent( World* world, Component* component )
	{
		GetGraphics( world )->DestroyModelComponent( ( Model* )component );
	}

	static void SaveModelComponent( Json& modelJson, Component* component )
	{
		auto model = ( Model* )component;
		Json colorRGBJson;
		colorRGBJson[ "r" ].SetNumber( model->mColorRGB[ 0 ] );
		colorRGBJson[ "g" ].SetNumber( model->mColorRGB[ 1 ] );
		colorRGBJson[ "b" ].SetNumber( model->mColorRGB[ 2 ] );
		modelJson[ "mModelPath" ].SetString( model->mModelPath );
    modelJson[ "mModelIndex" ].SetNumber( model->mModelIndex );
    modelJson[ "mTryingNewThing" ].SetNumber( model->mTryingNewThing );
		modelJson[ "mColorRGB" ].DeepCopy( &colorRGBJson );
	}

	static void LoadModelComponent( Json& modelJson, Component* component )
	{
		auto model = ( Model* )component;
		Json& colorRGBJson = modelJson[ "mColorRGB" ];
    model->mModelIndex = ( int )modelJson[ "mModelIndex" ].mNumber;
    model->mTryingNewThing = ( int )modelJson[ "mTryingNewThing" ].mNumber;
		model->mModelPath = modelJson[ "mModelPath" ].mString;
		model->mColorRGB = {
			( float )colorRGBJson[ "r" ].mNumber,
			( float )colorRGBJson[ "g" ].mNumber,
			( float )colorRGBJson[ "b" ].mNumber };
	}


	void ModelDebugImgui( Model* );
	void RegisterModelComponent()
	{
    sComponentRegistryEntry = ComponentRegistry_RegisterComponent();
		sComponentRegistryEntry->mName = "Model";
		//sComponentRegistryEntry->mNetworkBits = ComponentModelBits;
		sComponentRegistryEntry->mCreateFn = CreateModelComponent;
		sComponentRegistryEntry->mDestroyFn = DestroyModelComponent;
    sComponentRegistryEntry->mDebugImguiFn = []( Component* component ){ ModelDebugImgui( ( Model* )component ); };
		sComponentRegistryEntry->mSaveFn = SaveModelComponent;
		sComponentRegistryEntry->mLoadFn = LoadModelComponent;
	}

}

