$ScriptDirectory = Split-Path $MyInvocation.MyCommand.Path -Parent

if ($IsLinux) {
    bash "$ScriptDirectory/ice.sh" + ($Args -join ' ')
} elseif ($IsWindows) {
    cmd /C "$ScriptDirectory/ice.bat" + ($Args -join ' ')
}
