cls

:: change directory to that of this file (in case this bat file was called from another path )
:: so the pushd and popd calls later refer to the root tac dir
pushd "%~dp0"

:: -------------------------------------------------------------------------------------------------
:: Running from Developer Command Prompt for VS2022 sets $ENV{WindowsSdkVerBinPath} for cmake
:: -------------------------------------------------------------------------------------------------
IF NOT defined WindowsSdkVerBinPath (
  echo "please run this script from Developer Command Prompt for VS2022"
  pause
  exit /b 1
)

:: loop label for goto
:loop


:: change directory from tac/run to tac/ before running cmake
pushd ..
cmake -G "Visual Studio 17 2022" -A x64 -B build_vs2022

:: uncomment to see message( VERBOSE "...")
::
:: cmake --log-level=VERBOSE -G "Visual Studio 17 2022" -A x64 -B build_vs2022

popd

:: user input triggers each loop iteration
pause

:: cls at end (instead of at beginning) to show vcvars
cls
goto :loop
