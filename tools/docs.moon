import Command, group, argument, option, flag from require "ice.command"
import Path, File, Dir from require "ice.core.fs"
import Setting from require "ice.settings"
import Exec, Where from require "ice.tools.exec"
import Json from require "ice.util.json"

import Validation from require "ice.core.validation"
import Log from require "ice.core.logger"

class DocsCommand extends Command
    @settings {
        Setting 'docs.doxygen.config_template', default:'source/configs/doxyfile.template'
        Setting 'docs.doxygen.config_output', default:'build/doxygen'
    }

    @arguments {
        group 'general', description: 'General options and commands'
        argument 'tool',
            group: 'general'
            description: 'The tool for which utility commands will be executed.'
            name: 'tool'
            choices: { 'doxygen' }
            default: 'doxygen'

        group 'doxygen', description: 'Doxygen related options and commands'
        option 'generate',
            group: 'doxygen'
            description: 'Generates a final configuration file based on the input template.'
            name: '-g --generate'
            default: Setting\ref 'docs.doxygen.config_template'
        option 'output',
            group: 'doxygen'
            description: 'Updates the current configuration file without performing the generation step.'
            name: '-o --output'
            default: Setting\ref 'docs.doxygen.config_output'
    }

    -- TODO: Add this feature to IBT (all commands by default)
    -- init: (cmd) =>
    --     desc = "Settings:"

    --     for setting in *@@.settings_list
    --         desc ..= "\n   " .. "'#{setting.path}.#{setting.name}'"
    --         desc ..= " = #{setting.value}" if setting.value
    --         desc ..= " = <missing>" if setting.value == nil
    --         if setting.properties.default
    --             desc ..= "\n      - default value: #{setting.properties.default}"

    --     cmd\description desc


    prepare: (args, project) =>
        execute_fn = @["_execute_#{args.tool}"]
        @log\debug "Executing sub-command '#{args.tool}'..."
        execute_fn @, args, project if (type execute_fn) == "function"

    _execute_doxygen: (args, project) =>
        @fail "Missing doxygen config template '#{args.generate}' to generate the final config from." unless File\exists args.generate

        @log\info "Loading Doxygen configuration template from: #{args.generate}"
        template = File\load args.generate

        -- TODO: Configure the input paths?


        -- Replacements
        replacements = {
            ['OutputDirectory']: args.output
            ['CategoryFolders']: {}
            ['PublicFolders']: {}
        }

        -- Fine all source folders
        for category, type in Dir\list "source/code", recursive:false
            continue unless type == "directory"

            @log\verbose "Found new category folder 'source/code/#{category}'..."
            table.insert replacements.CategoryFolders, "source/code/#{category}"

            -- Project
            for project, type in Dir\list "source/code/#{category}", recursive:false
                continue unless type == "directory"

                -- Special case
                if project == "public" -- or folder == "private"
                    @log\verbose "Found new public source folder 'source/code/#{category}'..."
                    table.insert replacements.PublicFolders, "source/code/#{category}/#{project}"
                    continue

                -- Sources
                for folder, path in Dir\list "source/code/#{category}/#{project}", recursive:false
                    if folder == "public" -- or folder == "private"
                        @log\verbose "Found new public source folder 'source/code/#{category}'..."
                        table.insert replacements.PublicFolders, "source/code/#{category}/#{project}/#{folder}"

        -- Replace all tags
        errors = {}
        config = template\gsub "${(%w*)(%.?%.?%.?)}", (key, templ) ->
            return table.insert errors, "Template key '#{key or ''}' is invalid!" unless key and #key > 0
            return table.insert errors, "Template key '#{key}' missing replacement value!" unless replacements[key]
            if templ == "..."
                return table.concat ["\"#{path}\"" for path in *replacements[key]], ' \\\n                         '
            else
                return replacements[key]

        @log\error error for error in *errors
        @fail "Failing doxygen config generation due to one or more errors!" unless #errors == 0

        config_location = Path.Unix\join project.output_dir, 'doxyfile'
        File\save (Path\join project.workspace_dir, config_location), config
        @log\info "Saved final Doxygen config file at: #{config_location}"

{ :DocsCommand }
