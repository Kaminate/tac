// Contains common defines, template utility functions, preprocessor magic/hackery
//
// no dependencies i guess
//

#pragma once

namespace Tac
{

#if defined(_MSC_VER)
#define TAC_EXPORT __declspec(dllexport)
#define TAC_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#define TAC_EXPORT __attribute__((visibility("default")))
#define TAC_IMPORT
#else
#error Unknown compiler
#define TAC_EXPORT
#define TAC_IMPORT
#endif

  struct StackFrame
  {
    StackFrame( int line = 0,
                const char* file = nullptr,
                const char* function = nullptr );
    const char* ToString() const;
    int         mLine;
    const char* mFile;
    const char* mFunction;
  };

  template< typename T>
  struct OnDestructAux
  {
    T mT;
    OnDestructAux( T t ) : mT( t ) {}
    ~OnDestructAux() { mT(); }
    OnDestructAux( const OnDestructAux< T >& rhs ) = delete;
  };

  bool IsDebugMode();
  void AssertInternal( const char* message, const StackFrame& frame );
  char* va( const char* format, ... );

#define TAC_ARRAY_SIZE( a )                                ( sizeof( a ) / sizeof( a[ 0 ] ) )
#define TAC_CONCAT_AUX( a, b )                             a##b
#define TAC_CONCAT( a, b )                                 TAC_CONCAT_AUX( a, b )
#define TAC_STRINGIFY_AUX( stuff )                         #stuff
#define TAC_STRINGIFY( stuff )                             TAC_STRINGIFY_AUX( stuff )
#define TAC_OFFSET_OF( type, member )                      ((size_t)&reinterpret_cast<char const volatile&>((((type*)0)->member)))
#define TAC_UNUSED_PARAMETER( param )                      ( void ) param
#define TAC_STACK_FRAME                                    StackFrame( __LINE__, __FILE__, __FUNCTION__ )
#define TAC_ASSERT_MESSAGE( formatString, ... )            AssertInternal( va( formatString, ## __VA_ARGS__ ), TAC_STACK_FRAME )
  // rename this
#define TAC_ASSERT_IF_MSG( expression, formatString, ... ) if( !( expression ) ){ TAC_ASSERT_MESSAGE( formatString, __VA_ARGS__ ); }
#define TAC_ASSERT( expression )                           if( !( expression ) ){ TAC_ASSERT_MESSAGE( TAC_STRINGIFY( expression ) ); }
#define TAC_ASSERT_INVALID_CODE_PATH                       TAC_ASSERT_MESSAGE( "Invalid code path!" );
#define TAC_ASSERT_UNIMPLEMENTED                           TAC_ASSERT_MESSAGE( "Unimplemented!" );
#define TAC_ASSERT_INVALID_CASE( var )                     TAC_ASSERT_MESSAGE( "Invalid default case, %s = %i", TAC_STRINGIFY( var ), var );
#define TAC_ON_DESTRUCT_AUX( code, lambdaName, dtorName )  auto lambdaName = [&](){ code; }; OnDestructAux< decltype( lambdaName ) > dtorName( lambdaName );
#define TAC_ON_DESTRUCT( code )                            TAC_ON_DESTRUCT_AUX( code, TAC_CONCAT( lambda, __LINE__ ), TAC_CONCAT( dtor, __LINE__ ) )
#define TAC_DEFINE_HANDLE( Handle )                                             \
  struct Handle                                                                 \
  {                                                                             \
    Handle( int index = -1 ) : mIndex( index ){}                                \
    bool operator ==( Handle handle ) const { return mIndex == handle.mIndex; } \
    bool operator !=( Handle handle ) const { return mIndex != handle.mIndex; } \
    bool              IsValid() const { return mIndex != -1; }                  \
    explicit operator int() const { return mIndex; }                            \
    explicit operator unsigned() const { return mIndex; }                       \
    int               mIndex;                                                   \
  };


}

