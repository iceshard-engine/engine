import Application from require 'ice.application'

import UpdateCommand from require 'ice.commands.update'
import BuildCommand from require 'ice.commands.build'
import VStudioCommand from require 'ice.commands.vstudio'
import RunCommand from require 'tools.run'
import NatvisCommand from require 'tools.natvis'
import CodeStyleCommand from require 'tools.codestyle'

class IceShard extends Application
    @name: 'IceShard'
    @description: 'IceShard engine project tool.'
    @commands: {
        'build': BuildCommand
        'update': UpdateCommand
        'vstudio': VStudioCommand
        'run': RunCommand
        'natvis': NatvisCommand
        'codestyle': CodeStyleCommand
    }

    -- Plain call to the application
    execute: (args) =>
        print "#{@@name} project tool - v0.1-alpha"
        print ' For more options see the -h,--help output.'

{ :IceShard }
