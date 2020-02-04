import Command, option, flag from require "ice.command"
import FastBuild from require "ice.tools.fastbuild"

class BuildCommand extends Command
    @fbuild = FastBuild!

    @arguments {
        flag "clean",
            name:'-c --clean'
        option "target",
            name:'-t --target',
            default:'all-x64-ReleaseDebug'
    }

    execute: (args) =>

        os.mkdir "build" unless os.isdir "build"
        os.chdir "build", ->
            unless os.isfile 'fbuild.bff'
                gen = FastBuildGenerator 'fbuild.bff'
                gen\variables { 'WorkspaceRoot', workspace_root }
                gen\include "conan.bff"
                gen\include "#{workspace_root}\\source\\fbuild.bff"
                gen\close!

        @@fbuild\build config:'build/fbuild.bff', target:args.target, clean:args.clean, monitor:true, distributed:true

{ :BuildCommand }
