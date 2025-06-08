@ECHO off
SETLOCAL
PUSHD %~dp0

:: Cretae the build directory
IF NOT EXIST build\ (
    ECHO Preparing the workspace...
    MKDIR build
)

:: Create the build\tools directory
IF NOT EXIST build\tools\ (
    ECHO Preparing tools...
    MKDIR build\tools\
    CALL :_initialize
)

IF NOT EXIST build\tools\conanrun.bat (
    ECHO Rebuilding enviroment...
    CALL :_initialize
)

:: Move to the application
GOTO :_run

:: Initialize the project environment
:_initialize
PUSHD build\tools
conan install ..\..\tools -of . --build=missing --profile default
POPD
ECHO Workspace initialized...
GOTO :_exit

:_run
CALL build\tools\conanrun.bat
CALL "%ICE_SCRIPT%" workspace.moon %*
:: Save this value as it so the call to 'deactivate' wont erase it in some caes
set ERROR_CODE=%ERRORLEVEL%
CALL build\tools\deactivate_conanrun.bat

:_errcheck
:: Check the command return code
IF "%ERROR_CODE%" == "0" (
    GOTO :_exit
)
GOTO :_error

POPD

:_exit
exit /B 0

:_error
exit /B 1
