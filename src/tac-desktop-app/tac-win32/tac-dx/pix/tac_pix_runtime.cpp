#include "tac_pix_runtime.h" // self-inc

/*
  https://developercommunity.visualstudio.com/t/Compiler-bug-with-import-std-throwing-/10706221

  After upgrading from Visual Studio Community 2022 from 17.9.6 to 17.10.4,
  I started getting bogus compile errors about overloading sse intrinsics.
  I narrowed it down to import std being before or after my #include WinPixEventRuntime/pix3.h.
  A sample project that demonstrates this issue is at https://github.com/Kaminate/repro_pix
  and summary of code is below:

  ```cpp
  import std; // causes error
  #include WinPixEventRuntime/pix3.h
  ```

  ```cpp
  #include WinPixEventRuntime/pix3.h
  import std; // this is fine
  ```

  I downloaded Visual Studio Community 2022 Preview, version 17.11.0 Preview 4.0,
  which has no issue with either ordering. Note that this project uses C++23
  StandardLibraryModules <BuildStlModules> in its property sheet.  As a workaround,
  I am not including pix3.h, and instead just getting the proc address directly.

  ```cpp
  const HMODULE module { LoadLibraryA( "WinPixEventRuntime.dll" ) };
  sBeginEventOnCommandList = ( PixBeginEventSig )GetProcAddress( module, "PIXBeginEventOnCommandList" );
  sEndEventOnCommandList   = ( PixEndEventSig )GetProcAddress( module, "PIXEndEventOnCommandList" );
  sSetMarkerOnCommandList  = ( PixSetMarkerSig )GetProcAddress( module, "PIXSetMarkerOnCommandList" );
  ```
*/

#ifndef TAC_PIX_NUGET
#error pix runtime is not available
#endif

#define TAC_IS_COMPILER_BROKEN() true
#if TAC_IS_COMPILER_BROKEN()
using PixBeginEventSig = void( WINAPI* )( ID3D12GraphicsCommandList*, UINT64, _In_ PCSTR );
using PixEndEventSig = void( WINAPI* )( ID3D12GraphicsCommandList* );
using PixSetMarkerSig = void( WINAPI* )( ID3D12GraphicsCommandList*, UINT64, _In_ PCSTR );
#else
#include <WinPixEventRuntime/pix3.h>
#endif

namespace Tac::Render
{

  static bool             sInitialized;
#if TAC_IS_COMPILER_BROKEN()
  static PixBeginEventSig sBeginEvent{};
  static PixEndEventSig   sEndEvent{};
  static PixSetMarkerSig  sSetMarker{};
  static UINT64           sDefaultColor{};
#endif

  // -----------------------------------------------------------------------------------------------

  void PixRuntimeApi::Init( Errors& errors )
  {
      if( sInitialized  )
        return;

#if TAC_IS_COMPILER_BROKEN()
      const HMODULE hModule { LoadLibraryA( "WinPixEventRuntime.dll" ) };
      TAC_RAISE_ERROR_IF( !hModule,  "Failed to load WinPixEventRuntime.dll");
      sBeginEvent = ( PixBeginEventSig )GetProcAddress( hModule, "PIXBeginEventOnCommandList" );
      sEndEvent   = ( PixEndEventSig )GetProcAddress( hModule, "PIXEndEventOnCommandList" );
      sSetMarker  = ( PixSetMarkerSig )GetProcAddress( hModule, "PIXSetMarkerOnCommandList" );
#endif

      sInitialized = true;
  }

  void PixRuntimeApi::BeginEvent( ID3D12GraphicsCommandList* commandList, StringView str )
  {
    TAC_ASSERT( sInitialized );
#if TAC_IS_COMPILER_BROKEN()
      sBeginEvent(commandList, sDefaultColor, str );
#else
      PIXBeginEvent( commandList, PIX_COLOR_DEFAULT, str );
#endif
  }

  void PixRuntimeApi::EndEvent( ID3D12GraphicsCommandList* commandList )
  {
    TAC_ASSERT( sInitialized );
#if TAC_IS_COMPILER_BROKEN()
      sEndEvent(commandList);
#else
      PIXEndEvent(commandList);
#endif
  }

  void PixRuntimeApi::SetMarker( ID3D12GraphicsCommandList* commandList, StringView str )
  {
    TAC_ASSERT( sInitialized );
#if TAC_IS_COMPILER_BROKEN()
    sSetMarker( commandList, sDefaultColor, str );
#else
    PIXSetMarker( commandList, PIX_COLOR_DEFAULT, str );
#endif
  }

} // namespace Tac::Render



