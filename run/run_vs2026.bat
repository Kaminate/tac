cls

:: change directory to that of this file (in case this bat file was called from another path )
:: so the pushd and popd calls later refer to the root tac dir
pushd "%~dp0"

:: loop label for goto
:loop

:: change directory from tac/run to tac/ before running cmake
pushd ..
:: cmake                   -G "Visual Studio 18 2026" -A x64 -B build_vs2026

:: generate project files
:: -WDev enables developer warnings
cmake --log-level=VERBOSE -G "Visual Studio 18 2026" -A x64 -B build_vs2026 -Wdev

:: build the project
:: cmake --build build_vs2026

popd

:: user input triggers each loop iteration
pause

:: cls at end (instead of at beginning) to show vcvars
cls
goto :loop
