#!/bin/sh
cd "${0%/*}";
if [ $(uname) = "Darwin" ]; then
  clang++ "findhardlinks.cpp" -o "findhardlinks" -std=c++20 -DUSE_GHC_FILESYSTEM -Wno-empty-body; "./findhardlinks";
elif [ $(uname) = "Linux" ]; then
  g++ "findhardlinks.cpp" -o "findhardlinks" -std=c++20 -DUSE_GHC_FILESYSTEM -Wno-empty-body -static-libgcc -static-libstdc++ -lpthread; "./findhardlinks";
elif [ $(uname) = "FreeBSD" ]; then
  clang++ "findhardlinks.cpp" -o "findhardlinks" -std=c++20 -DUSE_GHC_FILESYSTEM -Wno-empty-body -lpthread; "./findhardlinks";
elif [ $(uname) = "DragonFly" ]; then
  g++ "findhardlinks.cpp" -o "findhardlinks" -std=c++20 -DUSE_GHC_FILESYSTEM -Wno-empty-body -static-libgcc -static-libstdc++ -lpthread; "./findhardlinks";
elif [ $(uname) = "OpenBSD" ]; then
  clang++ "findhardlinks.cpp" -o "findhardlinks" -std=c++20 -DUSE_GHC_FILESYSTEM -Wno-empty-body -lpthread; "./findhardlinks";
else
  g++ "findhardlinks.cpp" -o "findhardlinks.exe" -std=c++20 -DUSE_GHC_FILESYSTEM -Wno-empty-body -static-libgcc -static-libstdc++ -static; "./findhardlinks.exe";
fi;
