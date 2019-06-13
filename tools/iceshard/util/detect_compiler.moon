-- Read line if file exists
read_line = (file) ->
    result = nil
    if file
        result = file\read '*l'
        file\close!
    result


-- Get Windows 10 SDK information
get_win10_sdk = ->
    -- Helper function to ask for registry keys
    get_registry_key = (root, key, value_type) ->
        result = nil
        if f = io.popen "reg query \"#{root}\\Microsoft\\Microsoft SDKs\\Windows\\v10.0\" /v #{key}"
            -- Check the lines
            for line in f\lines!
                if result = line\match "#{key}[%s]+#{value_type}[%s]+(.+)"
                    break
            f\close!
        result

    -- Result table
    result = directory:nil, version:nil
    for root in *{ 'HKLM\\SOFTWARE\\Wow6432Node', 'HKCU\\SOFTWARE\\Wow6432Node', 'HKLM\\SOFTWARE', 'HKCU\\SOFTWARE' }
        result.directory = get_registry_key root, 'InstallationFolder', 'REG_SZ'
        result.version = get_registry_key root, 'ProductVersion', 'REG_SZ'
        if result.directory and result.version
            break

    result if result.directory and result.version


-- Get the windows 10 universal CRT information
get_universal_crt = ->
    -- Helper function to ask for registry keys
    get_registry_key = (root, key, value_type) ->
        result = nil
        if f = io.popen "reg query \"#{root}\\Microsoft\\Windows Kits\\Installed Roots\" /v #{key}"
            -- Check the lines
            for line in f\lines!
                if result = line\match "#{key}[%s]+#{value_type}[%s]+(.+)"
                    break
            f\close!
        result

    -- Result table
    result = nil
    for root in *{ 'HKLM\\SOFTWARE\\Wow6432Node', 'HKCU\\SOFTWARE\\Wow6432Node', 'HKLM\\SOFTWARE', 'HKCU\\SOFTWARE' }
        if result = get_registry_key root, 'KitsRoot10', 'REG_SZ'
            break
    result


-- Functions used to detect the right compiler on the right host
detect_visual_studio_compiler = ->
    visual_studio_installer = 'C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer'

    -- Check for the vswhere executable
    return false unless os.isdir visual_studio_installer
    return false unless os.isfile visual_studio_installer..'\\vswhere.exe'

    vs_installation_path = nil

    -- Get the visual studio installation
    vs_installation_path = read_line io.popen "\"#{visual_studio_installer}\\vswhere.exe\" -format value -property installationPath -version [15.9,17.0)"

    return false unless os.isfile "#{vs_installation_path}\\VC\\Auxiliary\\Build\\Microsoft.VCToolsVersion.default.txt"
    return false unless os.isfile "#{vs_installation_path}\\VC\\Auxiliary\\Build\\Microsoft.VCRedistVersion.default.txt"

    -- Get the tools and redist versions
    vc_tools_version = read_line io.open "#{vs_installation_path}\\VC\\Auxiliary\\Build\\Microsoft.VCToolsVersion.default.txt", 'r'
    vc_redist_version = read_line io.open "#{vs_installation_path}\\VC\\Auxiliary\\Build\\Microsoft.VCRedistVersion.default.txt", 'r'

    -- Get the windows sdk
    win10_sdk = get_win10_sdk!
    return false unless win10_sdk

    win10_ucrt = get_universal_crt!
    return false unless win10_ucrt

    -- Results
    {
        name: 'msvc'
        vsInstallationDir: vs_installation_path
        vcToolsVersion: vc_tools_version
        vcRedistVersion: vc_redist_version
        win10SDKDirectory: win10_sdk.directory
        win10SDKVersion: win10_sdk.version .. '.0'
        win10UCRTDirectory: win10_ucrt
    }


-- Detects a compiler for the given target at the given host
detect_compiler = (host, target) ->
    if host == 'windows'
        detect_visual_studio_compiler! if target == 'windows'



{ :detect_compiler }
