@echo off
cd /d "F:\repos\Tractor3D\Tractor3Dlib"

echo Formatting all C++ files...

for /r %%f in (*.cpp *.h *.hpp) do (
    set "filename=%%~nxf"
    if /i not "!filename!"=="pch.h" if /i not "!filename!"=="PlatformWindows.cpp" (
        echo Formatting: %%f
        "F:\Program Files\LLVM\bin\clang-format.exe" -i "%%f"
	)
)

echo Done!
pause
