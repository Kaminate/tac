#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/math/tac_math.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/core/tac_preprocessor.h" // C4100
#include "src/game-examples/fluid/tac_example_fluid.h"

//#include <cmath>
//#include <iostream>

// maybe broken up into
// fluid_cpu,
// fluid_gpu,
// fluid_renderer,
// fluid_debug

namespace Tac
{
#if 0
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
      sNew[ j ][ 0 ] = bf.boundaryX0( y[ j ], t );
      sNew[ j ][ imax ] = bf.boundaryX1( y[ j ], t );
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
#else

  enum IterationExample
  {
    None,
    FixedPoint,
    GaussSeidel,


  };

  static IterationExample sIterationExample;
  //static String sIterationExampleFn;
  //static float( *sGraphEqFx )( float );
  //static float( *sGraphEqGx )( float );

  typedef float( *Fn2D )( float );
  float Ex1FxEqualsXMinusCosX( float x ) { return x - Cos( x ); }
  float Ex1GxEqualsCosX( float x ) { return Cos( x ); }
  float Ex1YEqualsX( float x ) { return x; }


  struct EquationGrapher
  {
    v2 canvas_pos = {};
    v2 canvas_size = {};
    v2 viewCenter = {};
    v2 viewHalfDims = {};

    // Cached at the start of the frame
    v2 viewMini = { 0, 0 };
    v2 viewMaxi = { 0, 0 };
    v2 viewDims = { 0, 0 };

    float px_per_unit_y = 0;
    float px_per_unit_x = 0;

    const v4 axisColor = v4( 0, 0, 0, 1 );
    const float axisLineRadiusMinor = 0.5f;
    const float axisLineRadiusMajor = 2.0f;

    // Position of the graph's origin, in px, relative to the viewport of the desktop window
    v2 originViewport = { 0, 0 };

    void FrameBegin( v2 in_canvas_pos, v2 in_canvas_size )
    {
      this->canvas_pos = in_canvas_pos;
      this->canvas_size = in_canvas_size;

      // Initial size
      viewHalfDims.x = viewHalfDims.x ? viewHalfDims.x : 5;

      // Aspect
      viewHalfDims.y = viewHalfDims.x * canvas_size.y * ( 1.0f / canvas_size.x );

      // Cache frame variables
      viewMini = viewCenter - viewHalfDims;
      viewMaxi = viewCenter + viewHalfDims;
      viewDims = viewHalfDims * 2;
      px_per_unit_y = canvas_size.y / viewDims.y;
      px_per_unit_x = canvas_size.x / viewDims.x;

      // Position of the graph's origin, in px, relative to the viewport of the desktop window
      originViewport = v2( canvas_pos.x + -viewMini.x * ( 1.0f / viewDims.x ) * canvas_size.x,
                           canvas_pos.y + viewMaxi.y * ( 1.0f / viewDims.y ) * canvas_size.y );
    }

    void DrawVerticalGridLines( UI2DDrawData* drawData )
    {

      const float line_begin_x = 1.0f - ( viewMini.x - ( int )viewMini.x );
      const int line_count_x = ( int )viewDims.x - 1;
      float line_canvas_x = canvas_pos.x + line_begin_x * px_per_unit_x;
      for( int i = 0; i < line_count_x; ++i )
      {
        drawData->AddLine( v2( line_canvas_x, canvas_pos.y ),
                           v2( line_canvas_x, canvas_pos.y ) + v2( 0, canvas_size.y ),
                           axisLineRadiusMinor,
                           axisColor );
        line_canvas_x += px_per_unit_x;
      }
    }

    void DrawHorizontalGridLines( UI2DDrawData* drawData )
    {

      // draw horizontal axis lines
      const float line_begin_y = 1.0f - ( viewMini.y - ( int )viewMini.y );
      const int line_count_y = ( int )viewDims.y - 1;
      float line_canvas_y = canvas_pos.y + canvas_size.y - line_begin_y * px_per_unit_y;
      for( int i = 0; i < line_count_y; ++i )
      {
        drawData->AddLine( v2( canvas_pos.x, line_canvas_y ),
                           v2( canvas_pos.x, line_canvas_y ) + v2( canvas_size.x, 0 ),
                           axisLineRadiusMinor,
                           axisColor );
        line_canvas_y -= px_per_unit_y;
      }
    }

    void DrawYAxis( UI2DDrawData* drawData )
    {

      const bool isYAxisVisible = viewMini.x < 0 && viewMaxi.x > 0;
      if( isYAxisVisible )
        drawData->AddLine( v2( originViewport.x, canvas_pos.y ),
                           v2( originViewport.x, canvas_pos.y ) + v2( 0, canvas_size.y ),
                           axisLineRadiusMajor,
                           axisColor );

    }

    void DrawXAxis( UI2DDrawData* drawData )
    {
      const bool isXAxisVisible = viewMini.y < 0 && viewMaxi.y > 0;
      if( isXAxisVisible )
        drawData->AddLine( v2( canvas_pos.x, originViewport.y ),
                           v2( canvas_pos.x, originViewport.y ) + v2( canvas_size.x, 0 ),
                           axisLineRadiusMajor,
                           axisColor );

    }

    v2   GraphToWindow( v2 p )
    {
      return v2( canvas_pos.x + ( p.x - viewMini.x ) * px_per_unit_x,
                 canvas_pos.y + ( -p.y + viewMaxi.y ) * px_per_unit_y );
    }

    void DrawFn( Fn2D fn, float radius, v4 color, UI2DDrawData* drawData )
    {
      if( !fn )
        return;

      static Vector< v2 > points;
      points.clear();
      for( float x = viewMini.x; x < viewMaxi.x; x += viewDims.x / 100.0f )
      {
        float y = fn( x );
        points.push_back( v2( x, y ) );
      }

      if( points.empty() )
        return;

      v2 pPrev = points[ 0 ];
      auto isPointVisible = [ & ]( v2 p )->bool { return p.y > viewMini.y && p.y < viewMaxi.y; };
      bool prevVisible = isPointVisible( pPrev );
      for( int i = 1; i < points.size(); ++i )
      {
        v2 pCurr = points[ i ];
        bool currVisible = isPointVisible( pCurr );
        if( prevVisible && currVisible )
        {
          v2 from = GraphToWindow( pPrev );
          v2 to = GraphToWindow( pCurr );
          drawData->AddLine( from, to, radius, color );
        }

        pPrev = pCurr;
        prevVisible = currVisible;
      }

    }

    void MouseDrag()
    {

      const DesktopWindowHandle desktopWindowHandle = ImGuiGetWindowHandle();
      if( !IsWindowHovered( desktopWindowHandle ) )
        return;
      const DesktopWindowState* state = GetDesktopWindowState( desktopWindowHandle );
      if( !state )
        return;
      const v2 desktopWindowPos( ( float )state->mX, ( float )state->mY );
      const ImGuiRect canvasRectScreenspace = ImGuiRect::FromPosSize( desktopWindowPos + canvas_pos, canvas_size );
      if( !canvasRectScreenspace.ContainsPoint( KeyboardGetScreenspaceCursorPos() ) )
        return;
      if( KeyboardGetMouseDeltaScroll() )
      {
        this->viewHalfDims.x -= KeyboardGetMouseDeltaScroll() * 0.3f;
      }
      if( !KeyboardIsKeyDown( Key::MouseMiddle ) )
        return;
      const v2 mouseDeltaPos = KeyboardGetMouseDeltaPos();
      viewCenter -= v2( mouseDeltaPos.x / px_per_unit_x,
                        -mouseDeltaPos.y / px_per_unit_y );
    }
  };

  static EquationGrapher sEquationGrapher;

  void ExampleFluid::Update( Errors& errors )
  {
    struct FnDraw
    {
      FnDraw() = default;
      FnDraw( Fn2D fn, v4 color, float lineRadius )
      {
        mFn = fn;
        mColor = color;
        mLineRadius = lineRadius;
      }

      Fn2D  mFn;
      v4    mColor;
      float mLineRadius;
    };

    static Vector< String > texts;
    static Vector< FnDraw > fns;
    const int NULL_STEP = -1;
    static int iStep = NULL_STEP;
    static int iStepNext;
    static int stepCount;


    ImGuiText( "--------------------------------" );
    ImGuiTextf( "(debug text) iStep: %i, stepCount %i", iStep, stepCount );


    //if( sIterationExample != IterationExample::None )
    //{
      //ImGuiText( sIterationExampleFn );

    // Relative to window viewport
    const v2 canvas_pos = ImGuiGetCursorPos();
    const v2 canvas_size = v2( 400, 300 );
    ImGuiImage( ( int )Render::TextureHandle(), canvas_size );
    UI2DDrawData* drawData = ImGuiGetDrawData();

    sEquationGrapher.FrameBegin( canvas_pos, canvas_size );
    sEquationGrapher.DrawVerticalGridLines( drawData );
    sEquationGrapher.DrawHorizontalGridLines( drawData );
    sEquationGrapher.DrawXAxis( drawData );
    sEquationGrapher.DrawYAxis( drawData );
    for( const FnDraw& fn : fns )
      sEquationGrapher.DrawFn( fn.mFn, fn.mLineRadius, fn.mColor, drawData );
    //sEquationGrapher.DrawFn( sGraphEqFx, 1.0f, v4( 1, 0, 0, 1 ), drawData );
    //sEquationGrapher.DrawFn( sGraphEqGx, 1.0f, v4( 0, 1, 0, 1 ), drawData );
    //sEquationGrapher.DrawFn( []( float x ){ return x; }, 0.5f, v4( 0, 0, 0, 1 ), drawData );
    sEquationGrapher.MouseDrag();

    for( auto& text : texts )
    {
      ImGuiText( text );
    }
  //}


  // Next/Prev step ui
    {
      const bool nextAvailable = iStep < stepCount - 1;
      const bool prevAvailable = iStep > 0;
      if( prevAvailable && ( ImGuiButton( "Prev" ) || KeyboardIsKeyJustDown( Key::LeftArrow ) ) )
        iStepNext = iStep - 1;
      if( nextAvailable && prevAvailable )
        ImGuiSameLine();
      if( nextAvailable && ( ImGuiButton( "Next" ) || KeyboardIsKeyJustDown( Key::RightArrow ) ) )
        iStepNext = iStep + 1;
    }

    if( sIterationExample == IterationExample::FixedPoint )
    {
      stepCount = 4;
      if( iStep != iStepNext )
      {
        iStep = iStepNext;

        const Vector< String > textsStep0 = {
              "------------------",
              "Fixed Point Method",
              "------------------",
              "Solve the equation:",
              "x - cos(x) = 0"
        };

        const Vector< FnDraw > fnsStep0 = { FnDraw( Ex1FxEqualsXMinusCosX, v4( 1,0,0,1 ), 1.0f ) };

        const Vector< String > textsStep1 = [ & ]()
        {
          Vector< String > v = textsStep0;
          v.push_back( "Rewrite as" );
          v.push_back( "x = cos(x)" );
          return v;
        }( );

        const Vector< FnDraw > fnsStep1 = [ & ]()
        {
          Vector< FnDraw > v = fnsStep0;
          v.push_back( FnDraw( Ex1GxEqualsCosX, v4( 0, 1, 0, 1 ), 1.0f ) );
          return v;
        }( );

        const Vector< String > textsStep2 = [ & ]()
        {
          Vector< String > u = textsStep1;
          u.insert( u.end(),
                    {
                        "x = g(x)",
                        "The fixed point of g(x) is the point (x,g(x))",
                        "at which input x which equals output g(x)",
                        "",
                        "The series x_n+1 = g( x_n )",
                        "converges via the banach fixed point theorem",
                        "",
                        "If the function is a contraction ( lipschitz type < 1 )",
                        "Then it's a converging cauchy sequence",
                        "x* = lim n -> infinity x_n"
                    } );
          return u;
        }( );

        const Vector< FnDraw > fnsStep2 = [ & ]()
        {
          Vector< FnDraw > v = fnsStep1;
          v.push_back( FnDraw( Ex1YEqualsX, v4( 0, 0, 0, 1 ), 1.0f ) );
          return v;
        }( );

        switch( iStep )
        {
          case 0:
          {
            texts = textsStep0;
            fns = fnsStep0;
          } break;

          case 1:
          {
            texts = textsStep1;
            fns = fnsStep1;
          } break;

          case 2:
          {
            texts = textsStep2;
            fns = fnsStep2;
          } break;

          case 3:
          {
            texts = textsStep2;

            float val = 3;
            Vector< float > vals = { val };
            for( int iter = 0; iter < 20; ++iter )
            {
              val = Ex1GxEqualsCosX( val );
              vals.push_back( val );
            }

            for( int i = 0; i < vals.size(); ++i )
            {
              texts.push_back( va( "iteration %i, x = %f", i, vals[ i ] ) );
            }

            texts.push_back( "This converges to the Dottie number, 0.739085..." );
            fns = fnsStep2;
          } break;
        }
      }
    }

    if( ImGuiCollapsingHeader( "Select Method" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;

      IterationExample prev = sIterationExample;

      if( ImGuiButton( "Fixed-Point Iteration" ) )
        sIterationExample = IterationExample::FixedPoint;

      if( ImGuiButton( "Gauss-Seidel" ) )
        sIterationExample = IterationExample::GaussSeidel;

      if( sIterationExample != prev )
      {
        iStep = NULL_STEP;
        stepCount = 0;
        iStepNext = 0;
        texts.clear();
        fns.clear();
      }
    }

  }


#endif


} // namespace Tac
