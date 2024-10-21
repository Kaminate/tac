#include "MaterialTypes.hlsl"
#include "InputLayout.hlsli"

ConstantBuffer< MaterialParams >    sMaterialParams    : TAC_AUTO_REGISTER;
ConstantBuffer< ShaderGraphParams > sShaderGraphParams : TAC_AUTO_REGISTER;
ConstantBuffer< PerFrameParams >    sPerFrameParams    : TAC_AUTO_REGISTER;
ByteAddressBuffer                   sBuffers[]         : register( t0, space0 );




