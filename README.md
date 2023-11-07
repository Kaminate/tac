# Tac
This repository contains the full source code, game assets, and formal documentation for the Tac 2D game engine.

## Repository Structure
```
assets/                 Runtime loaded by the game (sound, textures, models...)
cmake/                  The directory CMAKE_MODULE_PATH is set to for find_package to find Find<package>.cmake files
run/                    Scripts
src/                    C++ source code
include/                GL stuff
```

## Getting the source


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


## Building ( Windows Visual Studio 2019 )

```
pushd run
run_vs2019
popd
pushd build_vs2019
tac.sln
```

## Building ( Linux Fedora 32 Visual Studio Code )

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

## Building ( MacOS, Visual Studio Code )

```
TODO:
create a tac environment repo?
- that automatically creates a ~/.vimrc that sources the github vimrc?
- that downloads ~/.bash_profile
  which contains an alias for gvim
  which adds cmake to the path
- that sets terminal.app to
  preferences --> profile --> shell --> run command --> source ~/.bash_profile
```

install homebrew package manager
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

install cmake using brew
``` 
brew install cmake
```

install sdl2 using brew. sdl2 is used as the window manager
```
brew install sdl2
```

install gdb
```
brew install gdb
nevermind, the x64 architecture is reqyired
```

install visual studio code 
https://code.visualstudio.com/download


(alternatively?)
```
brew install --cask visual-studio-code
```

```
cd run
sh ./run_unix_makefiles.sh
```

in vscode
```
install extension C/C++ 
install extension C/C++ Extension Pack
install extension CMake
install extension CMake Tools
open command palette (ctrl + shift + p)

>Cmake: Configure (Generates build files using kit and variant)

```

note: if the cmake configure fails with error no cmake at ""
```
>Preference: Open User Settings (JSON)
```
ensure that settings.json contains 
"cmake.cmakePath": "/Applications/CMAKE.app/Contents/bin/cmake"
or wherever your cmake is installed to


in vscode
```
>Tasks: Configure Task
+-> Create tasks.json file from template
+-> Others

this generates task.json
modify it 
```


## License
(c) 2017 Nathan Park  
This work is liscensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.  
You should have received a copy of the license along with this work.  
If not, see <http://creativecommons.org/licenses/by-nc-sa/4.0/>.  
[![Creative Commons License](assets/byncsa.png)](http://creativecommons.org/licenses/by-nc-sa/4.0/)

