@echo off
echo Terminating running instances of Roke and RokeSettings...
taskkill /f /im Roke.exe 2>nul
taskkill /f /im RokeSettings.exe 2>nul

echo Cleaning bin and obj directories...
if exist bin rmdir /s /q bin
if exist obj rmdir /s /q obj

echo Calling VsDevCmd.bat...
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat"

echo Restoring NuGet packages...
msbuild /t:Restore Roke.sln

echo Building solution in Release x64...
msbuild /p:Configuration=Release /p:Platform=x64 Roke.sln
