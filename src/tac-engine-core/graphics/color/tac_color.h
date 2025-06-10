#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{

  struct AbsoluteXYZ;

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
  template< int N >
  struct RelativeXYZ
  {
    float x{};
    float y{}; // represents a luminance factor
    float z{};
    const float* data() const { return &x; }
  };

  using RelativeXYZ1 = RelativeXYZ< 1 >;
  using RelativeXYZ100 = RelativeXYZ< 100 >;

  struct Blackbody
  {
    struct Wavelength{ float mNanometers; };
    struct Temperature{ float mKelvins; };
    Blackbody( Wavelength, Temperature );
    static DenseSpectrum TemperatureToSpectrum( Temperature );
    operator float() { return mRadiance; }
    float mRadiance{};
  };

  // -----------------------------------------------------------------------------------------------

  struct Linear_sRGB;
  struct Linear_scRGB;
  struct Encoded_sRGB;
  struct Encoded_scRGB;

  struct Linear_sRGB
  {
    Linear_sRGB();
    Linear_sRGB( const Encoded_sRGB& );
    Linear_sRGB( float, float, float );
    float r {};
    float g {};
    float b {};
  };

  // [0,1]
  struct Encoded_sRGB
  {
    Encoded_sRGB() = default;
    Encoded_sRGB( const Linear_sRGB& );
    float r {};
    float g {};
    float b {};
  };

  // -----------------------------------------------------------------------------------------------

  // In sRGB, the white point's luminance is 80 cd/m^2
  // D65/2 white point
  struct Linear_scRGB
  {
    Linear_scRGB() = default;
    Linear_scRGB( v3 );
    Linear_scRGB( float, float, float );
    Linear_scRGB( float );
    Linear_scRGB( const AbsoluteXYZ& );
    float r {};
    float g {};
    float b {};
  };

  struct Encoded_scRGB
  {
    float r {};
    float g {};
    float b {};
  };

  // -----------------------------------------------------------------------------------------------

  struct sRGBHelpers
  {
    // transfer "gamma" functions
    static float EncodedToLinear( float );
    static float LinearToEncoded( float );
  };


}

