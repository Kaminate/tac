#include "src/shell/windows/renderer/tac_renderer_directx11_shader_postprocess.h" // self-inc
#include "src/common/dataprocess/tac_text_parser.h"

namespace Tac::Render
{
  static int ParseBinding( const StringView& line )
  {
    String digits;
    int iBegin = line.find_first_of( '(' );
    int iEnd = line.find_first_of( ')' );
    TAC_ASSERT( iBegin != line.npos );
    TAC_ASSERT( iEnd != line.npos );
    for( int i = iBegin; i < iEnd; ++i )
    {
      char c = line[ i ];
      if( IsDigit( c ) )
        digits.push_back( c );
    }
    TAC_ASSERT( !digits.empty() );
    return Atoi( digits );
  }

  static void PostprocessShaderLineCbuffer( const StringView& line, ConstantBuffers* constantBuffers )
  {
    ParseData parseData( line.begin(), line.end() );
    parseData.EatWhitespace();
    if( !parseData.EatStringExpected( "cbuffer" ) )
      return;
    RendererDirectX11* renderer = RendererDirectX11::GetInstance();
    const StringView cbufname = parseData.EatWord();
    const ConstantBufferHandle constantBufferHandle = renderer->FindCbufferOfName( cbufname );
    const int parsedBinding = ParseBinding( line );
    const int predictedBinding = constantBuffers->size();
    TAC_ASSERT( constantBufferHandle.IsValid() );
    TAC_ASSERT( parsedBinding == predictedBinding );
    constantBuffers->push_back( constantBufferHandle );
  }

  static void PostprocessShaderLine( const StringView& line, ConstantBuffers* constantBuffers )
  {
    ParseData parseData( line.begin(), line.end() );
    parseData.EatWhitespace();
    if( parseData.EatStringExpected( "//" ) )
      return;

    PostprocessShaderLineCbuffer( line, constantBuffers );
  }

  void PostprocessShaderSource( const StringView& shaderStr, ConstantBuffers* constantBuffers )
  {
    ParseData parseData( shaderStr.data(), shaderStr.size() );
    while( parseData.GetRemainingByteCount() )
      PostprocessShaderLine( parseData.EatRestOfLine(), constantBuffers );
  }

} // namespace Tac::Render



