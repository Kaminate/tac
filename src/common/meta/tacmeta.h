#pragma once

#include "src/common/string/tacString.h"
#include "src/common/containers/tacVector.h"

#include <functional>
#include <map>

namespace Tac
{
  struct MetaType
  {
    virtual const char* GetName() const = 0;
    virtual size_t      GetSizeOf() const = 0;
    virtual const char* ToString( void* ) const = 0;
    virtual float       ToNumber( void* ) const = 0;
    virtual void        Cast( void* dst, void* src, const MetaType* srcType ) const = 0;

    // others...
    // New
    // Placement New
    // Delete
    // Dtor
    // Cast
    // Dereference Type
    // Dereference
    // Address type
    // Address
    // Lua Accessors
    // Serialization
    // Parsing
    // Alignment
    // Metadata
  };


  const MetaType& GetMetaType( const int& );
  const MetaType& GetMetaType( const float& );
  const MetaType& GetMetaType( const char*& );
  const MetaType& GetNullMetaType();

  template< typename T > const MetaType& GetMetaType()
  {
    T t;
    return GetMetaType( t );
  }

  template<> inline const MetaType&      GetMetaType< void >()
  {
    return GetNullMetaType();
  }

  template < typename T > T              MetaCast( void* v, const MetaType* m )
  {
    T t;
    GetMetaType( t )->Cast( &t, v, m );
    return t;
  }

  template< typename T >
  struct AutoLister
  {
    struct Iterator
    {
      Iterator( T* t ) : mT( t )            {}
      T* operator*()                        { return mT; }
      void operator++()                     { mT = ( ( AutoLister* )mT )->mNext; }
      bool operator!=( const Iterator& it ) { return mT != it.mT; }
      T* mT;
    };

    struct Range
    {
      Iterator begin()                      { return Head(); }
      Iterator end()                        { return nullptr; }
    };

    static T*& Head()                       { static T* sHead; return sHead; }
    AutoLister()                            { mNext = Head(); Head() = ( T* )this; }
    T* mNext;
  };



//  struct Meta;
//  struct MetaVar;
//  struct MetaVarCArray;
//  struct MetaVarDynArray;
//  struct MetaType;
//
//  struct MetaType
//  {
//    StringView mName;
//    int        mSize = 0;
//  };
//
//
//  struct MetaVarCArray : public MetaVar
//  {
//    int mCArrayCount = 0;
//  };
//
//  struct MetaVarDynArray : public MetaVar
//  {
//    std::function< void( void*, int ) > mResizeFunction;
//    std::function< void*( void* ) >     mDataFunction;
//  };
//
//
//  template< typename T >
//  struct MetaPodType : public MetaType
//  {
//    //MetaPod mMetaPod = MetaPod::Unknown;
//  };
//





//
//  void         MetaInit();
//  MetaType*    MetaGetTypeByName( StringView );
//  template< typename T >
//  MetaType*    MetaGetType();
//  void         MetaAddType( MetaType* );
//  void         MetaRegisterType( const char* name, int size );
//
//
//  template< typename T > struct     MetaTypeNameRegistry { static StringView sName; };
//  template< typename T > StringView MetaGetTypeName() { return MetaTypeNameRegistry< T >::sName; }
//#define TAC_META_REGISTER_TYPE_NAME( T ) MetaTypeNameRegistry< T >::sName = #T;
//  //template< typename T > void       MetaRegisterTypeName() 
//
//#define TAC_META_REGISTER_TYPE( T )    \
//   TAC_META_REGISTER_TYPE_NAME( T );   \
//   MetaRegisterType( #T, sizeof( T ) );

    void MetaUnitTestTitle( const char* );
#define TAC_META_UNIT_TEST_TITLE MetaUnitTestTitle( __FUNCTION__ )
} // namespace Tac

