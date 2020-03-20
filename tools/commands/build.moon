import Command, option, flag from require "ice.command"
import FastBuild from require "ice.tools.fastbuild"
import FastBuildGenerator from require "ice.generators.fastbuild"
import detect_compilers, detect_platforms from require "tools.utils.compilers"

class BuildCommand extends Command
    @fbuild = FastBuild!

    @arguments {
        flag "clean",
            name:'-c --clean'
        option "target",
            name:'-t --target',
            default:'all-x64-Develop'
    }

    execute: (args) =>

        workspace_root = os.cwd!\gsub '\\', '/'

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
                gen\include "#{workspace_root}/source/fbuild.bff"
                gen\close!

            @@fbuild\build config:'fbuild.bff', target:args.target, clean:args.clean, monitor:true, distributed:true, summary:false

{ :BuildCommand }
