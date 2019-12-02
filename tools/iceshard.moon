import Application from require "ice.application"

--[[ Built-In Commands ]]
import UpdateCommand from require "ice.command.update"
import BuildCommand from require "ice.command.build"
import CleanCommand from require "ice.command.clean"
import GenerateProjectsCommand from require "ice.command.generate_projects"

--[[ Application definition ]]
class IceShard extends Application
    @name: 'iceshard'
    @description: 'IceShard engine project tool.'
    @arguments: { }
    @commands: {
        'build': BuildCommand
        'update': UpdateCommand
        'clean': CleanCommand
        'generate': GenerateProjectsCommand
    }

    -- Plain call to the application
    execute: (args) =>
        print 'IceShard - v0.1-alpha'
        print ''
        print '> For more options see the -h,--help output.'

--[[ Run the application ]]--
IceShard!\run!
