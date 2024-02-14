#pragma once

struct ClipSpacePosition3
{
  float3 mFloat3;
  //void operator = (ClipSpacePosition3){ }
};

// https://github.com/microsoft/DirectXShaderCompiler/wiki/HLSL-2021#operator-overloading
// the sample code doesn't compile, because the conversion operator = straight up doesn't work
// https://github.com/microsoft/DirectXShaderCompiler/issues/6081
struct ClipSpacePosition4
{
#if 0
  // Error: Shader compilation failed
  // error: overloading 'operator=' is not allowed
  void operator = (ClipSpacePosition3 ) {}
#endif

#if 0
  // Exception thrown at 0x00007FF92E5BCF19
  // Microsoft C++ exception: hlsl::Exception at memory location 0x0000004AC3FF6150.
  // Error: Shader compilation failed
  // Internal Compiler error: llvm::cast<X>() argument of incompatible type!
  void operator = (ClipSpacePosition4 ) {}
#endif

#if 0
  void operator = (ClipSpacePosition3 xyz) { mFloat4 = float4( xyz.mFloat3, 1.0); }
#endif

#if 0
  void operator = (ClipSpacePosition3 xyz, float w = 1) { mFloat4 = float4( xyz.mFloat3, w); }
#endif

  float4 mFloat4;
};


