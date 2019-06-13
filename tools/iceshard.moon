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
        'generate-projects': GenerateProjectsCommand
    }

    -- Plain call to the application
    execute: (args) =>


--[[ Run the application ]]--
with IceShard!
    \run!
