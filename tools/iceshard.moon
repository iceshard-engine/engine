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
import LintCommand from require 'ice.commands.lint'
import SDKCommand from require 'ice.commands.sdk'

import RunCommand from require 'tools.run'
import NatvisCommand from require 'tools.natvis'
import L10NCommand from require 'tools.l10n'
import DocsCommand from require 'tools.docs'

LicenseCommand.settings.authors = "Dandielo <dandielo@iceshard.net>"
LicenseCommand.settings.license = "MIT"

class IceShard extends Application
    @name: 'IceShard'
    @description: 'IceShard engine project tool.'
    @commands: {
        -- Development
        'devenv': DevenvCommand
        'build': BuildCommand
        'lint': LintCommand
        -- Platform specific commands
        'android': AndroidCommand
        'webasm': WebAsmCommand
        'sdk': SDKCommand
        -- Additional commands
        'update': UpdateCommand
        'settings': SettingsCommand
        'license': LicenseCommand
        'script': ScriptCommand
        'exec': ExecCommand
        -- Custom commands
        'natvis': NatvisCommand
        'l10n': L10NCommand
        'docs': DocsCommand
    }

{ :IceShard }
