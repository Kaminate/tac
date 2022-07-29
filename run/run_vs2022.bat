:mylabel

cls
pushd ..
cmake -G"Visual Studio 17 2022" -A x64 -B build_vs2022
popd


pause


goto :mylabel
