#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  struct Json;
  struct ReadStream;
  struct WriteStream;
  struct String;

  struct MetaType
  {
    struct CastParams
    {
      dynmc void*     mDst     {};
      const void*     mSrc     {};
      const MetaType* mSrcType {};
    };

    struct CopyParams
    {
      dynmc void* mDst {};
      const void* mSrc {};
    };

    virtual const char* GetName() const = 0;
    virtual int         GetSizeOf() const = 0;
    virtual String      ToString( const void* ) const = 0;
    virtual float       ToNumber( const void* ) const = 0;
    virtual void        Cast( CastParams ) const = 0;

    virtual void        JsonSerialize( dynmc Json*, const void* ) const = 0;
    virtual void        JsonDeserialize( const Json*, dynmc void* ) const = 0;

    virtual void        Read( ReadStream*, dynmc void* ) const;
    virtual void        Write( WriteStream*, const void* ) const;

    virtual bool        Equals( const void*, const void* ) const;
    virtual void        Copy( CopyParams ) const;

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



} // namespace Tac

