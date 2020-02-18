import Command, option, flag from require "ice.command"
import Conan from require "ice.tools.conan"

class InstallCommand extends Command
    @conan = Conan!

    @arguments {
        flag "update",
            name: "--update"
            default: false

    }

    execute: (args) =>
        @@conan\install conanfile:'tools', update:args.update, install_folder:'build/tools'
        @@conan\install conanfile:'source', update:args.update, install_folder:'build'

{ :InstallCommand }
