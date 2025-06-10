#pragma once


namespace Tac
{
  struct v2;
  struct v3;

#define UseQuat() false


#if UseQuat()
  struct TmpQuat
  {
    float w, x, y, z;
    TmpQuat(const m3&);
    void Normalize();
    void Zero();

    float& operator[]( int );
    float operator[]( int ) const;
  };
#endif

  // Row-major order
  struct m3
  {
    float
      m00, m01, m02,
      m10, m11, m12,
      m20, m21, m22;

#if UseQuat()
    m3(TmpQuat);
#endif

    m3() = default;
    m3( float mm00, float mm01, float mm02,
        float mm10, float mm11, float mm12,
        float mm20, float mm21, float mm22 );
    float*       data();
    const float* data() const;
    float        Determinant() const;
    float&       operator()( int iRow, int iCol );
    float        operator()( int iRow, int iCol ) const;
    float&       operator[]( int );
    float        operator[]( int ) const;
    float        Trace() const;
    v3           GetColumn( int ) const;
    v3           GetRow( int ) const;
    bool         Invert( m3* ) const;

    // return the transpose of the cofactor matrix
    m3           Adjugate() const;

    // returns a matrix where each element is the deteriminant of...
    m3           Cofactor() const;
    void         Transpose();
    void         OrthoNormalize();
    static m3    Transpose( const m3& );
    static m3    FromColumns( const v3&, const v3&, const v3& );
    static m3    FromRows( const v3&, const v3&, const v3& );
    static m3    Identity();
    static m3    Scale( const v3& );
    static m3    RotRadEuler( const v3& );
    static m3    RotRadEulerInv( const v3& );
    static m3    RotRadX( float );
    static m3    RotRadY( float );
    static m3    RotRadZ( float );
    static m3    RotRadAngleAxis( float, const v3& );
    static m3    Translate( const v2& );
    static m3    CrossProduct( float, float, float );
    static m3    CrossProduct( const v3& );
    void operator*= ( float );
    void operator+= ( const m3& );
  };

  m3 operator* ( float, const m3& );
  v3 operator*( const m3&, const v3& );
  m3 operator*( const m3&, const m3& );

  void m3UnitTest();

} // namespace Tac

