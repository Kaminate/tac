while true
do

  clear
  pushd ..

  # You may need to add this line in .vscode/settings.json
  #   "cmake.buildDirectory": "${workspaceFolder}/build_unix_makefiles"
  cmake -G"Unix Makefiles" -B build_unix_makefiles

  popd

  read -p "Press [Enter] key to continue..."
done

