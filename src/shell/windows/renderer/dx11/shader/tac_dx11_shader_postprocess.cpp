#include "tac_dx11_shader_postprocess.h" // self-inc

#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "src/shell/windows/renderer/dx11/tac_renderer_dx11.h"
#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // IsSingleLineCommented

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

  struct Postprocesser
  {
    void PostprocessShaderLine( const StringView& line );

    ConstantBuffers mConstantBuffers;
  };

  void Postprocesser::PostprocessShaderLine( const StringView& line)
  {
    if( IsSingleLineCommented( line ) )
      return;

    ParseData parseData( line.begin(), line.end() );
    parseData.EatWhitespace();
    if( !parseData.EatStringExpected( "cbuffer" ) )
      return;

    RendererDirectX11* renderer = RendererDirectX11::GetInstance();
    const StringView cbufname = parseData.EatWord();
    const ConstantBufferHandle constantBufferHandle = renderer->FindCbufferOfName( cbufname );
    const int parsedBinding = ParseBinding( line );
    const int predictedBinding = mConstantBuffers.size();
    TAC_ASSERT( constantBufferHandle.IsValid() );
    TAC_ASSERT( parsedBinding == predictedBinding );
    mConstantBuffers.push_back( constantBufferHandle );
  }

  ConstantBuffers PostprocessShaderSource( const StringView& shaderStr )
  {
    Postprocesser postprocessor;

    ParseData parseData( shaderStr.data(), shaderStr.size() );
    while( parseData.GetRemainingByteCount() )
      postprocessor.PostprocessShaderLine( parseData.EatRestOfLine() );

    return postprocessor.mConstantBuffers;
  }

} // namespace Tac::Render



