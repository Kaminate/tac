
// Apparently I use this for serializing my terrain
// I should combine this with Json.h

#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"

#include <functional>
#include <map>

namespace Tac
{
  struct Meta;
  struct MetaVar;
  struct MetaVarCArray;
  struct MetaVarDynArray;
  struct MetaType;

  //enum MetaPod
  //{
  //  Unknown,
  //  Float,
  //  Int32,
  //};

  struct MetaVar
  {
    String mName;
    int mOffset = 0;
    MetaType* mMetaType = nullptr;
  };

  struct MetaVarCArray : public MetaVar
  {
    int mCArrayCount = 0;
  };

  struct MetaVarDynArray : public MetaVar
  {
    std::function<void( void*, int )>mResizeFunction;
    std::function<void*( void* )>mDataFunction;
  };

  struct MetaType
  {
    String mName;
    int mSize = 0;
  };
  template< typename T >
  struct MetaPodType : public MetaType
  {
    //MetaPod mMetaPod = MetaPod::Unknown;
  };
  struct MetaCompositeType : public MetaType
  {
    Vector< MetaVar* > mMetaVars;
  };

  struct Meta
  {
    Meta();
    static Meta* GetInstance();
    MetaType* GetType( StringView name );
    void AddType( MetaType* metaType );
    //void Load( std::ifstream& ifs, MetaType* metaType, void* data, Errors& errors );

    std::map< String, MetaType* > metaTypes;
  };


}

