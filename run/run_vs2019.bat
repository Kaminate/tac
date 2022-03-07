:mylabel

cls
pushd ..
cmake -G"Visual Studio 16 2019" -A x64 -B build_vs2019
popd


pause


goto :mylabel
