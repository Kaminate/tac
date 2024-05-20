#include "tac_settings_node.h" // self-inc
#include "tac_settings_root.h"

//#include "tac-engine-core/shell/tac_shell_timestep.h"
//#include "tac-std-lib/algorithm/tac_algorithm.h"
//#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-std-lib/filesystem/tac_filesystem.h"
//#include "tac-std-lib/math/tac_math.h"
//#include "tac-std-lib/memory/tac_memory.h"
//#include "tac-std-lib/os/tac_os.h"
//#include "tac-std-lib/string/tac_string_util.h"


namespace Tac
{

  SettingsNode::SettingsNode( SettingsRoot* root, Json* json ) : mRoot( root ), mJson( json ) {}

  void         SettingsNode::SetValue( Json val )
  {
    *mJson = val;
    mRoot->SetDirty();
  }

  Json&        SettingsNode::GetValue()
  {
    return *mJson;
  }

  bool          SettingsNode::IsValid() const
  {
    return mRoot;
  }

  Json&        SettingsNode::GetValueWithFallback( Json fallback )
  {
    if( mJson->mType == JsonType::Null )
      SetValue( fallback );

    return *mJson;
  }

  SettingsNode SettingsNode::GetChild( StringView path )
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
        TAC_ASSERT( ( unsigned )iElement < ( unsigned )root->mArrayElements.size() );
        root = root->mArrayElements[ iElement ];
        path.remove_prefix( iCloseSquareBracket );
      }

      TAC_ASSERT( oldRoot != root );
      TAC_ASSERT( StrCmp( oldPath, path ) );
    }
    TAC_ASSERT( root != mJson );

    return SettingsNode( mRoot, root );
  }


}

