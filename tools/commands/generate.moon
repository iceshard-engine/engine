import Command, option, flag from require "ice.command"
import FastBuild from require "ice.tools.fastbuild"
import FastBuildGenerator from require "ice.generators.fastbuild"

class GenerateCommand extends Command
    @fbuild = FastBuild!

    execute: (args) =>
        workspace_root = os.cwd!

        os.mkdir "build" unless os.isdir "build"
        os.chdir "build", ->
            unless os.isfile 'fbuild.bff'
                gen = FastBuildGenerator 'fbuild.bff'
                gen\variables { 'WorkspaceRoot', workspace_root }
                gen\include "conan.bff"
                gen\include "#{workspace_root}\\source\\fbuild.bff"
                gen\close!

        @@fbuild\build config:'build/fbuild.bff', target:'solution', summary:false

{ :GenerateCommand }
