:mylabel


cls
mkdir ..\build_vs2017
pushd ..\build_vs2017
cmake ..\ -G"Visual Studio 15 2017 Win64"
popd
pause


goto :mylabel
