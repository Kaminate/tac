// Contains common defines, template utility functions, preprocessor magic/hackery
//

#pragma once

// -------------------------------------------------------------------------------------------------

#ifdef NDEBUG
  #define TAC_IS_DEBUG_MODE() 0
#else
  #define TAC_IS_DEBUG_MODE() 1
#endif

// -------------------------------------------------------------------------------------------------

namespace Tac
{
  extern int asdf; // for edit-and-continue debug shenanigans

#if TAC_IS_DEBUG_MODE()
  constexpr bool kIsDebugMode { true };
#else
  constexpr bool kIsDebugMode {};
#endif

#define TAC_SHOULD_IMPORT_STD() false
}

// -------------------------------------------------------------------------------------------------

//      indicate that we are purposely doing nothing
#define TAC_NO_OP 
#define TAC_NO_OP_RETURN( val ) return val

#define TAC_ARRAY_SIZE( a )                                ( sizeof( a ) / sizeof( a[ 0 ] ) )
#define TAC_CONCAT_AUX( a, b )                             a##b
#define TAC_CONCAT( a, b )                                 TAC_CONCAT_AUX( a, b )
#define TAC_STRINGIFY_AUX( stuff )                         #stuff
#define TAC_STRINGIFY( stuff )                             TAC_STRINGIFY_AUX( stuff )
#define TAC_OFFSET_OF( type, member )                      ((int)(size_t)&reinterpret_cast<char const volatile&>((((type*)0)->member)))

#define TAC_PAD_BYTES( n )                                 char TAC_CONCAT( mPadding, __COUNTER__ )[ n ]

// replace with c++17 [[maybe_unused]]?
#define TAC_UNUSED_PARAMETER( param )                      ( void ) param

// -------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------

#define TAC_TMP_VAR_NAME                       TAC_CONCAT( tmp , __COUNTER__)

#define TAC_SCOPE_GUARD( Type, ... )           Type TAC_TMP_VAR_NAME { __VA_ARGS__ }

// -------------------------------------------------------------------------------------------------

namespace Tac
{
  template< typename T>
  struct OnDestructAux
  {
    OnDestructAux( T t ) : mT( t ) {}
    OnDestructAux( const OnDestructAux< T >& ) = delete;
    ~OnDestructAux() { mT(); }
    T mT;
  };
}


#define TAC_ON_DESTRUCT_AUX( code, lambda )                auto lambda = [&](){ code; }; TAC_SCOPE_GUARD( Tac::OnDestructAux< decltype( lambda ) >, lambda  )
#define TAC_ON_DESTRUCT( code )                            TAC_ON_DESTRUCT_AUX( code, TAC_TMP_VAR_NAME )

// used this (instead of commenting out) to show the intent to re-enable it later
#define TAC_TEMPORARILY_DISABLED() 0

#define TAC_DELETE_ME() 0 // if you see this you are free to delete the contents

// Will throw E0135 compile error "structure" has no member "var" if misspelled/renamed
#define TAC_MEMBER_NAME( structure, var ) ( ( const char* )TAC_OFFSET_OF( structure, var ), TAC_STRINGIFY( var ) )


// -------------------------------------------------------------------------------------------------

namespace Tac
{
  template < class T > struct remove_reference       { using type = T; };
  template < class T > struct remove_reference< T& >  { using type = T; };
  template < class T > struct remove_reference< T&& > { using type = T; };

  template <class T > using remove_reference_t = typename remove_reference<T>::type;

  template< typename T > remove_reference_t< T >&& move( T&& t )
  {
    // 't' is a lvalue (named rvalue reference)
    //
    // If 'T' is a 'Foo&', then we are calling this function with 'Foo& && t',
    // which collapses to 'Foo&'
    // ...
    // etc idk
    return static_cast< remove_reference_t< T >&& >( t );
  }

  template < class T > constexpr T&& forward( remove_reference_t< T >& t ) noexcept { return static_cast< T&& >( t ); }
  template < class T > constexpr T&& forward( remove_reference_t< T >&& t ) noexcept { return static_cast< T&& >( t ); }

  struct true_type  { static const bool value { true }; };
  struct false_type { static const bool value { false }; };
  template< typename T, typename U > struct is_same : public false_type {};
  template< typename T >             struct is_same< T, T > : public true_type {};
}

// -------------------------------------------------------------------------------------------------

#if TAC_IS_DEBUG_MODE()
#if defined( _MSC_VER )
#pragma warning( disable: 4100 ) // unused function parameter
#pragma warning( disable: 4189 ) // local variable is initialized but not referenced
#pragma warning( disable: 4505 ) // unreferenced function with internal linkage has been removed
#pragma warning( disable: 4702 ) // unreachable code. (its fucky with ranged for)
#pragma warning( disable: 4201 ) // nameless struct/union
#pragma warning( disable: 5050 ) // possible incompatible module import env
#pragma warning( disable: 6011 ) // possible nullptr dereference
#pragma warning( disable: 6385 ) // buffer read overrun
#pragma warning( disable: 6386 ) // buffer overrun indexing array (no its not)
#pragma warning( disable: 6387 ) // unexpected value passed to annotated argument
#pragma warning( disable: 28251 ) // inconsistant annotation for new
#pragma warning( disable: 33011 ) // unchecked upper bound array enum index
#endif // #if defined( _MSC_VER )
#endif // #if TAC_IS_DEBUG_MODE()

// -------------------------------------------------------------------------------------------------

// A dynmc variable is a variable that has not been declared const
// ie:
//     dynmc char* src{ ... };
//     const char* dst{ ... };
//     MemCpy( dst, src, n );
#define dynmc 

// -------------------------------------------------------------------------------------------------

// insane
#define ctor // constructor 
#define dtor // destructor
#define oper // operator

// -------------------------------------------------------------------------------------------------

