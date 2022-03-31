# Tac 3D Game Engine
This repository contains the full source code, game assets, and formal documentation for the Tac 3D game engine.

# Repository Structure
```
src/                    C++ source code
assets/                 Runtime loaded by the game (sound, textures, models...)
```

# Getting the source


```
git clone https://github.com/Kaminate/tac.git
cd tac
git submodule init
git submodule update
```

or

```
git clone --recurse-submodules https://github.com/Kaminate/tac.git
```


# Building ( Windows Visual Studio 2019 )

```
pushd run
run_vs2019
popd
pushd build_vs2019
tac.sln
```

# Building ( Linux Fedora 32 Visual Studio Code )

(untested)

```
su
yum check-update
yum install code // install the vs code ide
yum install gdb // install a debugger
yum install cmake-fedora // install build tools
```
(in vs code)
```
install C/C++ extension 
install cmake tools extension
open command palette (ctrl + shift + p)
>CMake: Select a Kit (selected kit will appear on status bar)
>CMake: Select Variant (Debug)
>CMake: Configure (Generates build files using kit and variant)
>CMake: Build

in .vscode/launch.json set
    "program": "${workspaceFolder}/build_unix_makefiles/src/creation/Creation",
in .vscode/settings.json set
    "cmake.buildDirectory": "${workspaceFolder}/build_unix_makefiles"
this is for run/run_unix_makefiles.h
```

# License
(c) 2017 Nathan Park  
This work is liscensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.  
You should have received a copy of the license along with this work.  
If not, see <http://creativecommons.org/licenses/by-nc-sa/4.0/>.  
[![Creative Commons License](assets/byncsa.png)](http://creativecommons.org/licenses/by-nc-sa/4.0/)

