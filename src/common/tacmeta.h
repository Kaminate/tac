// Apparently I use this for serializing my terrain
// I should combine this with tacJson.h

#pragma once

#include "common/tacString.h"
#include "common/containers/tacVector.h"

#include <functional>
#include <map>

struct TacMeta;
struct TacMetaVar;
struct TacMetaVarCArray;
struct TacMetaVarDynArray;
struct TacMetaType;

//enum TacMetaPod
//{
//  Unknown,
//  tacfloat,
//  tacint32,
//};

struct TacMetaVar
{
  TacString mName;
  int mOffset = 0;
  TacMetaType* mMetaType = nullptr;
};

struct TacMetaVarCArray : public TacMetaVar
{
  int mCArrayCount = 0;
};

struct TacMetaVarDynArray : public TacMetaVar
{
  std::function<void( void*, int )>mResizeFunction;
  std::function<void*( void* )>mDataFunction;
};

struct TacMetaType
{
  TacString mName;
  int mSize = 0;
};
template< typename T >
struct TacMetaPodType : public TacMetaType
{
  //TacMetaPod mMetaPod = TacMetaPod::Unknown;
};
struct TacMetaCompositeType : public TacMetaType
{
  TacVector< TacMetaVar* > mMetaVars;
};

struct TacMeta
{
  TacMeta();
  static TacMeta* GetInstance();
  TacMetaType* GetType( const TacString& name );
  void AddType( TacMetaType* metaType );
  //void Load( std::ifstream& ifs, TacMetaType* metaType, void* data, TacErrors& errors );

  std::map< TacString, TacMetaType* > metaTypes;
};

