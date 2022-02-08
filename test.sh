#!/bin/sh
cd "${0%/*}";
if [ $(uname) = "Darwin" ]; then
  clang++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "test.cpp" -o "test" -std=c++20 -Wno-empty-body -ObjC++ -DPROCESS_GUIWINDOW_IMPL -framework Cocoa -framework CoreFoundation  -framework CoreGraphics; "./test";
elif [ $(uname) = "Linux" ]; then
  g++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "test.cpp" -o "test" -std=c++20 -Wno-empty-body -static-libgcc -static-libstdc++ -lprocps -lpthread -DPROCESS_GUIWINDOW_IMPL `pkg-config x11 --cflags --libs`; "./test";
elif [ $(uname) = "FreeBSD" ]; then
  clang++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "test.cpp" -o "test" -std=c++20 -Wno-empty-body -lprocstat -lutil -lc -lpthread -DPROCESS_GUIWINDOW_IMPL `pkg-config x11 --cflags --libs`; "./test";
elif [ $(uname) = "DragonFly" ]; then
  g++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "test.cpp" -o "test" -std=c++20 -Wno-empty-body -static-libgcc -static-libstdc++ -lkvm -lc -lpthread -DPROCESS_GUIWINDOW_IMPL `pkg-config x11 --cflags --libs`; "./test";
elif [ $(uname) = "OpenBSD" ]; then
  clang++ "apiprocess/process.cpp" "apifilesystem/filesystem.cpp" "test.cpp" -o "test" -std=c++20 -Wno-empty-body -lkvm -lc -lpthread -DPROCESS_GUIWINDOW_IMPL `pkg-config x11 --cflags --libs`; "./test";
else
  "$MSYSTEM_PREFIX/../msys2_shell.cmd" -defterm -mingw32 -no-start -here -lc "g++ apiprocess/process.cpp -o apiprocess/process32.exe -std=c++20 -Wno-empty-body -static-libgcc -static-libstdc++ -static -m32"; "$MSYSTEM_PREFIX/../msys2_shell.cmd" -defterm -mingw64 -no-start -here -lc "g++ apiprocess/process.cpp -o apiprocess/process64.exe -std=c++20 -Wno-empty-body -static-libgcc -static-libstdc++ -static -m64"; cd "apiprocess"; xxd -i 'process32' | sed 's/\([0-9a-f]\)$/\0, 0x00/' > 'process32.h'; xxd -i 'process64' | sed 's/\([0-9a-f]\)$/\0, 0x00/' > 'process64.h'; cd ".."; "$MSYSTEM_PREFIX/../msys2_shell.cmd" -defterm -mingw32 -no-start -here -lc "g++ apiprocess/process.cpp apifilesystem/filesystem.cpp test.cpp -o test.exe -std=c++20 -Wno-empty-body -static-libgcc -static-libstdc++ -static -DPROCESS_WIN32EXE_INCLUDES -DPROCESS_GUIWINDOW_IMPL -m32"; rm "apiprocess/process32.exe" "apiprocess/process64.exe" "apiprocess/process32.h" "apiprocess/process64.h"; "./test.exe";
fi;
