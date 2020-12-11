// This file implements the JavaScript Object Notation,
// which is a file format for de/serializing data

#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"

#include <map>

namespace Tac
{
	struct Errors;
	typedef double JsonNumber;

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
		// Assign common types at time of creation
		Json( StringView );
		Json( JsonNumber );
		Json( bool );
		Json();
		Json( const Json& ) = delete;
		~Json();

		// Setters
		void                      Clear();
		void                      Parse( const char* bytes, int byteCount, Errors& );
		void                      Parse( StringView, Errors& );
		void                      DeepCopy( const Json* );
		void                      SetNull();
		void                      SetNumber( JsonNumber );
		void                      SetBool( bool );
		void                      SetString( StringView );
		void                      operator = ( const Json& );
		void                      operator = ( const Json* );
		void                      operator = ( StringView );
		void                      operator = ( JsonNumber );
		void                      operator = ( bool );

		// Getters
		Json*                     AddChild();
		Json*                     AddChild( StringView );
		Json&                     GetChild( StringView );
		String                    Stringify( const Indentation* = nullptr, int tabCount = 0 ) const;
		Json&                     operator[]( StringView );
		Json&                     operator[]( const char* );
		operator String();
		operator JsonNumber();
		operator bool();

		std::map< String, Json* > mChildren;
		String                    mString;
		JsonNumber                mNumber = 0;
		Vector< Json* >           mElements;
		bool                      mBoolean = false;
		JsonType                  mType = JsonType::Null; // JsonType::Object;
	};

}
