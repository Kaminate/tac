#include "common/tacmeta.h"
#include "common/tacSerialization.h"
#include "common/tacPreprocessor.h"
#include "common/tacUtility.h"
#include <fstream>
#include <iostream>


TacMeta::TacMeta()
{
  // float
  {
    auto metaFloat = new TacMetaPodType< float >();
    metaFloat->mName = TacStringify( float );
    metaFloat->mSize = sizeof( float );
    AddType( metaFloat );
  }
  // int32_t
  {
    auto metaint32_t = new TacMetaPodType< int32_t >();
    metaint32_t->mName = TacStringify( int32_t );
    metaint32_t->mSize = sizeof( int32_t );
    AddType( metaint32_t );
  }
  // int
  {
    auto metaint = new TacMetaPodType< int >();
    metaint->mName = TacStringify( int );
    metaint->mSize = sizeof( int );
    AddType( metaint );
  }
  // v2
  {
    auto x = new TacMetaVar;
    x->mMetaType = GetType( "float" );
    x->mName = "x";
    x->mOffset = TacOffsetOf( v4, x );

    auto y = new TacMetaVar;
    y->mMetaType = GetType( "float" );
    y->mName = "y";
    y->mOffset = TacOffsetOf( v4, y );

    auto metav2 = new TacMetaCompositeType();
    metav2->mName = TacStringify( v2 );
    metav2->mMetaVars = { x, y };
    metav2->mSize = sizeof( v2 );
    AddType( metav2 );
  }
  // v3
  {
    auto x = new TacMetaVar;
    x->mMetaType = GetType( "float" );
    x->mName = "x";
    x->mOffset = TacOffsetOf( v3, x );

    auto y = new TacMetaVar;
    y->mMetaType = GetType( "float" );
    y->mName = "y";
    y->mOffset = TacOffsetOf( v3, y );

    auto z = new TacMetaVar;
    z->mMetaType = GetType( "float" );
    z->mName = "z";
    z->mOffset = TacOffsetOf( v3, z );

    auto metav3 = new TacMetaCompositeType();
    metav3->mName = TacStringify( v3 );
    metav3->mMetaVars = { x, y, z };
    metav3->mSize = sizeof( v3 );
    AddType( metav3 );
  }
  // v4
  {
    auto x = new TacMetaVar;
    x->mMetaType = GetType( "float" );
    x->mName = "x";
    x->mOffset = TacOffsetOf( v4, x );

    auto y = new TacMetaVar;
    y->mMetaType = GetType( "float" );
    y->mName = "y";
    y->mOffset = TacOffsetOf( v4, y );

    auto z = new TacMetaVar;
    z->mMetaType = GetType( "float" );
    z->mName = "z";
    z->mOffset = TacOffsetOf( v4, z );

    auto w = new TacMetaVar;
    w->mMetaType = GetType( "float" );
    w->mName = "w";
    w->mOffset = TacOffsetOf( v4, w );

    auto metav4 = new TacMetaCompositeType();
    metav4->mName = TacStringify( v4 );
    metav4->mMetaVars = { x, y, z, w };
    metav4->mSize = sizeof( v4 );
    AddType( metav4 );
  }
}

TacMetaType* TacMeta::GetType( const TacString& name )
{
  auto it = metaTypes.find( name );
  if( it == metaTypes.end() )
    return nullptr;
  return ( *it ).second;
}

void TacMeta::AddType( TacMetaType* metaType )
{
  TacAssert( !GetType( metaType->mName ) );
  TacAssert( metaType->mName.size() );
  TacAssert( metaType->mSize );
  metaTypes[ metaType->mName ] = metaType;
}

//void TacMeta::Load(
//  std::ifstream& ifs,
//  TacMetaType* metaType,
//  void* data,
//  TacErrors& errors )
//{
//  for( auto metaVar : metaType->mMetaVars )
//  {
//    void* varBaseData = ( char* )data + metaVar.offset;
//    void* arrayBaseData = varBaseData;
//    TacString varName;
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
//  TacAssert( metaType->mMetaPod != Pod::Unknown );
//  TacString podStr;
//  if( !( ifs >> podStr ) )
//  {
//    errors = "fux";
//    return;
//  }
//
//  switch( metaType->mMetaPod )
//  {
//    case Pod::tacfloat: *( ( float* )data ) = ( float )atof( podStr.c_str() ); break;
//    case Pod::tacint32: *( ( int32_t* )data ) = ( int32_t )atoi( podStr.c_str() ); break;
//    TacInvalidDefaultCase(metaType->mMetaPod);
//  }
//}

TacMeta* TacMeta::GetInstance()
{
  static TacMeta meta;
  return &meta;
}
