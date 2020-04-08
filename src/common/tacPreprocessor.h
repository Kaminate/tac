// Contains common defines, template utility functions, preprocessor magic/hackery

#pragma once
#include "src/common/tacString.h"

namespace Tac
{

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
  // Am entertaining the idea of changing the return type to String
  // and calling imgui overloads that take Strings
  char* va( const char* format, ... );

#define TAC_CONCAT_AUX( a, b ) a##b
#define TAC_CONCAT( a, b ) TAC_CONCAT_AUX( a, b )

#define TAC_STRINGIFY_AUX( stuff ) #stuff
#define TAC_STRINGIFY( stuff ) TAC_STRINGIFY_AUX( stuff )

  // Used to bypass warnings when you don't use all the arguments passed
  // to you in a function call, without having to lower the warning level
#define TAC_UNUSED_PARAMETER( param ) ( void ) param



  struct StackFrame
  {
    StackFrame() = default;
    StackFrame( int line, StringView file, StringView function );
    int mLine = 0;
    String mFile;
    String mFunction;
    String ToString() const;
  };

  #define TAC_STACK_FRAME Tac::StackFrame( __LINE__, __FILE__, __FUNCTION__ )


  bool IsDebugMode();

  void AssertInternal( const String& message, const StackFrame& frame );

  // TODO: make macros TAC_CAPS

#define TAC_ASSERT_MESSAGE( formatString, ... ) AssertInternal( va( formatString, ## __VA_ARGS__ ), TAC_STACK_FRAME )
#define TAC_ASSERT( expression ) if( !( expression ) ){ TAC_ASSERT_MESSAGE( TAC_STRINGIFY( expression ) ); }
#define TAC_INVALID_CODE_PATH TAC_ASSERT_MESSAGE( "Invalid code path!" );
#define TAC_UNIMPLEMENTED TAC_ASSERT_MESSAGE( "Unimplemented!" );
#define TAC_INVALID_DEFAULT_CASE( var ) default: TAC_ASSERT_MESSAGE( "Invalid default case, %s = %i", TAC_STRINGIFY( var ), var ); break;
#define TAC_OFFSET_OF( type, member ) ((size_t)&reinterpret_cast<char const volatile&>((((type*)0)->member)))

  /* is this a good idea?
  template< typename TDerived, typename TBaseData >
  void DataSpawn( TDerived** ppResource, const TBaseData& resourceData )
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
    OnDestructAux( const OnDestructAux<T>&rhs ) = delete;
  };
  // dumb shit:
  // - can't use preprocessor commands in a OnDestruct
  // - can't use __LINE__ in a lambda
#define TAC_ON_DESTRUCT_AUX( code, lambdaName, dtorName ) auto lambdaName = [&](){ code; }; OnDestructAux< decltype( lambdaName ) > dtorName( lambdaName );
#define TAC_ON_DESTRUCT( code ) TAC_ON_DESTRUCT_AUX( code, TAC_CONCAT( lambda, __LINE__ ), TAC_CONCAT( dtor, __LINE__ ) )


}

