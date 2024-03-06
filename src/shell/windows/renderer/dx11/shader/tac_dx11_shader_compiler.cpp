#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_compiler.h" // self-inc

#include "src/shell/tac_desktop_app.h" // IsMainThread
#include "src/shell/tac_desktop_app_threads.h"

#include "src/common/assetmanagers/tac_asset.h" // AssetPathStringView
#include "src/common/system/tac_os.h" // OSDebugPrintLine
#include "src/common/dataprocess/tac_text_parser.h" // ParseData
#include "src/common/error/tac_error_handling.h" // TAC_RAISE_ERROR_RETURN
#include "src/common/graphics/renderer/tac_renderer_backend.h" // GetShaderAssetPath
#include "src/common/memory/tac_frame_memory.h"

#include <d3dcompiler.h> // D3DCompile

namespace Tac::Render
{

  static String TryImproveShaderErrorMessageLine( const ShaderNameStringView& shaderSource,
                                                  const StringView shaderStrOrig,
                                                  const StringView shaderStrFull,
                                                  const StringView errMsg )
  {
    const int lineNumber = [ & ]()
    {
      const int iOpenParen = StringView( errMsg ).find_first_of( '(' );
      const int iCloseParen = StringView( errMsg ).find_first_of( ')' );
      if( iOpenParen == StringView::npos || iCloseParen == StringView::npos )
        return -1;
      return ( int )ParseData( errMsg.data() + iOpenParen + 1,
                               errMsg.data() + iCloseParen ).EatFloat().GetValueOr( -1 );
    }( );

    const StringView errorLine = [ & ]()
    {
      StringView line;
      ParseData parseDataFull( shaderStrFull.data(), shaderStrFull.size() );
      for( int i = 0; i < ( int )lineNumber; ++i )
        line = parseDataFull.EatRestOfLine();
      return line;
    }( );
    if( errorLine.empty() )
      return errMsg;

    const int origLineNumber = [ & ]()
    {
      int curLineNumber = 1;
      ParseData parseDataOrig( shaderStrOrig.data(), shaderStrOrig.size() );
      while( parseDataOrig.GetRemainingByteCount() )
      {
        if( errorLine == parseDataOrig.EatRestOfLine() )
          return curLineNumber;
        curLineNumber++;
      }
      return -1;
    }( );
    if( origLineNumber == -1 )
      return errMsg;

    String result;
    result += errMsg.substr( errMsg.find_first_of( ')' ) + 3 );
    result += '\n';
    result += GetShaderAssetPath( shaderSource );
    result += ":";
    result += Tac::ToString( origLineNumber );
    result += " ";
    result += errorLine;
    result += '\n';
    return result;
  }

  static String TryImproveShaderErrorMessage( const ShaderNameStringView& shaderSource,
                                              const StringView& shaderStrOrig,
                                              const StringView& shaderStrFull,
                                              const char* errMsg )
  {
    String result;
    ParseData errMsgParse( StringView( errMsg ).begin(),
                           StringView( errMsg ).end() );
    while( errMsgParse.GetRemainingByteCount() )
    {
      const StringView errMsgLine = errMsgParse.EatRestOfLine();
      if( !errMsgLine.empty() )
        result += TryImproveShaderErrorMessageLine( shaderSource, shaderStrOrig, shaderStrFull, errMsgLine ) + "\n";
    }

    return result;

  }

  static void PrintShaderToOutput(const StringView& shaderStrFull)
  {
        ParseData parseData( shaderStrFull.data(), shaderStrFull.size() );
        int lineNumber = 1;
        OS::OSDebugPrintLine(" -----------" );
        while( parseData.GetRemainingByteCount() )
        {
          const StringView parseLine = parseData.EatRestOfLine();

          String lineNumberStr = Tac::ToString( lineNumber );
          while( lineNumberStr.size() < 3 )
            lineNumberStr += " ";

          OS::OSDebugPrintLine( String() + "line " + lineNumberStr + "|" + parseLine );
          lineNumber++;
        }
        OS::OSDebugPrintLine(" -----------" );
  }

  PCom<ID3DBlob> CompileShaderFromString( const ShaderNameStringView& shaderName,
                                            const StringView& shaderStrOrig,
                                            const StringView& shaderStrFull,
                                            const StringView& entryPoint,
                                            const StringView& shaderModel,
                                            Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    dwShaderFlags |= IsDebugMode ? D3DCOMPILE_DEBUG : 0;
    dwShaderFlags |= IsDebugMode ? D3DCOMPILE_SKIP_OPTIMIZATION : 0;

    PCom<ID3DBlob> errorBlob;
    PCom<ID3DBlob> shaderBlob;


    TAC_ASSERT( shaderModel.size() == 6 ); // ie "vs_5_0"
    const String shaderModelVer = String() + shaderModel[ 3 ] + '.' + shaderModel[ 5 ];
    TAC_ASSERT_MSG( StrCmp( shaderModelVer, "5.1" ) <= 0,
                    String() + "fxc cannot compile a shader model " + shaderModelVer +
                    " use dxc instead" );

    const HRESULT hr = D3DCompile( shaderStrFull.data(),
                                   shaderStrFull.size(),
                                   nullptr,
                                   nullptr, // D3D_SHADER_MACRO* pDefines,
                                   nullptr, // ID3DInclude* pInclude,
                                   entryPoint,
                                   shaderModel,
                                   dwShaderFlags,
                                   0,
                                   shaderBlob.CreateAddress(),
                                   errorBlob.CreateAddress() );

    if( FAILED( hr ) )
    {
      if constexpr( IsDebugMode )
      {
        OS::OSDebugPrintLine( String() + "Error loading shader from " + shaderName );

        PrintShaderToOutput( shaderStrFull );
      }

      const auto errorBlobStr = ( const char* )errorBlob->GetBufferPointer();
      const String errMsg = TryImproveShaderErrorMessage( shaderName,
                                                          shaderStrOrig,
                                                          shaderStrFull,
                                                          errorBlobStr );
      TAC_RAISE_ERROR_RETURN( errMsg, {} );
    }

    return shaderBlob;
  }

} // namespace Tac::Render



