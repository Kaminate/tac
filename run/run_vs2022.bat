cls

:: -------------------------------------------------------------------------------------------------
:: Call vcvars64.bat to set $ENV{WindowsSdkVerBinPath} for cmake
SETLOCAL
CALL "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
:: -------------------------------------------------------------------------------------------------

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
