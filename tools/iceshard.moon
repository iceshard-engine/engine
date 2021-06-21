import Application from require 'ice.application'

import InstallCommand from require 'ice.commands.install'
import BuildCommand from require 'ice.commands.build'
import VStudioCommand from require 'ice.commands.vstudio'
import RunCommand from require 'tools.run'

class IceShard extends Application
    @name: 'IceShard'
    @description: 'IceShard engine project tool.'
    @commands: {
        'build': BuildCommand
        'install': InstallCommand
        'vstudio': VStudioCommand
        'run': RunCommand
    }

    -- Plain call to the application
    execute: (args) =>
        print "#{@@name} project tool - v0.1-alpha"
        print ' For more options see the -h,--help output.'

{ :IceShard }
