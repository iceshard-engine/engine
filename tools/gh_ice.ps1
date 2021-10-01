$ScriptDirectory = Split-Path $MyInvocation.MyCommand.Path -Parent

if ($IsLinux) {
    $ScriptCall = "$ScriptDirectory/../ice.sh" + ($Args -join ' ')
    bash "$ScriptCall"
} elseif ($IsWindows) {
    $ScriptCall = "$ScriptDirectory/../ice.bat" + ($Args -join ' ')
    cmd /C "$ScriptCall"
}
