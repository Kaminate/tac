#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{

  struct AbsoluteXYZ;
  struct RelativeXYZ;

  struct DenseSpectrum
  {
    AbsoluteXYZ ToAbsoluteXYZ() const;
    static float InnerProduct(const DenseSpectrum&, const DenseSpectrum& );

    // Inclusive range, wavelength measured in nanometers
    static constexpr int kLambdaMin{ 360 };
    static constexpr int kLambdaMax{ 830 };
    static constexpr int kSampleCount = kLambdaMax - kLambdaMin + 1;
    float mValues[ kSampleCount ]{};
  };

  // CIE 1931 color space
  struct AbsoluteXYZ
  {
    float x{};
    float y{}; // Represents an absolute luminance (cd/m^2)
    float z{};
    const float* data() const { return &x; }

    static const DenseSpectrum X;
    static const DenseSpectrum Y;
    static const DenseSpectrum Z;
    static constexpr float YIntegral = 106.856895f;
  };

  // The three components are divided by the luminance of a given white point.
  // The white point has a luminance factor of 1
  struct RelativeXYZ
  {
    float x{};
    float y{}; // represents a luminance factor
    float z{};
    const float* data() const { return &x; }
  };

  struct Blackbody
  {
    struct Params { float mLambaWavelengthNanometers; float mTemperatureInKelvin; };
    Blackbody( Params );
    static DenseSpectrum TemperatureToSpectrum(float temperatureInKelvin);
    operator float() { return mRadiance; }
    float mRadiance{};
  };

  // In sRGB, the white point's luminance is 80 cd/m^2
  // D65/2 white point
  struct Linear_scRGB
  {
    static Linear_scRGB FromAbsoluteXYZ(AbsoluteXYZ);
    float r {};
    float g {};
    float b {};
  };

  struct Encoded_scRGB
  {
  };
}

