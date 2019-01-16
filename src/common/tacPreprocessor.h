// Contains common defines, template utility functions, preprocessor magic/hackery

#pragma once
#include "common/tacString.h"

#if defined(_MSC_VER)
// Microsoft
#define TAC_EXPORT __declspec(dllexport)
#define TAC_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
// GCC
#define TAC_EXPORT __attribute__((visibility("default")))
#define TAC_IMPORT
#else
#error Unknown compiler
#define TAC_EXPORT
#define TAC_IMPORT
#endif

// I use this a lot for passing to imgui
// Am entertaining the idea of changing the return type to TacString
// and calling imgui overloads that take TacStrings
char* va( const char* format, ... );

#define TacConcatAux( a, b ) a##b
#define TacConcat( a, b ) TacConcatAux( a, b )

#define TacStringifyAux( stuff ) #stuff
#define TacStringify( stuff ) TacStringifyAux( stuff )

// Used to bypass warnings when you don't use all the arguments passed
// to you in a function call, without having to lower the warning level
#define TacUnusedParameter( param ) ( void ) param



#define TAC_STACK_FRAME { __LINE__, __FILE__, __FUNCTION__ } 
struct TacStackFrame
{
  int mLine = 0;
  TacString mFile;
  TacString mFunction;
  TacString ToString() const;
};


bool TacIsDebugMode();

void TacAssertInternal( const TacString& message, const TacStackFrame& stackFrame );

#define TacAssertMessage( formatString, ... ) TacAssertInternal( va( formatString, ## __VA_ARGS__ ), TAC_STACK_FRAME )
#define TacAssert( expression ) if( !( expression ) ){ TacAssertMessage( TacStringify( expression ) ); }
#define TacInvalidCodePath TacAssertMessage( "Invalid code path!" );
#define TacUnimplemented TacAssertMessage( "Unimplemented!" );
#define TacInvalidDefaultCase( var ) default: TacAssertMessage( "Invalid default case, %s = %i", TacStringify( var ), var ); break;
#define TacOffsetOf( type, member ) ((size_t)&reinterpret_cast<char const volatile&>((((type*)0)->member)))

/* is this a good idea?
template< typename TDerived, typename TBaseData >
void TacDataSpawn( TDerived** ppResource, const TBaseData& resourceData )
{
  auto resource = new TDerived();
  *( TBaseData* )resource = resourceData;
  *ppResource = resource;
}
*/

// ie: OnDestruct( i++ )
template< typename T>
struct OnDestructAux
{
  T mT;
  OnDestructAux( T t ) : mT( t ) {}
  ~OnDestructAux() { mT(); }
  OnDestructAux(const OnDestructAux<T>&rhs) = delete;
};
// dumb shit:
// - can't use preprocessor commands in a OnDestruct
// - can't use __LINE__ in a lambda
#define OnDestruct2( code, lambdaName, dtorName ) auto lambdaName = [&](){ code; }; OnDestructAux< decltype( lambdaName ) > dtorName( lambdaName );
#define OnDestruct( code ) OnDestruct2( code, TacConcat( lambda, __LINE__ ), TacConcat( dtor, __LINE__ ) )


