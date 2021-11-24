import Command, option, flag from require "ice.command"
import Exec, Where from require "ice.tools.exec"
import Json from require "ice.util.json"

class Uncrustify extends Exec
    new: (path) => super path or (os.iswindows and Where\path "uncrustify.exe") or Where\path "uncrustify"

    run: (args) =>
        return false unless os.isfile args.input

        cmd = ""
        cmd ..= " -q" if args.quiet
        cmd ..= " -c #{args.config}" if os.isfile args.config
        cmd ..= " #{args.input}" if args.mode == 'check'
        cmd ..= " --check" if args.mode == 'check'

        cmd ..= " -f #{args.input}" if args.mode == 'diff' or args.mode == 'replace'
        cmd ..= " -o #{args.output}" if args.mode == 'diff'

        cmd ..= " --if-changed" if args.mode == 'diff'
        cmd ..= " --replace" if args.mode == 'replace'

        cmd ..= " > nul 2>&1" if args.mode == 'check' and args.quiet
        (super cmd) == 0

class CodeStyleCommand extends Command
    @uncrustify = Uncrustify!

    @arguments {
        option 'path',
            name: '-p --path'
            default: './source/code'
        option 'mode',
            name: '-m --mode'
            default: 'check'
            choices: { 'check', 'diff', 'replace' }
    }

    prepare: (args, project) =>
        @style_file = os.cwd! .. '/source/code_style.cfg'
        os.chdir args.path

    execute: (args) =>
        for_each_source_file = (path, fn) ->
            os.chdir path, (dir) ->
                for entry in os.listdir dir
                    continue if entry == '.' or entry == '..'

                    if (os.isfile entry) and entry\match '%.[hc]xx'
                        fn "#{dir}\\#{entry}" if os.isfile entry

                    elseif os.isdir entry
                        for_each_source_file entry, fn

        for_each_source_file '.', (file) ->
            out_file = file .. ".fixed"

            result = @@uncrustify\run
                config: @style_file
                input: file
                output: out_file
                mode: args.mode
                quiet: true

            if args.mode == 'check' and not result
                print "Invalid code style in file: #{file}" unless result

            elseif args.mode == 'diff'
                if os.isfile out_file
                    os.execute "fc.exe #{file} #{out_file}"
                    os.remove out_file

        -- Powershell example on achieving simmilar results
        -- Get-ChildItem -Path . -Recurse -File -Filter "*.hxx" | ForEach-Object {
        --     $Success = .\uncrustify.exe -c .\source\code_style.cfg $_.FullName --check
        --     if($? -eq $false) {
        --         $TempFile = $_.FullName + ".fixed"
        --         .\uncrustify.exe -c .\source\code_style.cfg -f $_.FullName -o $TempFile
        --         Compare-Object (Get-Content $_.FullName) (Get-Content $TempFile)
        --         Remove-Item -Path $TempFile
        --     }
        -- }

{ :CodeStyleCommand }
