@echo off
setlocal
cd /d "%~dp0"
if not defined VSCMD_ARG_TGT_ARCH call "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat"
if /i not "%VSCMD_ARG_TGT_ARCH%"=="x86" exit /b 1
if not exist .build mkdir .build
cl /nologo /std:c++17 /utf-8 /O2 /MT /EHsc /W4 /Fo.build\ /Fe:URW_QuestTrainer.exe src\QuestTrainer.cpp /link /MACHINE:X86 /SUBSYSTEM:WINDOWS user32.lib gdi32.lib bcrypt.lib shell32.lib
if errorlevel 1 exit /b 1
