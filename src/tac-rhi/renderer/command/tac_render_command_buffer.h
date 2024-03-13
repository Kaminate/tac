#pragma once

#include "tac-rhi/renderer/command/tac_render_command_type.h"
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac::Render
{

  struct CommandBuffer;
  struct CommandBufferIterator
  {
    bool                      IsValid();
    void*                     PopBytes( int );
    void                      PopCanary();
    template< typename T > T* Pop()            { return ( T* )PopBytes( sizeof( T ) ); }
    template< typename T > T* PopCommandData() { T* t = Pop< T >(); PopCanary(); return t; }

    const char*               bufferBegin = nullptr;
    const char*               bufferEnd = nullptr;
    const char*               bufferPos = nullptr;
  };

  // -----------------------------------------------------------------------------------------------


  struct CommandBuffer
  {
    void           PushCommand( CommandType, const void*, int );

    void           Resize( int );
    void           Clear();
    const char*    Data() const;
    int            Size() const;

    CommandBufferIterator GetIterator();

  private:
    void           Push( const void* , int );
    Vector< char > mBuffer;
  };

  // -----------------------------------------------------------------------------------------------



} // namespace Tac::Render

