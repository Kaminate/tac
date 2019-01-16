#include <fstream>
#include <iostream>

#include "tacmeta.h"
#include "tacSerialization.h"
#include "tacPreprocessor.h"
#include "tacUtility.h"

TacMeta::TacMeta()
{
  auto metaFloat = new Type();
  metaFloat->mMetaPod = Pod::tacfloat;
  metaFloat->name = TacStringify( float );
  metaFloat->size = sizeof( float );
  AddType( metaFloat );

  auto metaint32_t = new Type();
  metaint32_t->mMetaPod = Pod::tacint32;
  metaint32_t->name = TacStringify( int32_t );
  metaint32_t->size = sizeof( int32_t );
  AddType( metaint32_t );

  Var x;
  x.mMetaType = metaFloat;
  x.name = "x";
  x.offset = TacOffsetOf( v3, x );

  Var y;
  y.mMetaType = metaFloat;
  y.name = "y";
  y.offset = TacOffsetOf( v3, y );

  Var z;
  z.mMetaType = metaFloat;
  z.name = "z";
  z.offset = TacOffsetOf( v3, z );

  auto metav3 = new Type();
  metav3->name = TacStringify( v3 );
  metav3->mMetaVars = { x, y, z };
  metav3->size = sizeof( v3 );
  AddType( metav3 );

}

TacMeta::Type* TacMeta::GetType( const TacString& name )
{
  auto it = metaTypes.find( name );
  if( it == metaTypes.end() )
    return nullptr;
  return ( *it ).second;
}

void TacMeta::AddType( Type* metaType )
{
  TacAssert( !GetType( metaType->name ) );
  TacAssert( metaType->name.size() );
  TacAssert( metaType->size );
  metaTypes[ metaType->name ] = metaType;
}

// Make this static?
void TacMeta::Load(
  std::ifstream& ifs,
  Type* metaType,
  void* data,
  TacErrors& errors )
{
  for( auto metaVar : metaType->mMetaVars )
  {
    void* varBaseData = ( char* )data + metaVar.offset;
    void* arrayBaseData = varBaseData;
    TacString varName;
    if( !( ifs >> varName ) )
    {
      errors += "fux";
      return;
    }
    if( varName != metaVar.name )
    {
      errors += "encountered " + varName + " expected " + metaType->name;
      return;
    }
    int varCount = 1;
    if( metaVar.mIsStdArray )
    {
      if( !( ifs >> varCount ) )
      {
        errors += "fux";
        return;
      }
      metaVar.mResizeFunction( varBaseData, varCount );
      arrayBaseData = metaVar.mDataFunction( varBaseData );
    }
    if( metaVar.mCArrayCount )
      varCount = metaVar.mCArrayCount;
    for( int iVar = 0; iVar < varCount; ++iVar )
    {
      void* varData = ( char* )arrayBaseData + iVar * metaVar.mMetaType->size;
      Load( ifs, metaVar.mMetaType, varData, errors );
      TAC_HANDLE_ERROR( errors );
    }
  }
  if(!metaType->mMetaVars.empty())
    return;

  TacAssert( metaType->mMetaPod != Pod::Unknown );
  TacString podStr;
  if( !( ifs >> podStr ) )
  {
    errors = "fux";
    return;
  }

  switch( metaType->mMetaPod )
  {
    case Pod::tacfloat: *( ( float* )data ) = ( float )atof( podStr.c_str() ); break;
    case Pod::tacint32: *( ( int32_t* )data ) = ( int32_t )atoi( podStr.c_str() ); break;
    TacInvalidDefaultCase(metaType->mMetaPod);
  }
}

TacMeta* TacMeta::GetInstance()
{
  static TacMeta meta;
  return &meta;
}
