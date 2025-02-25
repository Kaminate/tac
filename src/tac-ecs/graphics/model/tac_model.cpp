#include "tac_model.h" // self-inc

#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/model/tac_model_debug.h"
#include "tac-engine-core/asset/tac_asset_hash_cache.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/string/tac_string_meta.h"

namespace Tac
{
  //static int sRegistryIndex;

  static ComponentInfo* sEntry;

  TAC_META_REGISTER_COMPOSITE_BEGIN( Model );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mModelPath );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mModelIndex );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mIsStatic );
  TAC_META_REGISTER_COMPOSITE_END( Model );

	static Component* CreateModelComponent( World* world )
	{
    Graphics* graphics{ Graphics::From( world ) };
		return graphics->CreateModelComponent();
	}

	static void       DestroyModelComponent( World* world, Component* component )
	{
    Graphics* graphics{ Graphics::From( world ) };
    graphics->DestroyModelComponent( ( Model* )component );
	}

#if 0
	static void       SaveModelComponent( Json& modelJson, Component* component )
	{
		auto model { ( Model* )component };
		//Json colorRGBJson;
		//colorRGBJson[ "r" ].SetNumber( model->mColorRGB[ 0 ] );
		//colorRGBJson[ "g" ].SetNumber( model->mColorRGB[ 1 ] );
		//colorRGBJson[ "b" ].SetNumber( model->mColorRGB[ 2 ] );
		modelJson[ "mModelPath" ].SetString( model->mModelPath.c_str() );
    modelJson[ "mModelIndex" ].SetNumber( model->mModelIndex );
    //modelJson[ "mTryingNewThing" ].SetNumber( model->mTryingNewThing );
		//modelJson[ "mColorRGB" ] = colorRGBJson;
	}

	static void       LoadModelComponent( Json& modelJson, Component* component )
	{
		auto model { ( Model* )component };
		//Json& colorRGBJson { modelJson[ "mColorRGB" ] };
    model->mModelIndex = ( int )modelJson[ "mModelIndex" ].mNumber;
		model->mModelPath = modelJson[ "mModelPath" ].mString;
		//model->mColorRGB.x = ( float )colorRGBJson[ "r" ].mNumber;
		//model->mColorRGB.y = ( float )colorRGBJson[ "g" ].mNumber;
		//model->mColorRGB.z = ( float )colorRGBJson[ "b" ].mNumber;
	}
#endif

#if 0
  struct AssetPathNetWriter : public NetVarWriter
  {
    void Write( Writer* writer, const void* src )
    {
      const AssetPathString* assetPath{ ( const AssetPathString* )src };
      const AssetHash assetHash( *assetPath );
      writer->Write( assetHash ); 
    }
  };

  struct AssetPathNetReader : public NetVarReader
  {
    bool Read( Reader* reader, void* dst ) override
    {
      AssetHash assetHash;
      if( !reader->Read( &assetHash ) )
        return false;

      *( AssetPathString* )dst = AssetHashCache::GetPathFromHash( assetHash );
      return true;
    }
  };

  static AssetPathNetWriter sAssetPathNetWriter;
  static AssetPathNetReader sAssetPathNetReader;
#endif



  // -----------------------------------------------------------------------------------------------

	const Model*          Model::GetModel( const Entity* entity )
	{
		return ( Model* )entity->GetComponent( sEntry );
	}

	dynmc Model*          Model::GetModel( dynmc Entity* entity )
	{
		return ( Model* )entity->GetComponent( sEntry );
	}

	const ComponentInfo*  Model::GetEntry() const
	{
    return sEntry;
	}

	void                  Model::RegisterComponent()
	{
    //const NetVar netColor
    //{
    //  .mDebugName           { "mColorRGB" },
    //  .mByteOffset          { TAC_OFFSET_OF( Model, mColorRGB ) },
    //  .mElementByteCount    { sizeof( float ) },
    //  .mElementCount        { 3 },
    //  .mIsTriviallyCopyable { true },
    //};

#if 0

    const NetVar netPath
    {
      .mDebugName  { "mModelPath" },
      .mByteOffset { TAC_OFFSET_OF( Model, mModelPath ) },
      .mVarReader  { &sAssetPathNetReader },
      .mVarWriter  { &sAssetPathNetWriter },
    };

    const NetVar netIndex
    {
      .mDebugName           { "mModelIndex" },
      .mByteOffset          { TAC_OFFSET_OF( Model, mModelIndex ) },
      .mElementByteCount    { sizeof( Model::mModelIndex ) },
      .mElementCount        { 1 },
      .mIsTriviallyCopyable { true },
    };

    NetVars netBits;
    //netBits.Add( netColor );
    netBits.Add( netPath );
    netBits.Add( netIndex );
#endif

    const MetaCompositeType* metaType{ ( MetaCompositeType* )&GetMetaType< Model >() };

    NetMembers netMembers{};
    netMembers.SetAll();

    const NetVarRegistration netVarRegistration
    {
      .mNetMembers { netMembers },
      .mMetaType   { metaType },
    };

    * ( sEntry = ComponentInfo::Register() ) = ComponentInfo
    {
      .mName               { "Model" },
      .mCreateFn           { CreateModelComponent },
      .mDestroyFn          { DestroyModelComponent },
      .mDebugImguiFn       { ModelDebugImgui },
      .mNetVarRegistration { netVarRegistration },
      .mMetaType           { metaType },
      //.mSaveFn       { SaveModelComponent },
      //.mLoadFn       { LoadModelComponent },
      //.mNetVars      { netBits },
    };
	}

} // namespace Tac

