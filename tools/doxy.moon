import Command, group, argument, option, flag from require "ice.command"
import Path, File, Dir from require "ice.core.fs"
import Setting from require "ice.settings"
import Exec, Where from require "ice.tools.exec"
import Json from require "ice.util.json"

import Validation from require "ice.core.validation"
import Log from require "ice.core.logger"

class DoxyCommand extends Command
    @settings {
        Setting 'doxy.path'
        Setting 'doxy.config'
    }
    @arguments {
        group 'pass-through', description: 'Doxygen pass-through options and flags'
        argument 'config-file',
            group: 'pass-through'
            description: 'The config file to be used when generating documentation. Default can be set in the settings.json file.'
            name: 'config-file'
            default: Setting\ref 'doxy.config'
        flag 'generate',
            group: 'pass-through'
            description: 'Generates a new configuration file.'
            name: '-g --generate'
        flag 'update',
            group: 'pass-through'
            description: 'Updates the current configuration file without performing the generation step.'
            name: '-u --update'
        flag 'build',
            group: 'pass-through'
            description: 'Builds the documentation using the given configuration file.'
            name: '-b --build'
    }

    -- TODO: Add this feature to IBT (all commands by default)
    init: (cmd) =>
        desc = "Settings:"

        for setting in *@@.settings_list
            desc ..= "\n   " .. "'#{setting.path}.#{setting.name}'"
            desc ..= " = #{setting.value}" if setting.value
            desc ..= " = <missing>" if setting.value == nil
            if setting.properties.default
                desc ..= "\n      - default value: #{setting.properties.default}"

        cmd\description desc


    prepare: (args, project) =>
        possible_paths = {
            Where\path Setting\get "doxy.path", -- Resolve the path in the settings
            os.env.DOXY_PATH -- Get the path from the env
        }

        for possible_doxy_path in *possible_paths
            @doxy = Exec possible_doxy_path if File\exists possible_doxy_path

        @fail "Missing a valid 'doxygen' executable! Please check your settings file and/or your enviroment variables."

    execute: (args, project) =>
        Validation\assert (args.update and args.generate) != true, "It's not supported to generate and update doxygen config file at the same time."

        if args.generate or args.update or  args.build
            cmd = ""
            if args.update
                cmd ..= " -u"
            elseif args.generate
                cmd ..= " -g"
            cmd ..= " " .. args['config-file']
            @doxy\run cmd

        else
            os.execute 'start build/doxy/html/index.html'


{ :DoxyCommand }
