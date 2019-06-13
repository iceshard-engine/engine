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

IF NOT EXIST build\tools\activate.bat (
    ECHO Rebuilding enviroment...
    CALL :_initialize
)

:: Check specific args before running
IF "%1" == "init" (
    SHIFT
    CALL :_initialize
    GOTO :_exit
)

:: Move to the application
GOTO :_run


:: Initialize the project environment
:_initialize
PUSHD build\tools
conan install ..\..\tools
POPD
ECHO Workspace initialized...
EXIT /B 0


:: Application runtime
:_run
CALL build\tools\activate.bat
CALL moon tools\iceshard.moon %*
CALL build\tools\deactivate.bat

POPD

:_exit
exit /B 0
