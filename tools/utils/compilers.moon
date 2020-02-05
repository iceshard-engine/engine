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


detect_compilers = (gen) ->
    compilers = { }

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

                    ver = read_version_file "Microsoft.VC#{type}Version.v142.default.txt"
                    ver or read_version_file "Microsoft.VC#{type}Version.default.txt"

                tools_version = get_version 'Tools'
                redist_version = (get_version 'Redist') or tools_version

            -- Check the compiler version against the required version
            if tools_version and tools_version > "14.2" and tools_version < "14.3" -- We require at least v142.4 (> 14.24)

                -- Check for the x64 architecture
                has_arch_x64 = os.isdir "Tools/MSVC/#{tools_version}/bin/Hostx64/x64"
                -- has_msvc_x86 = os.isdir "Tools/MSVC/#{tools_version}/bin/Hostx64/x86"

                -- Append a MSVC Compiler object
                if has_arch_x64
                    gen\line ".AvailableToolsets = { }"

                    gen\structure "ToolsetInfo", (gen) ->
                        gen\variables {
                            { 'Platform', 'Windows' }
                            { 'Architecture', 'x64' }
                            { 'Family', 'MSVC' }
                            { 'Toolset', 'v142' }
                        }
                    gen\line ".AvailableToolsets + .ToolsetInfo"

                    table.insert compilers, MSVCCompiler 'msvc', path, {
                        product: compiler.catalog.productLine
                        architecture: 'x64'
                        toolset: 'v142'
                        :tools_version
                        :redist_version
                    }

{ :detect_compilers }
