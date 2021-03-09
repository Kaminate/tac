#include "src/space/tacComponent.h"
#include "src/space/tacEntity.h"
#include "src/space/tacSystem.h"
#include "src/space/tacWorld.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacJson.h"
#include "src/common/tacPreprocessor.h"
#include <algorithm>

#pragma warning( disable: 6011)

namespace Tac
{


	Entity::~Entity()
	{
		RemoveAllComponents();
		// Assert that the world no longer contains this entity?
	}

	void Entity::RemoveAllComponents()
	{
		for( Component* component : mComponents )
		{
			ComponentRegistryEntry* entry = component->GetEntry();
			entry->mDestroyFn( mWorld, component );
		}
		mComponents.clear();
	}

	Component* Entity::AddNewComponent( const ComponentRegistryEntry* entry )
	{
		TAC_ASSERT( entry );
		TAC_ASSERT( !HasComponent( entry ) );
		TAC_ASSERT( entry->mCreateFn );
		Component* component = entry->mCreateFn( mWorld );
		TAC_ASSERT( component );
		mComponents.push_back( component );
		component->mEntity = this;
		return component;
	}

	Component* Entity::GetComponent( const ComponentRegistryEntry* entry )
	{
		for( Component* component : mComponents )
			if( component->GetEntry() == entry )
				return component;
		return nullptr;
	}

	const Component* Entity::GetComponent( const ComponentRegistryEntry* entry ) const
	{
		for( auto component : mComponents )
			if( component->GetEntry() == entry )
				return component;
		return nullptr;
	}

	bool Entity::HasComponent( const ComponentRegistryEntry* entry )
	{
		return GetComponent( entry ) != nullptr;
	}

	void Entity::RemoveComponent( const ComponentRegistryEntry* entry )
	{
		TAC_ASSERT_UNIMPLEMENTED;
		auto it = std::find_if(
			mComponents.begin(),
			mComponents.end(),
			[ & ]( Component* component ) { return component->GetEntry() == entry; } );
		TAC_ASSERT( it != mComponents.end() );
		Component* component = *it;
		mComponents.erase( it );
		//auto componentData = GetComponentData( componentType );
		//auto system = mWorld->GetSystem( componentData->mSystemType );
		//// todo:
		////   break this up  into a thing
		////   that auto does it when you register each system
		//auto graphics = ( Graphics* )mWorld->GetSystem( SystemType::Graphics );
		//switch( componentType )
		//{
		//  case ComponentRegistryEntryIndex::Say:
		//    break;
		//  case ComponentRegistryEntryIndex::Model:
		//    graphics->DestroyModelComponent( ( Model* )component );
		//    break;
		//  case ComponentRegistryEntryIndex::Collider:
		//    break;
		//  case ComponentRegistryEntryIndex::Terrain:
		//    break;
		//    TAC_ASSERT_INVALID_DEFAULT_CASE( componentType );
		//}

	}


	void Entity::DeepCopy( const Entity& entity )
	{
		//TAC_ASSERT( mWorld && entity.mWorld && mWorld != entity.mWorld );
		mEntityUUID = entity.mEntityUUID;
		RemoveAllComponents();
		//for( auto oldComponent : entity.mComponents )
		//{
		//  auto componentType = oldComponent->GetComponentType();
		//  auto newComponent = AddNewComponent( componentType );
		//  auto componentData = GetComponentData( componentType );
		//  for( auto& networkBit : componentData->mNetworkBits )
		//  {
		//    auto dst = ( char* )newComponent + networkBit.mByteOffset;
		//    auto src = ( char* )oldComponent + networkBit.mByteOffset;
		//    auto size = networkBit.mComponentByteCount * networkBit.mComponentCount;
		//    std::memcpy( dst, src, size );
		//  }
		//}
	}

	void Entity::AddChild( Entity* child )
	{
		TAC_ASSERT( !Contains( mChildren, child ) );
		TAC_ASSERT( child->mParent != this );
		child->Unparent();
		child->mParent = this;
		mChildren.push_back( child );
	}

	void Entity::DebugImgui()
	{
#if 0
		ImGui::PushID( this );
		OnDestruct( ImGui::PopID() );
		if( !ImGui::CollapsingHeader( va( "Entity id %i", mEntityUUID ), ImGuiTreeNodeFlags_DefaultOpen ) )
			return;
		ImGui::Indent();
		OnDestruct( ImGui::Unindent() );
		ImGui::DragFloat3( "Position", mPosition.data(), 0.1f );

		Vector< ComponentRegistryEntryIndex > couldBeAdded;
		for( int i = 0; i < ( int )ComponentRegistryEntryIndex::Count; i++ )
		{
			auto componentType = ( ComponentRegistryEntryIndex )i;
			if( HasComponent( componentType ) )
				continue;
			auto componentData = GetComponentData( componentType );
			if( !componentData )
				continue;
			couldBeAdded.push_back( componentType );
		}
		if( !couldBeAdded.empty() && ImGui::BeginMenu( "Add Component" ) )
		{
			for( auto componentType : couldBeAdded )
			{
				auto componentData = GetComponentData( componentType );
				if( !ImGui::MenuItem( componentData->mName ) )
					continue;
				AddNewComponent( componentType );
				break;
			}
			ImGui::EndMenu();
		}

		if( !mComponents.empty() && ImGui::BeginMenu( "Remove Component" ) )
		{
			for( auto component : mComponents )
			{
				auto componentType = component->GetComponentType();
				auto componentData = GetComponentData( componentType );
				if( !ImGui::MenuItem( componentData->mName ) )
					continue;
				RemoveComponent( componentType );
				break;
			}
			ImGui::EndMenu();
		}
		for( auto component : mComponents )
			component->DebugImgui();
#endif
	}

	void Entity::Unparent()
	{
		if( !mParent )
			return;
		for( int iChild = 0; iChild < mParent->mChildren.size(); ++iChild )
		{
			if( mParent->mChildren[ iChild ] == this )
			{
				mParent->mChildren[ iChild ] = mParent->mChildren[ mParent->mChildren.size() - 1 ];
				mParent->mChildren.pop_back();
				break;
			}
		}
		mParent = nullptr;
		// todo: relative positioning
	}

	static Json* Vector3ToJson( v3 v )
	{
		static Json json;
		json.GetChild( "x" ).SetNumber( v.x );
		json.GetChild( "y" ).SetNumber( v.y );
		json.GetChild( "z" ).SetNumber( v.z );
		return &json;
	}

	void Entity::Save( Json& entityJson )
	{
		Entity* entity = this;
		entityJson[ "mPosition" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mPosition ) );
		entityJson[ "mScale" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mScale ) );
		entityJson[ "mName" ].SetString( entity->mName );
		entityJson[ "mEulerRads" ].DeepCopy( Vector3ToJson( entity->mRelativeSpace.mEulerRads ) );
		entityJson[ "mEntityUUID" ].SetNumber( ( JsonNumber )entity->mEntityUUID );

		for( Component* component : entity->mComponents )
		{
			auto entry = component->GetEntry();
			Json componentJson;
			if( entry->mSaveFn )
				entry->mSaveFn( componentJson, component );
			entityJson[ StringView( entry->mName ) ].DeepCopy( &componentJson );
		}

		if( !entity->mChildren.empty() )
		{
			Json& childrenJson = entityJson[ "mChildren" ];
			for( Entity* childEntity : entity->mChildren )
				childEntity->Save( *childrenJson.AddChild() );
		}
	}

	static v3 Vector3FromJson( Json& json )
	{
		v3 v =
		{
			( float )json[ "x" ].mNumber,
			( float )json[ "y" ].mNumber,
			( float )json[ "z" ].mNumber,
		};
		return v;
	}

	void Entity::Load( Json& prefabJson )
	{
		Entity* entity = this;
		entity->mRelativeSpace.mPosition = Vector3FromJson( prefabJson[ "mPosition" ] );
		entity->mRelativeSpace.mScale = Vector3FromJson( prefabJson[ "mScale" ] );
		entity->mRelativeSpace.mEulerRads = Vector3FromJson( prefabJson[ "mEulerRads" ] );
		entity->mName = prefabJson[ "mName" ].mString;
		entity->mEntityUUID = ( EntityUUID )( UUID )prefabJson[ "mEntityUUID" ].mNumber;

		// I think these should have its own mComponents json node
		for( auto& prefabJson : prefabJson.mObjectChildrenMap )
		{
			StringView key = prefabJson.first;
			Json* componentJson = prefabJson.second;
      ComponentRegistryEntry* componentRegistryEntry = ComponentRegistry_FindComponentByName( key );
			if( !componentRegistryEntry )
				continue; // This key-value pair is not a component

			TAC_ASSERT( componentRegistryEntry );
			Component* component = entity->AddNewComponent( componentRegistryEntry );
			if( componentRegistryEntry->mLoadFn )
				componentRegistryEntry->mLoadFn( *componentJson, component );
		}

		if( Json* childrenJson = prefabJson.mObjectChildrenMap[ "mChildren" ] )
		{
			for( Json* childJson : childrenJson->mArrayElements )
			{
				Entity* childEntity = mWorld->SpawnEntity( NullEntityUUID );
				childEntity->Load( *childJson );

				entity->AddChild( childEntity );
			}
		}
	}

}

