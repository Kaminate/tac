#include "tacExampleFluid.h"
#include "src/common/tacPreprocessor.h" // C4100

// maybe broken up into
// fluid_cpu,
// fluid_gpu,
// fluid_renderer,
// fluid_debug

namespace Tac
{
  enum class BoundaryType
  {
    Dirichlet
  };

  typedef float( *Fn2D )( float, float );

  struct BoundaryFunctions
  {
    // [ ] Q: how does this work, the first param is x or y and the second param is t?
    Fn2D boundaryX0;
    Fn2D boundaryX1;
    Fn2D boundaryY0;
    Fn2D boundaryY1;
  };

  BoundaryFunctions bf;

  // globals

  float x0, y0, x1, y1; // rect bounds
  int imax, jmax; // lattice bounds
  float deltaT; // time step ( positive )
  float deltaX = ( x1 - x0 ) / imax; // cell x-dimension
  float deltaY = ( y1 - y0 ) / jmax; // cell x-dimension

  // initial values
  float RhoT0( float x, float y );

  // dirichlet boundary conditions
  float RhoX0( float y, float t ); // phi0
  float RhoX1( float y, float t ); // phi1
  float RhoY0( float y, float t ); // psi0
  float RhoY1( float y, float t ); // psi1

  // locals
  float kappa; // density viscosity constant ( positive )
  float lambdaX = ( kappa * deltaT ) / ( deltaX * deltaX );
  float lambdaY = ( kappa * deltaT ) / ( deltaY * deltaY );
  BoundaryType type;
  float* x; // lattice cell center x
  float* y; // lattice cell center y
  float* rhoOld; // density, old time
  float* rhoNew; // density, new time
  float t = 0;

  void SetBoundaryFunctions( BoundaryType t )
  {
    BoundaryFunctions dirichletBoundary;
    dirichletBoundary.boundaryX0 = RhoX0;
    dirichletBoundary.boundaryX1 = RhoX1;
    dirichletBoundary.boundaryY0 = RhoY0;
    dirichletBoundary.boundaryY1 = RhoY1;

    if( t == BoundaryType::Dirichlet )
    {
      type = t;
      bf = dirichletBoundary;
    }
    else
    {
      TAC_CRITICAL_ERROR_UNIMPLEMENTED;
    }
  }

  void InitializeExplicit( float* x, float* y, Fn2D initT0, float** sOld )
  {
    for( int i = 0; i < imax; ++i )
    {
      x[ i ] = x0 + deltaX * i;
      for( int j = 0; j < jmax; ++j )
      {
        y[ j ] = y0 + deltaY * j;
        sOld[ j + i * imax ] = initT0( x[ i ], y[ j ] );
      }

    }

  }

  void UpdateBoundary( float t,
                       int* x,
                       int* y,
                       BoundaryFunctions bf,
                       float* sNew )
  {
    TAC_ASSERT( type == BoundaryType::Dirichlet );
    // edges
    // [ ] Q: why does this not just cover the corner?
    for( int j = 1; j < jmax; ++j )
    {
      sNew[ j ][ 0 ] = bf.boundaryX0( y[j],t);
      sNew[ j ][ imax ] = bf.boundaryX1( y[j],t);
    }
    for( int i = 1; i < imax; ++i )
    {
      sNew[ 0 ][ i ] = bf.boundaryY0( x[ i ], t );
      sNew[ jmax ][ i ] = bf.boundaryY1( x[ i ], t );
    }

    // corners
    // [ ] Q: we could use the boundaryY0 function here also, no?
    sNew[ 0 ][ 0 ] = bf.boundaryX0( y[ 0 ], t );
    sNew[ 0 ][ imax ] = bf.boundaryX1( y[ 0 ], t );
    sNew[ jmax ][ 0 ] = bf.boundaryX0( y[ jmax ], t );
    sNew[ jmax ][ imax ] = bf.boundaryX1( y[ jmax ], t );

  }

  void UpdateDiffusionExplicit( float t,
                                float* x,
                                float* y,
                                float lambdaX,
                                float lambdaY,
                                BoundaryFunctions bf,
                                float* sOld,
                                float* sNew )
  {
    // update interior
    for( int j = 1; j < jmax; ++j )
    {
      for( int i = 1; i < imax; ++i )
      {
        // equation (5.91)
        sNew[ ... ]
          = sOld[ ... ]
          + lambdaX * ( sOld[ j, i + 1 ] - 2 sOld[ i, j ] + sOld[ i - 1, j ] )
          + lambdaY * ( sOld[ j + 1, i ] - 2 sOld[ i, j ] + sOld[ i, j - 1 ] )
      }

      UpdateBoundary( t, x, y, bf, sNew );
    }
  }

  void ExampleFluid::Init( Errors& errors )
  {
    SetBoundaryFunctions( BoundaryType::Dirichlet );
    InitializeExplicit( x, y, RhoT0, rhoOld );

  }

  void ExampleFluid::Update( Errors& errors )
  {
    UpdateDiffusionExplicit( t, x, y, lambdaX, lambdaY, bf, rhoOld, rhoNew );
    SwapBuffers( &rhoOld, &rhoNew );
    t = t + deltaT;
  }

  void ExampleFluid::Uninit( Errors& errors )
  {

  }

} // namespace Tac
