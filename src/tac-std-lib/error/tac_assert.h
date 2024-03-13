#pragma once

#include "tac-std-lib/error/tac_stack_frame.h"

namespace Tac
{
  struct AssertFns
  {
    //using Callback = void ( * )( const char*, StackFrame );
    //static void SetCallback( Callback );
    static void HandleAssert( const char*, StackFrame );
    static void HandleAssertCase( const char*, int, StackFrame );
    static void HandleAssertIndex( unsigned, unsigned, StackFrame );
  };
}

#define TAC_ASSERT_CRITICAL( msg )     Tac::AssertFns::HandleAssert( msg, TAC_STACK_FRAME )
#define TAC_ASSERT( exp )              if( !( exp ) ) TAC_ASSERT_CRITICAL( #exp )
#define TAC_ASSERT_INVALID_CODE_PATH   TAC_ASSERT_CRITICAL( "Invalid code path!"  )
#define TAC_ASSERT_UNIMPLEMENTED       TAC_ASSERT_CRITICAL( "Unimplemented!"  )
#define TAC_ASSERT_MSG( exp, msg )     if( !( exp ) ) TAC_ASSERT_CRITICAL( msg )
#define TAC_ASSERT_RANGE( foos )       for( const auto& foo : foos ) TAC_ASSERT( foo )
#define TAC_ASSERT_INVALID_CASE( v )   Tac::AssertFns::HandleAssertCase( #v, ( int )v, TAC_STACK_FRAME )
#define TAC_ASSERT_INDEX_AUX( ui, un ) if( !( ui < un ) ) Tac::AssertFns::HandleAssertIndex( ui, un, TAC_STACK_FRAME )
#define TAC_ASSERT_INDEX( i, n )       TAC_ASSERT_INDEX_AUX( ( unsigned )i , ( unsigned )n )

