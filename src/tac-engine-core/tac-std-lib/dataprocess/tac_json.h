// This file implements the JavaScript Object Notation,
// which is a file format for de/serializing data

#pragma once

//#include "tac-std-lib/tac_core.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_map.h"

namespace Tac
{
  struct Errors;
  using JsonNumber = double;

	const bool JsonParseDebug = false;

	enum class JsonType
	{
		String,
		Number,
		Object,
		Array,
		Bool,
		Null
	};

	struct Indentation
	{
		int    spacesPerTab = 2;
		bool   convertTabsToSpaces = true;
	};

	// The function names Stringify and Parse mimic the built-in javascript api
	struct Json
	{
		// Assign common types at time of level_editor
		Json( String );
		Json( const char* );
		Json( StringView );
		Json( JsonNumber );
		Json( bool );
		Json();
    Json( Json&& ) noexcept;
    Json( const Json& );
		~Json();

		// Setters
		void                      Clear();
		void                      Parse( const char* bytes, int byteCount, Errors& );
		void                      Parse( StringView, Errors& );
		void                      DeepCopy( const Json* );
		void                      TakeOver( Json&& ) noexcept;
		void                      SetNull();
		void                      SetNumber( JsonNumber );
		void                      SetBool( bool );
		void                      SetString( StringView );
		void                      operator = ( const Json& );
		void                      operator = ( const Json* );
		void                      operator = ( StringView );
		void                      operator = ( JsonNumber );
		void                      operator = ( bool );
		void                      operator = ( Json&& ) noexcept;

		// Getters
		Json*                     AddChild();
    void                      AddChild( const Json& );

    // could just use getchild(key) everywhere, but that makes the api stupid/inconsistant when
    // you can use addchild for arrays but not objects
		Json*                     AddChild( StringView );
		Json&                     GetChild( StringView );
		Json*                     FindChild( StringView ) const;
		bool                      HasChild( StringView );
		String                    Stringify( const Indentation* = nullptr, int tabCount = 0 ) const;
		Json&                     operator[]( StringView );
		Json&                     operator[]( const char* );
		operator String();
		operator JsonNumber();
		operator bool();

		Map< String, Json* >      mObjectChildrenMap;
		String                    mString;
		JsonNumber                mNumber = 0;
		Vector< Json* >           mArrayElements;
		bool                      mBoolean = false;

    //                        Some functions want to convert the type of a json node.
    //                        If the type is null, then no data will be lost due during the conversion.
    //                        This assumption could not be made if mType defauts to JsonType::Object
		JsonType                  mType = JsonType::Null;
	};

} // namespace Tac

