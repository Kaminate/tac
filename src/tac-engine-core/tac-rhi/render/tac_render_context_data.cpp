#include "tac_render_context_data.h" // self-inc

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  BackendCmdSerializer::BackendCmdSerializer( BackendCmdData* data ) : mData{ data } {}

  void BackendCmdSerializer::PushCommandType( CommandType2 type )
  {
    mData->mCommands.push_back( type );
  }

  void BackendCmdSerializer::PushUpdateMemory( SmartPtr< UpdateMemory > mem )
  {
    mData->mUpdateMemories.push_back( mem );
  }

  void BackendCmdSerializer::PushBytes( const void* bytes, int byteCount )
  {
    const int n = mData->mCommandByteData.size();
    mData->mCommandByteData.resize( n + byteCount );
    MemCpy( mData->mCommandByteData.data() + n, bytes, byteCount );
  }

  void BackendCmdSerializer::PushString( StringView s )
  {
    const int n = s.size();
    PushInt( n );
    PushBytes( s.data(), n );
  }

  void BackendCmdSerializer::PushPointer( const void* ptr )
  {
    PushBytes( ptr, sizeof( void* ) );
  }

  void BackendCmdSerializer::PushStackFrame( const StackFrame& sf )
  {
    PushPointer( sf.mFile );
    PushPointer( sf.mFunction );
    PushInt( sf.mLine );
  }

  void BackendCmdSerializer::PushInt( int i )
  {
    PushBytes( &i, sizeof( int ) );
  }

  void BackendCmdSerializer::PushRenderHandle( const RenderHandle& rh )
  {
    mData->mRenderHandles.push_back( rh );
  }

  void BackendCmdSerializer::PushDynBufCreateParams( const DynBufCreateParams& params )
  {
    PushCommandType( CommandType2::kCreateDynamicBuffer );
    PushRenderHandle( params.mHandle );
    PushInt( params.mByteCount );
    PushStackFrame( params.mStackFrame );
  }

  void BackendCmdSerializer::PushDynBufUpdateParams( const DynBufUpdateParams& params ) 
  {
    PushCommandType( CommandType2::kUpdateDynamicBuffer );
    PushRenderHandle( params.mHandle );
    PushInt( params.mByteOffset );
    PushUpdateMemory( params.mUpdateMemory );
  }

  void BackendCmdSerializer::PushSetRenderObjectNameParams( const SetRenderObjectNameParams& params ) 
  {
    PushCommandType( CommandType2::kSetName );
    PushRenderHandle( params.mHandle );
    PushString( params.mName );
  }

  // -----------------------------------------------------------------------------------------------

  BackendCmdDeserializer::BackendCmdDeserializer( BackendCmdData* data ) :mData{ data } {}

  bool         BackendCmdDeserializer::IsValid() const
  {
    const int n = mData->mCommands.size();
    return mCmdTypeIdx < n;
  }

  const void*  BackendCmdDeserializer::PopBytes( int byteCount )
  {
    const int n = mData->mCommandByteData.size();
    TAC_ASSERT( mCmdByteIdx + byteCount <= n );
    const void* bytes = mData->mCommandByteData.data() + mCmdByteIdx;
    mCmdByteIdx += byteCount;
    return bytes;
  }

  int          BackendCmdDeserializer::PopInt()
  {
    return  *( int* )PopBytes( sizeof( int ) );
  }

  RenderHandle BackendCmdDeserializer::PopRenderHandle()
  {
    const int n = mData->mRenderHandles.size();
    TAC_ASSERT_INDEX( mHandleIdx, n );
    return mData->mRenderHandles[ mHandleIdx++ ];
  }

  CommandType2 BackendCmdDeserializer::PopCommandType()
  {
    const int n = mData->mCommands.size();
    TAC_ASSERT_INDEX( mCmdTypeIdx, n );
    return mData->mCommands[ mCmdTypeIdx++ ];
  }

  StringView   BackendCmdDeserializer::PopString()
  {
    const int n = PopInt();
    const auto bytes = ( const char* )PopBytes( n );
    return StringView( bytes, n );
  }

  StackFrame   BackendCmdDeserializer::PopStackFrame()
  {
    StackFrame sf;
    sf.mFile = ( const char* )PopPointer();
    sf.mFunction = ( const char* )PopPointer();
    sf.mLine = PopInt();
    return sf;
  }

  const void*  BackendCmdDeserializer::PopPointer()
  {
    return PopBytes( sizeof( void* ) );
  }

  SmartPtr< UpdateMemory > BackendCmdDeserializer::PopUpdateMemory()
  {
    TAC_ASSERT_INDEX( mUpdateMemIdx, mData->mUpdateMemories.size() );
    return mData->mUpdateMemories[ mUpdateMemIdx++ ];
  }

  DynBufUpdateParams BackendCmdDeserializer::PopDynBufUpdateParams()
  {
    DynBufUpdateParams params;
    params.mHandle.Assign( PopRenderHandle() );
    params.mByteOffset = PopInt();
    params.mUpdateMemory = PopUpdateMemory();
    return params;
  }

  SetRenderObjectNameParams BackendCmdDeserializer::PopSetRenderObjectNameParams()
  {
    SetRenderObjectNameParams params;
    params.mHandle = PopRenderHandle();
    params.mName = PopString();
    return params;
  }

  DynBufCreateParams BackendCmdDeserializer::PopDynBufCreateParams()
  {
    DynBufCreateParams params;
    params.mHandle.Assign( PopRenderHandle() );
    params.mByteCount = PopInt();
    params.mStackFrame = PopStackFrame();
    return params;
  }

} // namespace Tac::Render

