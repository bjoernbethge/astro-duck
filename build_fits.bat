@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d D:\projects\synapticore-io\astro-duck
make release
if errorlevel 1 (echo BUILD FAILED & exit /b 1)
echo BUILD SUCCESS
