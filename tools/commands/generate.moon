import Command, option, flag from require "ice.command"
import FastBuild from require "ice.tools.fastbuild"
import FastBuildGenerator from require "ice.generators.fastbuild"
import detect_compilers, detect_platforms from require "tools.utils.compilers"

class GenerateCommand extends Command
    @fbuild = FastBuild!

    execute: (args) =>
        workspace_root = os.cwd!

        os.mkdir "build" unless os.isdir "build"
        os.chdir "build", ->
            unless false and os.isfile "detected_toolsets.bff"
                gen = FastBuildGenerator "detected_toolsets.bff"
                detect_compilers gen
                detect_platforms gen
                gen\close!

            unless false and os.isfile "fbuild.bff"
                gen = FastBuildGenerator "fbuild.bff"

                gen\variables { { 'WorkspaceRoot', workspace_root } }
                gen\line!
                gen\include "conan.bff"
                gen\include "detected_toolsets.bff"
                gen\include "#{workspace_root}\\source\\fbuild.bff"
                gen\close!

        @@fbuild\build config:'build/fbuild.bff', target:'solution', summary:false

{ :GenerateCommand }
