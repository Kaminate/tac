#pragma once

#include "src/common/string/tacString.h"
#include "src/common/containers/tacVector.h"

#include <functional>
#include <map>

namespace Tac
{
  struct MetaType
  {
    virtual const char* GetName() const;
    virtual size_t      GetSizeOf() const;
    virtual const char* ToString( void* ) const;
    virtual float       ToNumber( void* ) const;
    virtual void        Cast( void* dst, void* src, MetaType* srcType ) const;

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

  template<> inline const MetaType& GetMetaType< void >()
  {
    return GetNullMetaType();
  }

  template <typename T> T MetaCast( void* v, const MetaType* m )
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
      Iterator( T* t ) : mT( t ) {}
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

  struct MetaFn : public AutoLister< MetaFn >
  {
  public:
    //const char*     Name() const;
    //const MetaType* RetType() const;
    //const MetaType* ArgType( int ) const;
    //int             ArgCount() const;

    //void Apply( ? ? ? );
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
//  struct MetaVar
//  {
//    StringView    mName;
//    int           mOffset = 0;
//    MetaType*     mMetaType = nullptr;
//  };
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
//  struct MetaCompositeType : public MetaType
//  {
//    Vector< MetaVar* > mMetaVars;
//  };
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
}

