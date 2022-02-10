#!/bin/sh
cd "${0%/*}";
if [ $(uname) = "Darwin" ]; then
  clang++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "filesystem.cpp" -o "filesystem" -std=c++20 -Wno-empty-body;
elif [ $(uname) = "Linux" ]; then
  g++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "filesystem.cpp" -o "filesystem" -std=c++20 -Wno-empty-body -static-libgcc -static-libstdc++;
elif [ $(uname) = "FreeBSD" ]; then
  clang++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "filesystem.cpp" -o "filesystem" -std=c++20 -Wno-empty-body -lc;
elif [ $(uname) = "DragonFly" ]; then
  g++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "filesystem.cpp" -o "filesystem" -std=c++20 -Wno-empty-body -static-libgcc -static-libstdc++ -lc;
elif [ $(uname) = "OpenBSD" ]; then
  clang++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "filesystem.cpp" -o "filesystem" -std=c++20 -Wno-empty-body -lc;
else
  g++ "apifilesystem/filesystem.cpp" "filesystem.cpp" -o "filesystem.exe" -std=c++20 -Wno-empty-body -static-libgcc -static-libstdc++ -static;
fi;