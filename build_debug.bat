@echo off
echo ============================================
echo  Building TSFileEditor (Debug, MSVC2019 32bit)
echo ============================================

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
if %ERRORLEVEL% neq 0 (
    echo Failed to initialize MSVC environment!
    pause
    exit /b 1
)

cd /d c:\Work\Kinto\TsFileEditor\build\Desktop_Qt_5_15_2_MSVC2019_32bit-Debug

echo.
echo Running qmake...
C:\Qt\Qt5.15.2\5.15.2\msvc2019\bin\qmake.exe c:\Work\Kinto\TsFileEditor\TSFileEditor.pro -spec win32-msvc CONFIG+=debug
if %ERRORLEVEL% neq 0 (
    echo qmake failed!
    pause
    exit /b 1
)

echo.
echo Running nmake...
nmake
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ============================================
echo  Build succeeded!
echo  Output: c:\Work\Kinto\TsFileEditor\bin\TSFileEditor.exe
echo ============================================
pause
