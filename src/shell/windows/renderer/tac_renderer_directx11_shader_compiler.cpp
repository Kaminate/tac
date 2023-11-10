#include "src/shell/windows/renderer/tac_renderer_directx11_shader_compiler.h" // self-inc

#include "src/shell/tac_desktop_app.h" // IsMainThread
#include "src/common/system/tac_os.h" // OSDebugPrintLine
#include "src/common/dataprocess/tac_text_parser.h" // ParseData
#include "src/common/core/tac_error_handling.h" // TAC_RAISE_ERROR_RETURN
#include "src/common/graphics/tac_renderer_backend.h" // GetShaderAssetPath

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

    const String s1 = String( StringView( errMsg ).substr( StringView( errMsg ).find_first_of( ')' ) + 3 ) );
    // const String s2 = Filesystem::FilepathToFilename( GetShaderPath( shaderSource ) );
    const String s2 = GetShaderAssetPath( shaderSource );
    const String s3 = va( ":%i ", origLineNumber );
    const String s4 = String( errorLine ).c_str();

    const String result2 = va( "%s\n%s%s%s\n", s1.c_str(), s2.c_str(), s3.c_str(), s4.c_str() );
    return result2;
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

  ID3DBlob* CompileShaderFromString( const ShaderNameStringView& shaderSource,
                                            const StringView& shaderStrOrig,
                                            const StringView& shaderStrFull,
                                            const char* entryPoint,
                                            const char* shaderModel,
                                            Errors& errors )
  {
    TAC_ASSERT( IsMainThread() );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    dwShaderFlags |= IsDebugMode ? D3DCOMPILE_DEBUG : 0;
    dwShaderFlags |= IsDebugMode ? D3DCOMPILE_SKIP_OPTIMIZATION : 0;

    ID3DBlob* pErrorBlob = nullptr;
    ID3DBlob* pBlobOut = nullptr;

    const HRESULT hr = D3DCompile( shaderStrFull.data(),
                                   shaderStrFull.size(),
                                   nullptr,
                                   nullptr, // D3D_SHADER_MACRO* pDefines,
                                   nullptr, // ID3DInclude* pInclude,
                                   entryPoint,
                                   shaderModel,
                                   dwShaderFlags,
                                   0,
                                   &pBlobOut,
                                   &pErrorBlob );

    if( FAILED( hr ) )
    {
      if( IsDebugMode )
      {
        //String shaderPath = GetShaderPath( shaderSource );
        
        OS::OSDebugPrintfLine( "Error loading shader from %s", shaderSource.c_str() );
        ParseData parseData( shaderStrFull.data(), shaderStrFull.size() );
        int lineNumber = 1;
        OS::OSDebugPrintLine(" -----------" );
        while( parseData.GetRemainingByteCount() )
        {
          StringView parseLine = parseData.EatRestOfLine();
          const char* text = va( "line %3i|%.*s", lineNumber++, parseLine.size(), parseLine.data() );
          OS::OSDebugPrintLine( text );
        }
        OS::OSDebugPrintLine(" -----------" );
      }

      const String errMsg = TryImproveShaderErrorMessage( shaderSource,
                                                          shaderStrOrig,
                                                          shaderStrFull,
                                                          ( const char* )pErrorBlob->GetBufferPointer() );
      TAC_RAISE_ERROR_RETURN( errMsg, errors, nullptr );
    }

    return pBlobOut;
  }

} // namespace Tac::Render



