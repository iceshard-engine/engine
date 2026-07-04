import Command, group, argument, option, flag from require "ice.command"
import Setting from require "ice.settings"
import Json from require "ice.util.json"
import Path, Dir, File from require "ice.core.fs"

class L10NCommand extends Command
    @settings {
        Setting 'l10n.generated.location', default:'source/data/l10n/strings'
        -- Accent Tool Integration settings
        Setting 'l10n.accent.file_format', default:'java_properties'
        Setting 'l10n.accent.name_pattern', default:'parentDirectory'
    }

    @arguments {
        group 'general', description:"Localization utilities"
        argument 'mode',
            group: 'general'
            description: 'Selects the process that this command should run.\n- \'generate\' updates the built-in l10n values.\n- \'accent\' generates accent.json file to be used with the Accent tool.\n'
            name: 'mode'
            choices: { 'generate', 'accent' }
            default: 'generate'

        group 'accent', description:"Accent integration"
        option 'accent_output',
            description: 'The output location where accent.json should be generated.'
            group: 'accent'
            name: '-o --output'
            default: Setting\ref 'project.output_dir'
    }

    prepare: (args, project) =>
        os.chdir "source/code" if args.mode == 'generate'

    iterate_over_source_files: (path, l10n_keypath, l10n_keys) =>
        return unless path\match "%.[hc]xx"

        prefix = "builtin."
        path = Path\normalize path
        @log\verbose "Searching for l10n keys inside '#{path}' source file..."

        if file = File\open path, mode:'rb'
            lineidx = 0

            for line in file\lines!
                lineidx += 1
                for keypath, keyid, flags_fallback in line\gmatch '"([%w%.%-_]+)/([%w%.%-_]+)([^\"]*)"_i18n'

                    if keypath and keyid
                        @log\debug "Found l10n key '#{keypath}/#{keyid}' in file '#{path}(#{lineidx})'"

                        -- Skip localization keys flagged with 'no-builtin'
                        fallback_message = ""
                        if flags_fallback
                            if (flags_fallback\match "no%-builtin") ~= nil
                                @log\verbose "Skipped l10n key due to 'no-builtin' flag"
                                continue

                            -- Get the fallback message if any
                            fallback_message = flags_fallback\match "|(.+)"

                        -- Ensure that builtin l10n references match the expected keypath prefix
                        unless keypath\match prefix --l10n_keypath
                            @log\warning "Keypath '#{keypath}' found in file '#{path}(#{lineidx})' does not match expected prefix '#{prefix}'!"

                        -- Ensure the keypath table exists
                        if l10n_keys[keypath] == nil
                            l10n_keys[keypath] = { }

                        -- Store the keypath
                        if l10n_keys[keypath][keyid] == nil
                            if fallback_message == nil
                                @log\info "Storing l10n reference '#{keypath}/#{keyid}' without fallback message."
                            else
                                @log\info "Storing l10n reference '#{keypath}/#{keyid}' with '#{fallback_message}' fallback message."

                            l10n_keys[keypath][keyid] = fallback_message or ''

                        elseif fallback_message ~= nil
                            stored_message = l10n_keys[keypath][keyid]

                            if stored_message == ''
                                @log\info "Updating l10n reference '#{keypath}/#{keyid}' with '#{fallback_message}' fallback message."
                                l10n_keys[keypath][keyid] = fallback_message

                            elseif stored_message ~= fallback_message
                                @log\error "Conflict on existing l10n reference '#{keypath}/#{keyid}' with existing fallback message. [stored: '#{stored_message}', found: '#{fallback_message}']"

            file\close!

    execute: (args, project) =>
        execute_fn = @["_execute_#{args.mode}"]
        @log\debug "Executing sub-command '#{args.mode}'..."
        execute_fn @, args, project if (type execute_fn) == "function"

    _execute_generate: (args, project) =>
        locations = { }
        l10n_keys = { }

        for category in Dir\list ".", recursive:false
            continue unless Dir\exists category

            for project in Dir\list category, recursive:false
                subpath = Path\join category, project
                continue unless Dir\exists subpath
                @log\debug "Entering path: #{subpath}"

                l10n_keypath = "builtin.#{category}.#{project}"
                @log\verbose "Searching localization keys inside source files with base key-path: '#{l10n_keypath}'"

                for path in Dir\list subpath, recursive:true
                    @iterate_over_source_files path, l10n_keypath, l10n_keys

        @log\info "Deleting previously generated files..."
        strings_dir = "#{project.workspace_dir}/#{@settings.l10n.generated.location}"
        Dir\delete strings_dir, with_files:true

        @log\info "Creating built-in string tables..."
        for keypath, keyids in pairs l10n_keys

            -- Skip the "a" keypath as it's special
            continue if keypath == "a"

            -- Prepare the directory for the keypath
            output_dir = "#{strings_dir}/#{keypath}/"
            @log\verbose "Creating directory at: #{output_dir}"

            -- Prepare the contents as sorted java properties
            ids_list = [:key, :value for key, value in pairs keyids]
            table.sort ids_list, (v0, v1) -> v0.key < v1.key

            -- Create the file contents
            contents = "# I18N built-in strings\n"
            for {:key, :value} in *ids_list
                contents ..= "#{key}=#{value}\n"

            if Dir\create output_dir
                output_file = Path\join output_dir, "en.isl10n"
                @log\info "Saving collected l10n data in #{output_file}"
                File\save output_file, contents, mode:'wb'

    _execute_accent: (args, project) =>
        -- Create the object that represents Accent settings for the current configuration
        accent_json = {
            files: {
                {
                    namePattern: @settings.l10n.accent.name_pattern
                    format: @settings.l10n.accent.file_format,
                    source: "#{@settings.l10n.generated.location}/**/en.isl10n"
                    target: "#{@settings.l10n.generated.location}/%document_path%/%slug%.isl10n"
                }
            }
        }

        @log\info "Accent configuration written to '%s'", Path\join args.output, "accent.json"
        File\save (Path\join args.output, "accent.json"), (Json\encode accent_json), mode:'wb'

{ :L10NCommand }
