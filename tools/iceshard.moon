import Application from require "ice.application"

import InstallCommand from require "tools.commands.install"
import BuildCommand from require "tools.commands.build"
import GenerateCommand from require "tools.commands.generate"

class IceShard extends Application
    @name: 'iceshard'
    @description: 'IceShard engine project tool.'
    @commands: {
        'build': BuildCommand
        'install': InstallCommand
        -- 'clean': CleanCommand
        'generate': GenerateCommand
    }

    -- Plain call to the application
    execute: (args) =>
        print 'IceShard - v0.1-alpha'
        print ''
        print '> For more options see the -h,--help output.'

IceShard!\run!
