@echo off
REM Builds MCPGameProjectEditor. The project path is derived from this script's
REM location (%~dp0) so the checkout can live anywhere; the previous hardcoded
REM absolute path only worked on one machine.
REM Override the engine with: set UE_ROOT=C:\Program Files\Epic Games\UE_5.8
REM Keep this file ASCII-only - cmd.exe misparses non-ASCII bytes in REM lines.
if "%UE_ROOT%"=="" set UE_ROOT=C:\Program Files\Epic Games\UE_5.7

"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" MCPGameProjectEditor Win64 Development -Project="%~dp0MCPGameProject\MCPGameProject.uproject" -WaitMutex -FromMsBuild
exit /B %ERRORLEVEL%
