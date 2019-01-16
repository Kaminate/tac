// Apparently I use this for serializing my terrain
// I should combine this with tacJson.h

#pragma once

#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"

#include <functional>
#include <map>

struct TacMeta
{
  enum Pod
  {
    Unknown,
    tacfloat,
    tacint32,
  };
  struct Type;
  struct Var
  {
    TacString name;
    int offset = 0;
    Type* mMetaType = nullptr;
    int mCArrayCount = 0;
    bool mIsStdArray = false;
    std::function<void( void*, int )>mResizeFunction;
    std::function<void*( void* )>mDataFunction;
  };
  struct Type
  {
    TacString name;
    TacVector< Var > mMetaVars;
    Pod mMetaPod = Pod::Unknown;
    int size = 0;
  };
  TacMeta();
  static TacMeta* GetInstance();
  Type* GetType( const TacString& name );
  void AddType( Type* metaType );
  // TODO: make static?
  void Load( std::ifstream& ifs, Type* metaType, void* data, TacErrors& errors );
  std::map< TacString, Type* > metaTypes;
};

