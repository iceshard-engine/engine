import Application from require 'ice.application'

import UpdateCommand from require 'ice.commands.update'
import BuildCommand from require 'ice.commands.build'
import DevenvCommand from require 'ice.commands.devenv'
import LicenseCommand from require 'ice.commands.license'
import ScriptCommand from require 'ice.commands.script'
import ExecCommand from require 'ice.commands.exec'
import AndroidCommand from require 'ice.commands.android'
import WebAsmCommand from require 'ice.commands.webasm'
import SettingsCommand from require 'ice.commands.settings'

import RunCommand from require 'tools.run'
import NatvisCommand from require 'tools.natvis'
import DoxyCommand from require 'tools.doxy'

LicenseCommand.settings.authors = "Dandielo <dandielo@iceshard.net>"
LicenseCommand.settings.license = "MIT"

class IceShard extends Application
    @name: 'IceShard'
    @description: 'IceShard engine project tool.'
    @commands: {
        'build': BuildCommand
        'update': UpdateCommand
        'devenv': DevenvCommand
        'exec': ExecCommand
        'codestyle': CodeStyleCommand
        'license': LicenseCommand
        'script': ScriptCommand
        'android': AndroidCommand
        'webasm': WebAsmCommand
        'settings': SettingsCommand
        -- Custom commands
        'natvis': NatvisCommand
        'doxy': DoxyCommand
    }

{ :IceShard }
