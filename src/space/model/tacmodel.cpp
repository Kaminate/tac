
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


	ComponentRegistryEntry* Model::ModelComponentRegistryEntry;

	const Model* Model::GetModel( const Entity* entity )
	{
		return ( Model* )entity->GetComponent( Model::ModelComponentRegistryEntry );
	}

	Model* Model::GetModel( Entity* entity )
	{
		return ( Model* )entity->GetComponent( Model::ModelComponentRegistryEntry );
	}

	ComponentRegistryEntry* Model::GetEntry()
	{
		return Model::ModelComponentRegistryEntry;
	}

	static Component* CreateModelComponent( World* world )
	{
		return Graphics::GetSystem( world )->CreateModelComponent();
	}

	static void DestroyModelComponent( World* world, Component* component )
	{
		Graphics::GetSystem( world )->DestroyModelComponent( ( Model* )component );
	}

	static void SaveModelComponent( Json& modelJson, Component* component )
	{
		auto model = ( Model* )component;
		Json colorRGBJson;
		colorRGBJson[ "r" ].SetNumber( model->mColorRGB[ 0 ] );
		colorRGBJson[ "g" ].SetNumber( model->mColorRGB[ 1 ] );
		colorRGBJson[ "b" ].SetNumber( model->mColorRGB[ 2 ] );
		modelJson[ "mGLTFPath" ].SetString( model->mGLTFPath );
		modelJson[ "mColorRGB" ].DeepCopy( &colorRGBJson );
	}

	static void LoadModelComponent( Json& modelJson, Component* component )
	{
		auto model = ( Model* )component;
		model->mGLTFPath = modelJson[ "mGLTFPath" ].mString;
		model->mColorRGB = {
			( float )modelJson[ "mColorRGB" ][ "r" ].mNumber,
			( float )modelJson[ "mColorRGB" ][ "g" ].mNumber,
			( float )modelJson[ "mColorRGB" ][ "b" ].mNumber };
	}


	void ModelDebugImgui( Component* );
	void Model::SpaceInitGraphicsModel()
	{
    Model::ModelComponentRegistryEntry = ComponentRegistry_RegisterComponent();
		Model::ModelComponentRegistryEntry->mName = "Model";
		Model::ModelComponentRegistryEntry->mNetworkBits = ComponentModelBits;
		Model::ModelComponentRegistryEntry->mCreateFn = CreateModelComponent;
		Model::ModelComponentRegistryEntry->mDestroyFn = DestroyModelComponent;
		Model::ModelComponentRegistryEntry->mDebugImguiFn = ModelDebugImgui;
		Model::ModelComponentRegistryEntry->mSaveFn = SaveModelComponent;
		Model::ModelComponentRegistryEntry->mLoadFn = LoadModelComponent;
	}

}

