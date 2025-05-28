#pragma once

namespace Tac
{
  constexpr int kLambdaMinInclusiveNanometers{ 360 };
  constexpr int kLambdaMaxInclusiveNanometers{ 830 };

  struct DenseSpectrum
  {
    float mValues[ kLambdaMaxInclusiveNanometers - kLambdaMinInclusiveNanometers + 1 ]{};
  };

  // CIE 1931 color space
  struct XYZ
  {
    float x{};
    float y{};
    float z{};

    static const DenseSpectrum X;
    static const DenseSpectrum Y;
    static const DenseSpectrum Z;
  };

  struct Blackbody
  {
    struct Params { float mLambaWavelengthNanometers; float mTemperatureInKelvin; };
    Blackbody( Params );
    static DenseSpectrum TemperatureToSpectrum(float temperatureInKelvin);
    operator float() { return mRadiance; }
    float mRadiance{};
  };
}

