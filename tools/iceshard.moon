--[[ Hooks ]]
package.moonpath ..= ";tools\\?.moon;tools\\?\\init.moon"

--[[ Application ]]
import Application from require "iceshard.application"

--[[ Commands ]]
import BuildCommand from require "iceshard.command.build"
import GenerateProjectsCommand from require "iceshard.command.generate_projects"

--[[ Application definition ]]
class IceShard extends Application
    @name: 'iceshard'
    @description: 'IceShard engine project tool.'
    @arguments: { }
    @commands: {
        'build': BuildCommand
        'generate': GenerateProjectsCommand
    }

    -- Plain call to the application
    execute: (args) =>
        print 'IceShard - v0.1-alpha'
        print ''
        print '> For more options see the -h,--help output.'

--[[ Run the application ]]--
with IceShard!
    \run!
