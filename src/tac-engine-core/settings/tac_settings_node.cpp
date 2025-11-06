#include "tac_settings_node.h" // self-inc

#include "tac-engine-core/settings/tac_settings_root.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac
{
  SettingsNode::SettingsNode( SettingsRoot* root, Json* json ) : mRoot( root ), mJson( json ) {}
 
  void SettingsNode::SetValue( Json val )
  {
    *mJson = val;
    mRoot->SetDirty();
  }

  auto SettingsNode::GetValue() -> Json& { return *mJson; }

  bool SettingsNode::IsValid() const { return mRoot; }

  auto SettingsNode::GetValueWithFallback( Json fallback ) -> Json&
  {
    if( mJson->mType != fallback.mType )
      SetValue( fallback );
    return *mJson;
  }

  auto SettingsNode::GetChild( StringView path ) -> SettingsNode
  {
    Json* root{ mJson };
    while( !path.empty() )
    {
      // Non-leaf nodes are either JsonType::Objects/Arrays
      StringView oldPath{ path };
      Json* oldRoot{ root };
      if( path.front() == '.' )
        path.remove_prefix( 1 );

      if( IsAlpha( path.front() ) )
      {
        root->mType = root->mType == JsonType::Null ? JsonType::Object : root->mType;
        TAC_ASSERT( root->mType == JsonType::Object );
        const char* keyEnd{ path.data() };
        while( keyEnd < path.end() &&
               ( IsSpace( *keyEnd ) ||
               IsAlpha( *keyEnd ) ||
               IsDigit( *keyEnd ) ||
               *keyEnd == '_' ) )
          keyEnd++;
        const StringView key( path.data(), keyEnd );
        path.remove_prefix( key.size() );
        root = &root->GetChild( key );
      }
      else if( path.front() == '[' )
      {
        root->mType = root->mType == JsonType::Null ? JsonType::Array : root->mType;
        TAC_ASSERT( root->mType == JsonType::Array );
        const int iCloseSquareBracket{ path.find_first_of( "]" ) };
        TAC_ASSERT( iCloseSquareBracket != StringView::npos );
        const int iElement{ Atoi( StringView( path.data() + 1, path.data() + iCloseSquareBracket ) ) };
        if( iElement >= root->mArrayElements.size() )
        {
          int elementsNeeded{ iElement + 1 - root->mArrayElements.size() };
          while( elementsNeeded-- )
            root->AddChild();
        }

        root = root->mArrayElements[ iElement ];
        path.remove_prefix( iCloseSquareBracket + 1 );
      }

      TAC_ASSERT( oldRoot != root );
      TAC_ASSERT( StrCmp( oldPath, path ) );
    }
    TAC_ASSERT( root != mJson );
    return SettingsNode( mRoot, root );
  }

  auto SettingsNode::GetChildrenArray() -> Span< SettingsNode >
  {
    mJson->mType = JsonType::Array;
    const int n{ mJson->mArrayElements.size() };
    SettingsNode* data{ ( SettingsNode* )FrameMemoryAllocate( n * sizeof( SettingsNode ) ) };
    for( int i{}; i < n; ++i )
      data[ i ] = SettingsNode( mRoot, mJson->mArrayElements[ i ] );
    return Span< SettingsNode >{ data, n };
  }

}

