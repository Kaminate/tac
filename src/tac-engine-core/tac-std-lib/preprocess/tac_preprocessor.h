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

  template< typename T>
  struct OnDestructAux
  {
    OnDestructAux( T t ) : mT( t ) {}
    OnDestructAux( const OnDestructAux< T >& ) = delete;
    ~OnDestructAux() { mT(); }
    T mT;
  };
}

// -------------------------------------------------------------------------------------------------

#define TAC_NO_OP                                          // indicate that we are purposely doing nothing
#define TAC_ARRAY_SIZE( a )                                ( sizeof( a ) / sizeof( a[ 0 ] ) )
#define TAC_CONCAT_AUX( a, b )                             a##b
#define TAC_CONCAT( a, b )                                 TAC_CONCAT_AUX( a, b )
#define TAC_OFFSET_OF( T, m )                              ((int)(size_t)&reinterpret_cast<char const volatile&>((((T*)0)->m)))
#define TAC_TYPESAFE_STRINGIFY_TYPE( T )                   ( (T*)nullptr, #T )
#define TAC_TYPESAFE_STRINGIFY_MEMBER( T, m )              ( ( const char* )TAC_OFFSET_OF( T, m ), #m )
#define TAC_PAD_BYTES( n )                                 char TAC_CONCAT( mPadding, __COUNTER__ )[ n ]
#define TAC_UNUSED_PARAMETER( param )                      ( void ) param // replace with c++17 [[maybe_unused]]?
#define TAC_TMP_VAR_NAME                                   TAC_CONCAT( tmp , __COUNTER__)
#define TAC_SCOPE_GUARD( Type, ... )                       Type TAC_TMP_VAR_NAME { __VA_ARGS__ }
#define TAC_ON_DESTRUCT_AUX( code, lambda )                auto lambda = [&](){ code; }; TAC_SCOPE_GUARD( Tac::OnDestructAux< decltype( lambda ) >, lambda  )
#define TAC_ON_DESTRUCT( code )                            TAC_ON_DESTRUCT_AUX( code, TAC_TMP_VAR_NAME )
#define TAC_TEMPORARILY_DISABLED() 0                       // Show the intent to re-enable it later
#define TAC_DELETE_ME()                                    0 // if you see this you are free to delete the contents
#define dynmc                                              // purposely not const

// for dynmc,
//     cpp.hint *should* suppress warning VCR001, but there is a workaround in Visual Studio:
//     Options > Text Editor > C/C++ > View -> set "Macros in Skipped Browsking Regions" to "None"

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
#else
#pragma warning( disable: 4201 ) // nameless struct/union
#endif // #if TAC_IS_DEBUG_MODE()

// -------------------------------------------------------------------------------------------------

