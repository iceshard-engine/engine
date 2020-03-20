import VSWhere from require "ice.tools.vswhere"
import Json from require "ice.util.json"


class Compiler
    new: (@family, @path) =>

class MSVCCompiler extends Compiler
    new: (family, path, @properties) => super family, path


detect_compilers_msvc = (version) ->
    vswhere = VSWhere!
    vswhere\find products:'*', all:true, format:'json', version:version, requires:{
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64'
    }

compiler_msvc_x64_v142 = (gen, tools_path) ->
    gen\compiler
        name:"compiler-msvc-x64-v142"
        executable: "#{tools_path}\\cl.exe"
        extra_files: {
            "#{tools_path}\\c1.dll",
            "#{tools_path}\\c1xx.dll",
            "#{tools_path}\\c2.dll",
            "#{tools_path}\\msobj140.dll",
            "#{tools_path}\\mspdb140.dll",
            "#{tools_path}\\mspdbcore.dll",
            "#{tools_path}\\mspdbsrv.exe",
            "#{tools_path}\\mspft140.dll",
            "#{tools_path}\\msvcp140.dll",
            "#{tools_path}\\atlprov.dll",
            "#{tools_path}\\tbbmalloc.dll",
            "#{tools_path}\\vcruntime140.dll",
            "#{tools_path}\\1033\\mspft140ui.dll",
            "#{tools_path}\\1033\\clui.dll",
        }

    return "compiler-msvc-x64-v142"


detect_compilers = (gen) ->
    toolchain_list = { }
    toolchain_names = { }

    -- Append all MSVC compilers
    for compiler in *detect_compilers_msvc!
        path = compiler.installationPath

        -- Try to enter this directory,
        os.chdir "#{path}/VC", ->
            tools_version = nil
            redist_version = nil

            -- Read the Tools and Redist version files
            os.chdir "Auxiliary/Build", ->
                get_version = (type) ->
                    read_version_file = (file) ->
                        result = nil
                        if f = io.open file, 'r'
                            result = f\read '*l'
                            f\close!
                        result

                    ver = read_version_file "Microsoft.VC#{type}Version.default.txt"
                    ver or read_version_file "Microsoft.VC#{type}Version.default.txt"

                tools_version = get_version 'Tools'
                redist_version = (get_version 'Redist') or tools_version

            -- Check the compiler version against the required version
            if tools_version and tools_version > "14.2" and tools_version < "14.3" -- We require at least v142.4 (> 14.24)

                -- Check for the x64 architecture
                current_dir = os.cwd! -- "#{path}/VC"
                has_arch_x64 = os.isdir "Tools/MSVC/#{tools_version}/bin/Hostx64/x64"

                -- Define MSVC-x64-v142
                if has_arch_x64
                    toolchain_path = "#{current_dir}\\Tools\\MSVC\\#{tools_version}\\bin\\Hostx64\\x64"
                    toolchain_name = "msvc-x64-v142"
                    toolchain_struct = "Toolchain_MSVC_x64_v142"

                    gen\structure toolchain_struct, (gen) ->
                        gen\variables { { 'ToolchainPath', toolchain_path } }
                        gen\line!

                        compiler_name = compiler_msvc_x64_v142 gen, '$ToolchainPath$'
                        gen\line!

                        gen\variables {
                            { 'ToolchainCompilerFamily', 'MSVC' }
                            { 'ToolchainArchitecture', 'x64' }
                            { 'ToolchainToolset', 'v142' }
                            { 'ToolchainCompiler', compiler_name }
                            { 'ToolchainLibrarian', "$ToolchainPath$\\lib.exe" }
                            { 'ToolchainLinker', "$ToolchainPath$\\link.exe" }
                            { 'ToolchainIncludeDirs', {
                                "#{current_dir}\\Tools\\MSVC\\#{tools_version}\\include",
                                "#{current_dir}\\Tools\\MSVC\\#{tools_version}\\atlmfc\\include",
                                "#{current_dir}\\Auxiliary\\VS\\include",
                                "#{current_dir}\\Auxiliary\\VS\\UnitTest\\include",
                            } }
                            { 'ToolchainLibDirs', {
                                "#{current_dir}\\Tools\\MSVC\\#{tools_version}\\lib\\x64",
                                "#{current_dir}\\Tools\\MSVC\\#{tools_version}\\atlmfc\\lib\\x64",
                                "#{current_dir}\\Auxiliary\\VS\\lib\\x64",
                                "#{current_dir}\\Auxiliary\\VS\\UnitTest\\lib",
                            } }
                            { 'ToolchainLibs', {
                                'kernel32',
                                'user32',
                                'gdi32',
                                'winspool',
                                'comdlg32',
                                'advapi32',
                                'shell32',
                                'ole32',
                                'oleaut32',
                                'uuid',
                                'odbc32',
                                'odbccp32',
                                'delayimp',
                            } }
                        }
                    gen\line!

                    table.insert toolchain_list, toolchain_struct
                    table.insert toolchain_names, toolchain_name


    gen\line '.ToolchainList = {'
    gen\indented (gen) ->
        gen\line ".#{value}" for value in *toolchain_list
    gen\line '}'

    gen\line '.ToolchainNames = {'
    gen\indented (gen) ->
        gen\line "'#{value}'" for value in *toolchain_names
    gen\line '}\n'


detect_windows_platform = (gen) ->
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

    sdk_list = { }
    sdk_names = { }

    if win_sdk = get_win10_sdk!
        sdk_name = "Sdk-Windows-10"
        sdk_struct = "Sdk_Windows_10"

        gen\structure sdk_struct, (gen) ->
            gen\variables {
                -- { 'SdkDirectory', win_sdk.directory }
                -- { 'SdkVersion', win_sdk.version }
                { 'SdkIncludeDirs', {
                    "#{win_sdk.directory}Include\\#{win_sdk.version}.0\\ucrt"
                    "#{win_sdk.directory}Include\\#{win_sdk.version}.0\\um"
                    "#{win_sdk.directory}Include\\#{win_sdk.version}.0\\shared"
                    "#{win_sdk.directory}Include\\#{win_sdk.version}.0\\winrt"
                } }
                { 'SdkLibDirs', {
                    "#{win_sdk.directory}Lib\\#{win_sdk.version}.0\\ucrt\\x64"
                    "#{win_sdk.directory}Lib\\#{win_sdk.version}.0\\um\\x64"
                } }
                { 'SdkLibs', {
                } }
            }
        gen\line!

        table.insert sdk_names, sdk_name
        table.insert sdk_list, sdk_struct

    gen\line '.SdkList = {'
    gen\indented (gen) ->
        gen\line ".#{value}" for value in *sdk_list
    gen\line '}'

    gen\line '.SdkNames = {'
    gen\indented (gen) ->
        gen\line "'#{value}'" for value in *sdk_names
    gen\line '}'



detect_platforms = (gen) ->
    detect_windows_platform gen

{ :detect_compilers, :detect_platforms }
