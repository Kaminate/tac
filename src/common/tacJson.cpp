#include "src/common/tacJson.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacMemory.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacTextParser.h"

namespace Tac
{
	static String DoubleQuote( StringView s )
	{
		String quote = String( 1, '\"' );
		return quote + s + quote;
	}

	static void ExpectCharacter( char c, char expected, Errors& errors )
	{
		if( c == expected )
			return;
		const String errorMsg = "Unexpected character " + String( 1, c ) + ", expected " + String( 1, expected );
		TAC_RAISE_ERROR( errorMsg, errors );
	}

	static void ParseObject( Json* json, ParseData* parseData, Errors& errors );

	static void ParseArray( Json* json, ParseData* parseData, Errors& errors );

	static void ParseQuotedString( StringView* stringView, ParseData* parseData, Errors& errors )
	{
		if( parseData->GetRemainingByteCount() < 2 )
			TAC_RAISE_ERROR( "not enough space for quoted string", errors );
		const char* quotedStrBegin = parseData->GetPos();
		if( *quotedStrBegin != '\"' )
			TAC_RAISE_ERROR( "string does not begin with quote", errors );
		parseData->EatByte();
		if( !parseData->EatUntilCharIsPrev( '\"' ) )
			TAC_RAISE_ERROR( "string does not end with quote", errors );
		const char* quotedStrEnd = parseData->GetPos();
		*stringView = StringView( quotedStrBegin + 1, quotedStrEnd - 1 );
	}

	static void ParseUnknownType( Json* json, ParseData* parseData, Errors& errors )
	{
		json->Clear();
		parseData->EatWhitespace();
		const char* b = parseData->PeekByte();
    if( !b )
      return;
		//TAC_HANDLE_ERROR_IF( !b, "Failed to parse unknown type", errors );
		const char c = *b;
		if( c == '{' )
		{
			ParseObject( json, parseData, errors );
		}
		else if( c == '[' )
		{
			ParseArray( json, parseData, errors );
		}
		else if( c == '\"' )
		{
			StringView s;
			ParseQuotedString( &s, parseData, errors );
			TAC_HANDLE_ERROR( errors );
			json->SetString( s );
		}
		else if( isdigit( c ) || c == '-' || c == '.' )
		{
			auto f = parseData->EatFloat();
			TAC_HANDLE_ERROR_IF( !f.HasValue(), "Failed to parse number", errors );
			json->SetNumber( ( JsonNumber )f.GetValue() );
		}
		else if( c == 'n' )
		{
			if( !parseData->EatStringExpected( "null" ) )
				TAC_RAISE_ERROR( "Failed parsing null", errors );
			json->mType = JsonType::Null;
		}
		else if( c == 't' )
		{
			const StringView str = "true";
			if( !parseData->EatStringExpected( str ) )
				TAC_RAISE_ERROR( "encountered t but no true", errors );
			json->SetBool( true );
		}
		else if( c == 'f' )
		{
			const StringView str = "false";
			if( !parseData->EatStringExpected( str ) )
				TAC_RAISE_ERROR( "encountered f but no false", errors );
			json->SetBool( false );
		}
		else if( c == '/' )
		{
			parseData->EatRestOfLine();
			ParseUnknownType( json, parseData, errors );
		}
		else
		{
			const String errorMsg = String( "Unexpected character: " ) + c;
			TAC_RAISE_ERROR( errorMsg, errors );
		}
	}

	static void ParseObject( Json* json, ParseData* parseData, Errors& errors )
	{
		json->mType = JsonType::Object;
		if( parseData->EatWord() != "{" )
			TAC_RAISE_ERROR( "Expected {", errors );

		for( ;; )
		{
			parseData->EatWhitespace();
			if( parseData->EatStringExpected( "}" ) )
				break;
			if( parseData->EatStringExpected( "," ) )
				continue;
			if( parseData->EatStringExpected( "//" ) )
			{
				parseData->EatRestOfLine();
				continue;
			}

			StringView key;
			ParseQuotedString( &key, parseData, errors );
			TAC_HANDLE_ERROR( errors );
			TAC_HANDLE_ERROR_IF( key.empty(), "object key cannot be empty", errors );
			if( !parseData->EatUntilCharIsPrev( ':' ) )
				TAC_RAISE_ERROR( "Missing : after json key", errors );
			ParseUnknownType( json->AddChild( key ), parseData, errors );
			TAC_HANDLE_ERROR( errors );
		}
	}

	static void ParseArray( Json* json, ParseData* parseData, Errors& errors )
	{
		json->mType = JsonType::Array;
		parseData->EatWhitespace();
		if( parseData->EatWord() != "[" )
			TAC_RAISE_ERROR( "Expected [", errors );
		for( ;; )
		{
			parseData->EatWhitespace();
			if( parseData->EatStringExpected( "]" ) )
				break;

			if( parseData->EatStringExpected( "," ) )
				continue;

			// Handle comments
			if( parseData->EatStringExpected( "//" ) )
			{
				parseData->EatRestOfLine();
				continue;
			}

			ParseUnknownType( json->AddChild(), parseData, errors );
			TAC_HANDLE_ERROR( errors );
		}
	}

	static String Tab( const Indentation* indentation, int tabCount )
	{
		const Indentation defaultIndentation;
		indentation = indentation ? indentation : &defaultIndentation;
		String tab = indentation->convertTabsToSpaces ? String( indentation->spacesPerTab, ' ' ) : "\t";
		String result;
		 for( int i = 0; i < tabCount; ++i )
			 result += tab;
		 return result;
	}

	Json::Json( StringView v ){ SetString( v ); }
	Json::Json( JsonNumber v ){ SetNumber( v ); }
	Json::Json( bool v ){ SetBool( v ); }
	Json::Json(){ SetNull(); }
	Json::~Json() { Clear(); }
	Json::operator String() { return mString; }
	Json::operator JsonNumber() { return mNumber; }
	Json::operator bool() { return mBoolean; }
	void                      Json::SetNull() { mType = JsonType::Null; }
	void                      Json::SetNumber( JsonNumber number ){ mType = JsonType::Number; mNumber = number; }
	void                      Json::SetString( StringView str ) { mType = JsonType::String; mString = str; }
	void                      Json::SetBool( bool b ) { mType = JsonType::Bool; mBoolean = b; }
	void                      Json::Clear()
	{
		for( auto pair : mChildren )
			delete pair.second;
		mChildren.clear();

		for( auto element : mElements )
			delete element;
		mElements.clear();
	}
	String                    Json::Stringify( const Indentation* indentation, int tabCount ) const
	{
		int iChild = 0;
		String result;
		auto GetSeparator = [ & ]( int childCount ) { return iChild++ != childCount - 1 ? "," : ""; };
		switch( mType )
		{
		case JsonType::String:
		{
			result = DoubleQuote( mString );
		} break;
		case JsonType::Number:
		{
			if( ( ( JsonNumber )( ( int )mNumber ) ) == mNumber )
				result = ToString( ( int )mNumber );
			else
				result = ToString( mNumber );
		} break;
		case JsonType::Null:
		{
			result = "null";
		} break;
		case JsonType::Bool:
		{
			result = mBoolean ? "true" : "false";
		} break;
		case JsonType::Object:
		{
			result += Tab( indentation, tabCount ) + "{\n";
			tabCount++;
			for( auto pair : mChildren )
			{
				String childKey = pair.first;
				Json* childValue = pair.second;

				result += Tab( indentation, tabCount ) + DoubleQuote( childKey ) + ":";
				result += Contains( { JsonType::Array, JsonType::Object }, childValue->mType ) ? "\n" : " ";
				result += childValue->Stringify( indentation, tabCount );
				result += GetSeparator( ( int )mChildren.size() );
				result += "\n";
			}
			tabCount--;
			result += Tab( indentation, tabCount ) + "}";
		} break;
		case JsonType::Array:
		{
			result += Tab( indentation, tabCount ) + "[\n";
			tabCount++;
			for( Json* element : mElements )
			{
				if( !Contains( { JsonType::Array, JsonType::Object }, element->mType ) )
					result += Tab( indentation, tabCount );
				result +=
					element->Stringify( indentation, tabCount ) +
					GetSeparator( ( int )mElements.size() ) +
					"\n";
			}
			tabCount--;
			result += Tab( indentation, tabCount ) + "]";
		} break;
		}
		return result;
	}

	void                      Json::Parse( const char* bytes, int byteCount, Errors& errors )
	{
		ParseData parseData( bytes, byteCount );
		ParseUnknownType( this, &parseData, errors );
	}
	void                      Json::Parse( StringView s, Errors& errors )
	{
		Parse( s.data(), s.size(), errors );
	}

	Json&                     Json::GetChild( StringView key )
	{
		Json* child = mChildren[ key ];
		if( child )
			return *child;
		child = TAC_NEW Json;
		child->mType = JsonType::Null;
		mChildren[ key ] = child;
		mType = JsonType::Object;
		return *child;
	}
	Json&                     Json::operator[]( StringView key ) { return GetChild( key ); }
	Json&                     Json::operator[]( const char* key ) { return GetChild( key ); }
	void                      Json::operator = ( const Json& json ) { DeepCopy( &json ); }
	void                      Json::operator = ( const Json* json ) { DeepCopy( json ); }
	void                      Json::operator = ( StringView v ) { SetString( v ); }
	void                      Json::operator = ( JsonNumber v ) { SetNumber( v ); }
	void                      Json::operator = ( bool v ){ SetBool( v ); }

	Json*                     Json::AddChild()
	{
		mType = JsonType::Array;
		auto child = TAC_NEW Json;
		mElements.push_back( child );
		return child;
	}

	Json*                     Json::AddChild( StringView key )
	{
		mType = JsonType::Object;
		auto child = TAC_NEW Json;
		mChildren[ key ] = child;
		return child;
	}

	void                      Json::DeepCopy( const Json* json )
	{
		mType = json->mType;
		mString = json->mString;
		mNumber = json->mNumber;
		mBoolean = json->mBoolean;
		for( auto pair : json->mChildren )
			AddChild( pair.first )->DeepCopy( pair.second );
		for( const Json* child : json->mElements )
			AddChild()->DeepCopy( child );
	}



}
