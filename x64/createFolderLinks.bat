@echo off
mklink /J ".\Debug\shaders\" "..\src\shaders\"
mklink /J ".\Release\shaders\" "..\src\shaders\"
pause
