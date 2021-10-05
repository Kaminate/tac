#include "src/common/meta/tacMeta.h"
#include "src/common/tacSerialization.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacUtility.h"
#include "src/common/tacMemory.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacPreprocessor.h"
#include <fstream>
#include <iostream>
#include <iomanip>

namespace Tac
{
  //const char* MetaType::GetName() const          { return 0; }
  //size_t      MetaType::GetSizeOf() const        { return 0; }
  //const char* MetaType::ToString( void* ) const  { return 0; }
  //float       MetaType::ToNumber( void* ) const  { return 0; }
  //void        MetaType::Cast( void*, void*, const MetaType* ) const {}


  static struct IntMetaType : public MetaType
  {
    const char* GetName() const override             { return "int"; }
    size_t      GetSizeOf() const override           { return sizeof( int ); }
    const char* ToString( void* v ) const override   { return FrameMemoryPrintf( "%i", *( int* )v ); }
    float       ToNumber( void* v ) const override   { return ( float )*( int* )v; }
    void        Cast( void* dst, void* src, const MetaType* srcType ) const override
    {
      *( int* )dst = ( int )srcType->ToNumber( src );
    }
  } sIntMetaType;

  static struct CharStarMetaType : public MetaType
  {
    const char* GetName() const override             { return "char*"; }
    size_t      GetSizeOf() const override           { return sizeof( char* ); }
    const char* ToString( void* v ) const override   { return *( const char** )v; }
    float       ToNumber( void* v ) const override   { return ( float )Atoi( *( const char** )v ); }
    void        Cast( void* dst, void* src, const MetaType* srcType ) const override
    {
      *( const char** )dst = srcType->ToString( src );
    }
  } sCharStarMetaType;

  static struct FloatMetaType : public MetaType
  {
    const char* GetName() const override             { return "float"; }
    size_t      GetSizeOf() const override           { return sizeof( float ); }
    const char* ToString( void* v ) const override   { return FrameMemoryPrintf( "%f", *( float* )v ); }
    float       ToNumber( void* v ) const override   { return *( float* )v; }
    void        Cast( void* dst, void* src, const MetaType* srcType ) const override
    {
      *( float* )dst = ( float )srcType->ToNumber( src );
    }
  } sFloatMetaType;

  static struct NullMetaType : public MetaType
  {
    const char* GetName() const override             { return "null"; }
    size_t      GetSizeOf() const override           { return 0; }
    const char* ToString( void* v ) const override   { return "null"; }
    float       ToNumber( void* v ) const override   { return 0; }
    void        Cast( void* dst, void* src, const MetaType* srcType ) const override
    {
      TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
    }
  } sNullMetaType;

  const MetaType& GetMetaType( const int& )   { return sIntMetaType; }
  const MetaType& GetMetaType( const float& ) { return sFloatMetaType; }
  const MetaType& GetMetaType( const char*& ) { return sCharStarMetaType; }
  const MetaType& GetNullMetaType()           { return sNullMetaType; }

//static std::map< String, MetaType* > metaTypes;





//void         MetaInit()
//{
//  // float
//  {
//    auto metaFloat = TAC_NEW MetaPodType< float >;
//    metaFloat->mName = TAC_STRINGIFY( float );
//    metaFloat->mSize = sizeof( float );
//    MetaAddType( metaFloat );
//  }
//  // int32_t
//  {
//    auto metaint32_t = TAC_NEW MetaPodType< int32_t >;
//    metaint32_t->mName = TAC_STRINGIFY( int32_t );
//    metaint32_t->mSize = sizeof( int32_t );
//    MetaAddType( metaint32_t );
//  }
//  // int
//  {
//    auto metaint = TAC_NEW MetaPodType< int >;
//    metaint->mName = TAC_STRINGIFY( int );
//    metaint->mSize = sizeof( int );
//    MetaAddType( metaint );
//  }

//  // v2
//  {
//    auto x = TAC_NEW MetaVar;
//    x->mMetaType = MetaGetType( "float" );
//    x->mName = "x";
//    x->mOffset = TAC_OFFSET_OF( v4, x );

//    auto y = TAC_NEW MetaVar;
//    y->mMetaType = MetaGetType( "float" );
//    y->mName = "y";
//    y->mOffset = TAC_OFFSET_OF( v4, y );

//    auto metav2 = TAC_NEW MetaCompositeType;
//    metav2->mName = TAC_STRINGIFY( v2 );
//    metav2->mMetaVars = { x, y };
//    metav2->mSize = sizeof( v2 );
//    MetaAddType( metav2 );
//  }

//  // v3
//  {
//    auto x = TAC_NEW MetaVar;
//    x->mMetaType = MetaGetType( "float" );
//    x->mName = "x";
//    x->mOffset = TAC_OFFSET_OF( v3, x );

//    auto y = TAC_NEW MetaVar;
//    y->mMetaType = MetaGetType( "float" );
//    y->mName = "y";
//    y->mOffset = TAC_OFFSET_OF( v3, y );

//    auto z = TAC_NEW MetaVar;
//    z->mMetaType = MetaGetType( "float" );
//    z->mName = "z";
//    z->mOffset = TAC_OFFSET_OF( v3, z );

//    auto metav3 = TAC_NEW MetaCompositeType;
//    metav3->mName = TAC_STRINGIFY( v3 );
//    metav3->mMetaVars = { x, y, z };
//    metav3->mSize = sizeof( v3 );
//    MetaAddType( metav3 );
//  }
//  // v4
//  {
//    auto x = TAC_NEW MetaVar;
//    x->mMetaType = MetaGetType( "float" );
//    x->mName = "x";
//    x->mOffset = TAC_OFFSET_OF( v4, x );

//    auto y = TAC_NEW MetaVar;
//    y->mMetaType = MetaGetType( "float" );
//    y->mName = "y";
//    y->mOffset = TAC_OFFSET_OF( v4, y );

//    auto z = TAC_NEW MetaVar;
//    z->mMetaType = MetaGetType( "float" );
//    z->mName = "z";
//    z->mOffset = TAC_OFFSET_OF( v4, z );

//    auto w = TAC_NEW MetaVar;
//    w->mMetaType = MetaGetType( "float" );
//    w->mName = "w";
//    w->mOffset = TAC_OFFSET_OF( v4, w );

//    auto metav4 = TAC_NEW MetaCompositeType;
//    metav4->mName = TAC_STRINGIFY( v4 );
//    metav4->mMetaVars = { x, y, z, w };
//    metav4->mSize = sizeof( v4 );
//    MetaAddType( metav4 );
//  }
//}

//MetaType*    MetaGetType( StringView name );
//void         MetaAddType( MetaType* );

//MetaType* MetaGetType( StringView name )
//{
//  auto it = metaTypes.find( name );
//  if( it == metaTypes.end() )
//    return nullptr;
//  return ( *it ).second;
//}

//void      MetaAddType( MetaType* metaType )
//{
//  TAC_ASSERT( !MetaGetType( metaType->mName ) );
//  TAC_ASSERT( metaType->mName.size() );
//  TAC_ASSERT( metaType->mSize );
//  metaTypes[ metaType->mName ] = metaType;
//}

////void Meta::Load(
////  std::ifstream& ifs,
////  MetaType* metaType,
////  void* data,
////  Errors& errors )
////{
////  for( auto metaVar : metaType->mMetaVars )
////  {
////    void* varBaseData = ( char* )data + metaVar.offset;
////    void* arrayBaseData = varBaseData;
////    String varName;
////    if( !( ifs >> varName ) )
////    {
////      errors += "fux";
////      return;
////    }
////    if( varName != metaVar.name )
////    {
////      errors += "encountered " + varName + " expected " + metaType->name;
////      return;
////    }
////    int varCount = 1;
////    if( metaVar.mIsStdArray )
////    {
////      if( !( ifs >> varCount ) )
////      {
////        errors += "fux";
////        return;
////      }
////      metaVar.mResizeFunction( varBaseData, varCount );
////      arrayBaseData = metaVar.mDataFunction( varBaseData );
////    }
////    if( metaVar.mCArrayCount )
////      varCount = metaVar.mCArrayCount;
////    for( int iVar = 0; iVar < varCount; ++iVar )
////    {
////      void* varData = ( char* )arrayBaseData + iVar * metaVar.mMetaType->size;
////      Load( ifs, metaVar.mMetaType, varData, errors );
////      TAC_HANDLE_ERROR( errors );
////    }
////  }
////  if(!metaType->mMetaVars.empty())
////    return;
////
////  Assert( metaType->mMetaPod != Pod::Unknown );
////  String podStr;
////  if( !( ifs >> podStr ) )
////  {
////    errors = "fux";
////    return;
////  }
////
////  switch( metaType->mMetaPod )
////  {
////    case Pod::Float: *( ( float* )data ) = ( float )atof( podStr.c_str() ); break;
////    case Pod::Int32: *( ( int32_t* )data ) = ( int32_t )atoi( podStr.c_str() ); break;
////    TAC_ASSERT_INVALID_DEFAULT_CASE(metaType->mMetaPod);
////  }
////}


  void MetaUnitTestTitle( const char* title )
  {
    const char* dashes = "-----------";
    std::cout << dashes << " " << title << " " << dashes << std::endl;
  }

} // namespace Tac

