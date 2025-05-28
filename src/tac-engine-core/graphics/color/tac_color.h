#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{

  struct XYZ;

  struct DenseSpectrum
  {
    XYZ ToXYZ() const;
    static float InnerProduct(const DenseSpectrum&, const DenseSpectrum& );

    // Inclusive range, wavelength measured in nanometers
    static constexpr int kLambdaMin{ 360 };
    static constexpr int kLambdaMax{ 830 };
    static constexpr int kSampleCount = kLambdaMax - kLambdaMin + 1;
    float mValues[ kSampleCount ]{};
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
    static constexpr float YIntegral = 106.856895f;
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

