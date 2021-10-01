if ($IsLinux) {
    bash ".\ice.sh" + ($Args -join ' ')
} elseif ($IsWindows) {
    cmd /C ".\ice.bat" + ($Args -join ' ')
}
