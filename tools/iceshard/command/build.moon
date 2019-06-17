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
        flag {
            name:'-v --verbose'
            description:'Runs the build commands in verbose mode'
            default:false
        }
    }

    -- Build command call
    execute: (args) =>
        -- Generate projects first
        super args, true

        current_dir = lfs.currentdir!
        if lfs.chdir "build"
            additonal_arguments = ""
            additonal_arguments ..= " -verbose" if args.verbose
            additonal_arguments ..= " -clean" if args.rebuild or args.clean


            -- Run fastbuild with the right target
            os.execute "fbuild -config ../source/fbuild.bff #{args.target} #{additonal_arguments}"

            lfs.chdir current_dir



{ :BuildCommand }
