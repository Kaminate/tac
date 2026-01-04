// This file implements the JavaScript Object Notation,
// which is a file format for de/serializing data
//
// TODO: Add escape sequences: \" \\ \/ \b \f \n \r \t \u

#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_map.h"

namespace Tac
{
  struct Errors;
  using JsonNumber = double;

  const bool JsonParseDebug {};

	enum class JsonType
	{
		Null = 0,
		String,
		Number,
		Object,
		Array,
		Bool,
	};

	struct Indentation
	{
    int    mSpacesPerTab        { 2 };
    bool   mConvertTabsToSpaces { true };
	};

	// The function names Stringify and Parse mimic the built-in javascript api
  struct [[nodiscard]] Json
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

    static Json Parse( StringView, Errors& );
		static Json Parse( const char* bytes, int byteCount, Errors& );

		// Setters
		void Clear();
		void DeepCopy( const Json* );
		void TakeOver( Json&& ) noexcept;
		void SetNull();
		void SetNumber( JsonNumber );
		void SetBool( bool );
		void SetString( StringView );
		auto operator = ( const Json& ) -> Json&;
		auto operator = ( const Json* ) -> Json&;
		auto operator = ( StringView ) -> Json&;
		auto operator = ( JsonNumber ) -> Json&;
		auto operator = ( int ) -> Json&;
		auto operator = ( bool ) -> Json&;
		auto operator = ( Json&& ) noexcept -> Json&;

		// Getters
		auto AddChild() -> Json*;
    void AddChild( const Json& );

    // could just use getchild(key) everywhere, but that makes the api stupid/inconsistant when
    // you can use addchild for arrays but not objects
		auto AddChild( StringView ) -> Json*;
		auto GetChild( StringView ) -> Json&;
		auto FindChild( StringView ) const -> Json*;
		auto HasChild( StringView ) -> bool;
		auto Stringify( const Indentation* = nullptr, int tabCount = 0 ) const -> String;
		auto operator[]( StringView ) -> Json&;
		auto operator[]( const char* ) -> Json&;
		auto operator[]( int ) -> Json&;
		operator String();
		operator JsonNumber();
		operator bool();

    Map< String, Json* >      mObjectChildrenMap {};
    String                    mString            {};
		JsonNumber                mNumber            {};
    Vector< Json* >           mArrayElements     {};
		bool                      mBoolean           {};

    //                        Some functions want to convert the type of a json node.
    //                        If the type is null, then no data will be lost due during the conversion.
    //                        This assumption could not be made if mType defauts to JsonType::Object
		JsonType                  mType              { JsonType::Null };
	};

} // namespace Tac

