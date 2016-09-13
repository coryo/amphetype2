@ECHO OFF
SET CWD=%cd%
SET OLDPATH=%PATH%

SET CMAKE_DIR=C:\Program Files\CMake\bin
SET QT_DIR=D:\Qt\5.7\msvc2015

SET PATH=%PATH%;%QT_DIR%\bin;%CMAKE_DIR%


CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

if not exist build md build
cd build || GOTO done
cmake .. -GNinja || GOTO done
ninja || GOTO done
GOTO done
REM msbuild amphetype2.sln /p:Configuration=Release

:done
    cd %CWD%
    SET PATH=%OLDPATH%
