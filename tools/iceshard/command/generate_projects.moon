import Command, option, flag from require "iceshard.application"
import detect_compiler from require "iceshard.util.detect_compiler"

lfs = require 'lfs'

class GenerateProjectsCommand extends Command
    @description: "Generates project files for the selected IDE"
    @arguments: {
        option {
            name:'-s --build-system'
            description:'The build system for which projects will be generated. Currently only \'fastbuild\' is supported.'
            default:'fastbuild'
            args:1
        }
        flag {
            name:'-r --rebuild',
            description:'Regenerates all project files.'
            default:false,
        }
    }

    -- Build command call
    execute: (args, skip_fastbuild_target) =>
        if compiler = detect_compiler os.host!, os.host! then with compiler
            compiler_attributes = [{ name:k, value:v } for k, v in pairs compiler]
            table.sort compiler_attributes, (a, b) -> a.name < b.name

            -- FASTBuild build system
            if args.build_system == 'fastbuild'
                if file = io.open 'build/compiler_info.bff', 'w+'
                    file\write '// Generated file\n'
                    file\write ".#{name} = '#{value}'\n" for {:name, :value} in *compiler_attributes
                    file\close!

            -- Run conan in the build directory
            current_dir = lfs.currentdir!
            if lfs.chdir "build"

                if args.rebuild or not os.isfile 'conan.bff'
                    os.execute "conan install ../source"

                -- Run fastbuilds 'solution' target
                unless skip_fastbuild_target
                    os.execute "fbuild -config ../source/fbuild.bff solution"

                lfs.chdir current_dir



{ :GenerateProjectsCommand }
