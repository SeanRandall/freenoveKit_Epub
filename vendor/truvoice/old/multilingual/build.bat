@echo off
REM Rebuilds speak.exe from source. Requires VC98 (Visual C++ 6) installed at
REM the path below - matches the era of the SAPI 4.0a SDK these headers came
REM from, so ABI/name-mangling lines up with anything linked against it.
call "C:\Program Files (x86)\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
cd /d "%~dp0"
cl.exe /nologo /W3 /EHsc /D_CRT_SECURE_NO_WARNINGS /I. main.cpp audioout.cpp audioin.cpp ctools.cpp /link /OUT:speak.exe user32.lib gdi32.lib ole32.lib oleaut32.lib winmm.lib advapi32.lib uuid.lib
if exist speak.exe copy /Y speak.exe ..\speak.exe
