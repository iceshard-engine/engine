import Command, option from require "ice.command"
import Exec from require "ice.tools.exec"
import Json from require "ice.util.json"

class RunCommand extends Command
    @arguments {
        option 'scenario_file',
            name: '-f --file'
            default: 'scenarios.json'
        option 'scenario',
            name: '-s --scenario'
    }

    prepare: (args, project) =>
        @current_dir = os.cwd!
        if run_scenarios = io.open args.file, 'r'
            scenario_file = Json\decode run_scenarios\read '*a'
            selected_scenario_name = args.scenario or scenario_file.default

            -- Get the selected scenario
            @scenario = scenario_file.scenarios[selected_scenario_name]

            -- Prepare all steps
            for step in *@scenario
                result = step.executable\gsub "%%([a-zA-Z_%-]+)%%", (match) ->
                    (os.getenv match) or error "Enviroment variable #{match} not found!"
                step.executable = result\gsub "\\", '/'

            -- Close the file
            run_scenarios\close!

    execute: (args) =>
        if not @scenario
            print "No valid scenario was selected! Provided scenario name: #{args.scenario}"
            return false

        else
            os.chdir @current_dir

            for step in *@scenario
                app_path = step.executable
                if step.working_dir
                    os.chdir "#{@current_dir}/#{step.working_dir}"

                if not os.isfile app_path
                    app_path = @current_dir .. '/' .. step.executable

                if step.use_start_cmd
                    app_path = "start " .. app_path

                --print "#{app_path} #{table.concat step.options, ' '}"
                os.execute "\"#{app_path}\" #{table.concat step.options, ' '}"

            return true

{ :RunCommand }
