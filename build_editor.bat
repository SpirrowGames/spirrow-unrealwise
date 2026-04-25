@echo off
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" MCPGameProjectEditor Win64 Development -Project="C:\Users\owner\Documents\Unreal Projects\spirrow-unrealwise\MCPGameProject\MCPGameProject.uproject" -WaitMutex -FromMsBuild
exit /B %ERRORLEVEL%
