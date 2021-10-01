$ScriptDirectory = Split-Path $MyInvocation.MyCommand.Path -Parent

if ($IsLinux) {
    $ScriptArgs = ($Args -join ' ')
    bash "$ScriptDirectory/../ice.sh" $ScriptArgs
} elseif ($IsWindows) {
    $ScriptArgs = ($Args -join ' ')
    cmd /C "$ScriptDirectory/../ice.bat" $ScriptArgs
}
