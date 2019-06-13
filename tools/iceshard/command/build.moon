import Command, option, flag from require "iceshard.application"
import GenerateProjectsCommand from require "iceshard.command.generate_projects"

class BuildCommand extends GenerateProjectsCommand
    @description: "Builds the engine in the Release configuration."
    @arguments: {
        option {
            name:'-s --build-system'
            description:'The build system for which projects will be generated. Currently only \'fastbuild\' is supported.'
            default:'fastbuild'
            args:1
        }
        option {
            name:'-t --target'
            description:'The target which should be build.'
            default:'all-x64-ReleaseDebug'
            args:1
        }
        flag {
            name:'-r --rebuild',
            description:'Makes a clean build regenerating project files.'
            default:false,
        }
    }

    -- Build command call
    execute: (args) =>
        -- Generate projects first
        super args, true

        current_dir = lfs.currentdir!
        if lfs.chdir "build"
            -- Run fastbuild with the right target
            os.execute "fbuild -config ../source/fbuild.bff #{args.target} #{args.rebuild and '-clean' or ''}"

            lfs.chdir current_dir



{ :BuildCommand }
