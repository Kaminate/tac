
#include "src/common/tacMeta.h"
#include "src/common/tacSerialization.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacUtility.h"
#include "src/common/tacMemory.h"
#include <fstream>

namespace Tac
{

Meta::Meta()
{
  // float
  {
    auto metaFloat = TAC_NEW MetaPodType< float >;
    metaFloat->mName = TAC_STRINGIFY( float );
    metaFloat->mSize = sizeof( float );
    AddType( metaFloat );
  }
  // int32_t
  {
    auto metaint32_t = TAC_NEW MetaPodType< int32_t >;
    metaint32_t->mName = TAC_STRINGIFY( int32_t );
    metaint32_t->mSize = sizeof( int32_t );
    AddType( metaint32_t );
  }
  // int
  {
    auto metaint = TAC_NEW MetaPodType< int >;
    metaint->mName = TAC_STRINGIFY( int );
    metaint->mSize = sizeof( int );
    AddType( metaint );
  }
  // v2
  {
    auto x = TAC_NEW MetaVar;
    x->mMetaType = GetType( "float" );
    x->mName = "x";
    x->mOffset = TAC_OFFSET_OF( v4, x );

    auto y = TAC_NEW MetaVar;
    y->mMetaType = GetType( "float" );
    y->mName = "y";
    y->mOffset = TAC_OFFSET_OF( v4, y );

    auto metav2 = TAC_NEW MetaCompositeType;
    metav2->mName = TAC_STRINGIFY( v2 );
    metav2->mMetaVars = { x, y };
    metav2->mSize = sizeof( v2 );
    AddType( metav2 );
  }
  // v3
  {
    auto x = TAC_NEW MetaVar;
    x->mMetaType = GetType( "float" );
    x->mName = "x";
    x->mOffset = TAC_OFFSET_OF( v3, x );

    auto y = TAC_NEW MetaVar;
    y->mMetaType = GetType( "float" );
    y->mName = "y";
    y->mOffset = TAC_OFFSET_OF( v3, y );

    auto z = TAC_NEW MetaVar;
    z->mMetaType = GetType( "float" );
    z->mName = "z";
    z->mOffset = TAC_OFFSET_OF( v3, z );

    auto metav3 = TAC_NEW MetaCompositeType;
    metav3->mName = TAC_STRINGIFY( v3 );
    metav3->mMetaVars = { x, y, z };
    metav3->mSize = sizeof( v3 );
    AddType( metav3 );
  }
  // v4
  {
    auto x = TAC_NEW MetaVar;
    x->mMetaType = GetType( "float" );
    x->mName = "x";
    x->mOffset = TAC_OFFSET_OF( v4, x );

    auto y = TAC_NEW MetaVar;
    y->mMetaType = GetType( "float" );
    y->mName = "y";
    y->mOffset = TAC_OFFSET_OF( v4, y );

    auto z = TAC_NEW MetaVar;
    z->mMetaType = GetType( "float" );
    z->mName = "z";
    z->mOffset = TAC_OFFSET_OF( v4, z );

    auto w = TAC_NEW MetaVar;
    w->mMetaType = GetType( "float" );
    w->mName = "w";
    w->mOffset = TAC_OFFSET_OF( v4, w );

    auto metav4 = TAC_NEW MetaCompositeType;
    metav4->mName = TAC_STRINGIFY( v4 );
    metav4->mMetaVars = { x, y, z, w };
    metav4->mSize = sizeof( v4 );
    AddType( metav4 );
  }
}

MetaType* Meta::GetType( StringView name )
{
  auto it = metaTypes.find( name );
  if( it == metaTypes.end() )
    return nullptr;
  return ( *it ).second;
}

void Meta::AddType( MetaType* metaType )
{
  TAC_ASSERT( !GetType( metaType->mName ) );
  TAC_ASSERT( metaType->mName.size() );
  TAC_ASSERT( metaType->mSize );
  metaTypes[ metaType->mName ] = metaType;
}

//void Meta::Load(
//  std::ifstream& ifs,
//  MetaType* metaType,
//  void* data,
//  Errors& errors )
//{
//  for( auto metaVar : metaType->mMetaVars )
//  {
//    void* varBaseData = ( char* )data + metaVar.offset;
//    void* arrayBaseData = varBaseData;
//    String varName;
//    if( !( ifs >> varName ) )
//    {
//      errors += "fux";
//      return;
//    }
//    if( varName != metaVar.name )
//    {
//      errors += "encountered " + varName + " expected " + metaType->name;
//      return;
//    }
//    int varCount = 1;
//    if( metaVar.mIsStdArray )
//    {
//      if( !( ifs >> varCount ) )
//      {
//        errors += "fux";
//        return;
//      }
//      metaVar.mResizeFunction( varBaseData, varCount );
//      arrayBaseData = metaVar.mDataFunction( varBaseData );
//    }
//    if( metaVar.mCArrayCount )
//      varCount = metaVar.mCArrayCount;
//    for( int iVar = 0; iVar < varCount; ++iVar )
//    {
//      void* varData = ( char* )arrayBaseData + iVar * metaVar.mMetaType->size;
//      Load( ifs, metaVar.mMetaType, varData, errors );
//      TAC_HANDLE_ERROR( errors );
//    }
//  }
//  if(!metaType->mMetaVars.empty())
//    return;
//
//  Assert( metaType->mMetaPod != Pod::Unknown );
//  String podStr;
//  if( !( ifs >> podStr ) )
//  {
//    errors = "fux";
//    return;
//  }
//
//  switch( metaType->mMetaPod )
//  {
//    case Pod::Float: *( ( float* )data ) = ( float )atof( podStr.c_str() ); break;
//    case Pod::Int32: *( ( int32_t* )data ) = ( int32_t )atoi( podStr.c_str() ); break;
//    TAC_INVALID_DEFAULT_CASE(metaType->mMetaPod);
//  }
//}

Meta* Meta::GetInstance()
{
  static Meta meta;
  return &meta;
}

}

