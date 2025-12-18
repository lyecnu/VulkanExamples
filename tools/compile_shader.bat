@echo off
setlocal EnableExtensions

rem === Encoding ===
chcp 65001 >nul

rem === Paths ===
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "ROOT_DIR=%%~fI"
set "DEFAULT_SHADER_DIR=%ROOT_DIR%\shaders\glsl"

rem === Args ===
set "INPUT_PATH="
set "TARGET_ENV=vulkan1.3"

:ParseArgs
if "%~1"=="" goto :ArgsDone
set "ARG=%~1"

rem 注意：在 () 块里配合 shift 会遇到 %~1 预展开问题，所以这里用 goto 分支处理参数。
if /I "%ARG%"=="-t" goto :ArgTargetEnvValue
if /I "%ARG%"=="--target-env" goto :ArgTargetEnvValue
if /I "%ARG:~0,13%"=="--target-env=" goto :ArgTargetEnvEquals

if /I "%ARG%"=="-d" goto :ArgDirValue
if /I "%ARG%"=="--dir" goto :ArgDirValue
if /I "%ARG:~0,6%"=="--dir=" goto :ArgDirEquals

echo 错误：未知参数："%ARG%"
call :PrintUsage
exit /b 1

:ArgTargetEnvValue
shift
if "%~1"=="" (
	echo 错误：%ARG% 需要一个参数
	call :PrintUsage
	exit /b 1
)
set "TARGET_ENV=%~1"
shift
goto :ParseArgs

:ArgTargetEnvEquals
set "TARGET_ENV=%ARG:~13%"
shift
goto :ParseArgs

:ArgDirValue
shift
if "%~1"=="" (
	echo 错误：%ARG% 需要一个参数
	call :PrintUsage
	exit /b 1
)
call :SetInputPath "%~1" || exit /b 1
shift
goto :ParseArgs

:ArgDirEquals
set "DIR_VALUE=%ARG:~6%"
for %%I in ("%DIR_VALUE%") do set "DIR_VALUE=%%~I"
call :SetInputPath "%DIR_VALUE%" || exit /b 1
shift
goto :ParseArgs

:ArgsDone
if not "%INPUT_PATH%"=="" set "INPUT_PATH=%INPUT_PATH:/=\%"

call :ResolveTargetDir "%INPUT_PATH%" || exit /b 1
call :FindGlslc || exit /b 1

echo glslc目录："%GLSLC%"
echo 编译目录："%TARGET_DIR%"
echo target-env：%TARGET_ENV%

set /a OK=0,FAIL=0

rem cmd.exe 的 for /r 只稳定支持单个通配符，这里拆成两次遍历。
for /r "%TARGET_DIR%" %%F in (*.vert) do call :CompileOne "%%~fF"
for /r "%TARGET_DIR%" %%F in (*.frag) do call :CompileOne "%%~fF"

echo.
echo 完成。成功：%OK%  失败：%FAIL%
pause
if %FAIL% gtr 0 exit /b 2
exit /b 0

rem === Helpers ===
:SetInputPath
if not "%INPUT_PATH%"=="" (
	echo 错误：目录参数重复："%~1"
	call :PrintUsage
	exit /b 1
)
set "INPUT_PATH=%~1"
exit /b 0

:PrintUsage
echo 用法：compile_shader.bat [--dir DIR ^| -d DIR] [--target-env=ENV ^| --target-env ENV ^| -t ENV]
echo.
echo   ENV 可选：vulkan1.0 vulkan1.1 vulkan1.2 vulkan1.3 vulkan1.4 vulkan opengl4.5 opengl
pause
exit /b 0

rem === Resolve compile target directory ===
rem Priority:
rem   1) If INPUT_PATH is an existing directory, use it.
rem   2) If DEFAULT_SHADER_DIR\INPUT_PATH exists, use it.
rem   3) Otherwise, error.
:ResolveTargetDir
set "TARGET_DIR=%DEFAULT_SHADER_DIR%"
if "%~1"=="" goto :ResolveTargetDirDone

if exist "%~1\" (
	set "TARGET_DIR=%~1"
) else if exist "%DEFAULT_SHADER_DIR%\%~1\" (
	set "TARGET_DIR=%DEFAULT_SHADER_DIR%\%~1"
) else (
	echo 错误：未找到目标目录："%~1"
	echo 用法：compile_shader.bat -d triangle ^| --dir ..\shaders\glsl\triangle
	pause
	exit /b 1
)

:ResolveTargetDirDone
for %%I in ("%TARGET_DIR%") do set "TARGET_DIR=%%~fI"
exit /b 0

rem === Locate glslc ===
:FindGlslc
set "GLSLC=%SCRIPT_DIR%glslc.exe"
if exist "%GLSLC%" exit /b 0

for /f "delims=" %%P in ('where glslc 2^>nul') do (
	set "GLSLC=%%P"
	exit /b 0
)

echo 错误：未找到 glslc（请把 glslc.exe 放到 tools\ 或加入 PATH）
pause
exit /b 1

rem === Compile one file ===
:CompileOne
set "FILE=%~1"
set "FILE_DIR=%~dp1"
if "%FILE_DIR:~-1%"=="\" set "FILE_DIR=%FILE_DIR:~0,-1%"

set "EXT=%~x1"
if /I "%EXT%"==".vert" (
	set "STAGE=vert"
) else if /I "%EXT%"==".frag" (
	set "STAGE=frag"
) else (
	exit /b 0
)

echo [阶段:%STAGE%] "%FILE%"
"%GLSLC%" --target-env=%TARGET_ENV% -fshader-stage=%STAGE% -I "%FILE_DIR%" -I "%DEFAULT_SHADER_DIR%" -o "%FILE%.spv" "%FILE%"
if errorlevel 1 (
	echo 错误：编译失败
	set /a FAIL+=1
) else (
	set /a OK+=1
)
exit /b 0
