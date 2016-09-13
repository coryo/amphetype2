@ECHO OFF
SET OLDPATH=%PATH%

SET QTDIR=D:\Qt\5.7\msvc2015

SET BOOST_ROOT=D:\local\boost_1_61_0
SET BOOST_INCLUDEDIR=%BOOST_ROOT%\boost
SET BOOST_LIBRARYDIR=%BOOST_ROOT%\lib32-msvc-14.0

SET PATH=%PATH%;%QTDIR%\bin;C:\Program Files\CMake\bin


CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

if not exist build md build
cd build
cmake ../src -GNinja
ninja
REM msbuild Amphetype2.sln /p:Configuration=Release

copy %QTDIR%\bin\Qt5Core.dll . /Y
copy %QTDIR%\bin\Qt5Gui.dll . /Y
copy %QTDIR%\bin\Qt5Widgets.dll . /Y
copy %QTDIR%\bin\Qt5PrintSupport.dll . /Y

cd ..

SET PATH=%OLDPATH%
