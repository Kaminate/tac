#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacPreprocessor.h"
namespace Tac
{
	struct Errors
	{
		enum Flags
		{
			kNone = 0b0000,
			kDebugBreakOnAppend = 0b0001,
			// Append stack frame?
			// Append messages?
		};
		Errors( Flags flags = kNone );
		operator bool() const;
		bool                 size() const;
		bool                 empty() const;
		void                 clear();
		String               ToString() const;
		void                 Append( const StackFrame& );
		void                 Append( const StringView& );
		void                 OnAppend();
		String               mMessage;
		Vector< StackFrame > mFrames;
		Flags                mFlags;
		bool                 mBroken;
	};

#define TAC_RAISE_ERROR( msg, errors )                     { errors.Append( msg ); errors.Append( TAC_STACK_FRAME ); return; }
#define TAC_RAISE_ERROR_RETURN( msg, errors, returnValue ) { errors.Append( msg ); errors.Append( TAC_STACK_FRAME ); return returnValue; }
#define TAC_HANDLE_ERROR( errors )                                   if( errors ){ errors.Append( TAC_STACK_FRAME ); return; }
#define TAC_HANDLE_ERROR_RETURN( errors, returnValue )               if( errors ){ errors.Append( TAC_STACK_FRAME ); return returnValue; }
#define TAC_HANDLE_ERROR_IF( pred, msg, errors )                     if( pred ){ TAC_RAISE_ERROR( msg, errors ); }
#define TAC_HANDLE_ERROR_IF_REUTRN( pred, msg, errors, returnValue ) if( pred ){ TAC_RAISE_ERROR_RETURN( msg, errors, returnValue ); }

}
