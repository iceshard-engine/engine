@ECHO off
SETLOCAL
PUSHD %~dp0

:: Cretae the build directory
IF NOT EXIST build\ (
    ECHO Preparing the workspace...
    MKDIR build
)

CALL :_find_profile_arg %*

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

:: Check specific args before running
IF "%1" == "init" (
    SHIFT
    CALL :_initialize
    GOTO :_run
    GOTO :_exit
)

IF "%1" == "script" (
    GOTO :_run_mini
)

:: Move to the application
GOTO :_run

:: Find the value of the --conan_profile argument
:_find_profile_arg
IF [%1] EQU [] (
    CALL SET CONAN_PROFILE="default"
    GOTO :_exit
)
IF "%1" == "--conan_profile" (
    CALL SET CONAN_PROFILE="%2"
) ELSE (
    SHIFT
    GOTO :_find_profile_arg
)
GOTO :_exit

:: Initialize the project environment
:_initialize
PUSHD build\tools
conan install ..\..\tools -of . --build=missing --profile %CONAN_PROFILE%
POPD
ECHO Workspace initialized...
GOTO :_exit

:: Application runtime
:_run_mini
CALL build\tools\conanrunenv_mini.bat
CALL "%ICE_SCRIPT%" workspace.moon %*

:: Save this value as it so the call to 'deactivate' wont erase it in some caes
set ERROR_CODE=%ERRORLEVEL%
CALL build\tools\deactivate_conanrun.bat

GOTO :_errcheck

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
