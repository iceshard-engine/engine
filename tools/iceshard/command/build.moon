import Command, option, flag from require "iceshard.application"
import GenerateProjectsCommand from require "iceshard.command.generate_projects"

class BuildCommand extends GenerateProjectsCommand
    @description: "Builds the engine in the Release configuration."
    @arguments: {
        option {
            name:'-t --target'
            description:'The target which should be build.'
            default:'all-x64-ReleaseDebug'
            args:1
        }
        flag {
            name:'-c --clean'
            description:'Runs a clean build.'
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
            os.execute "fbuild -config ../source/fbuild.bff #{args.target} #{(args.rebuild or args.clean) and '-clean' or ''}"

            lfs.chdir current_dir



{ :BuildCommand }
