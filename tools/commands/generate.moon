import Command, option, flag from require "ice.command"
import FastBuild from require "ice.tools.fastbuild"
import FastBuildGenerator from require "ice.generators.fastbuild"
import Conan from require "ice.tools.conan"

import detect_compilers, detect_platforms from require "tools.utils.compilers"

class GenerateCommand extends Command
    @conan = Conan!
    @fbuild = FastBuild!

    @arguments {
        flag "force-update-deps"
            name: '-f --force-update-deps'
    }

    execute: (args) =>
        workspace_root = os.cwd!

        if args.force_update_deps or not os.isfile "build/tools/conaninfo.txt"
            @@conan\install conanfile:'tools', update:args.update, install_folder:'build/tools'

        if args.force_update_deps or not os.isfile "build/conaninfo.txt"
            @@conan\install conanfile:'source', update:args.update, install_folder:'build'

        os.mkdir "build" unless os.isdir "build"
        os.chdir "build", ->
            unless os.isfile "detected_toolsets.bff"
                gen = FastBuildGenerator "detected_toolsets.bff"
                detect_compilers gen
                detect_platforms gen
                gen\close!

            unless os.isfile "fbuild.bff"
                gen = FastBuildGenerator "fbuild.bff"

                gen\variables { { 'WorkspaceRoot', workspace_root } }
                gen\line!
                gen\include "conan.bff"
                gen\include "detected_toolsets.bff"
                gen\include "#{workspace_root}\\source\\fbuild.bff"
                gen\close!

        @@fbuild\build config:'build/fbuild.bff', target:'solution', summary:false

{ :GenerateCommand }
