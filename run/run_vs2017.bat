:mylabel


cls
pushd ..
cmake -G"Visual Studio 15 2017 Win64" -B build_vs2017
popd
pause


goto :mylabel