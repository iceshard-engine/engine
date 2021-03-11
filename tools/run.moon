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

            -- Close the file
            run_scenarios\close!


        os.chdir project.output_dir

    execute: (args) =>
        if not @scenario
            print "No valid scenario was selected! Provided scenario name: #{args.scenario}"
            return false

        else
            for step in *@scenario
                app_path = @current_dir .. '/' .. step.executable
                os.execute "start #{app_path} \"#{table.concat step.options, ' '}\""

            return true

{ :RunCommand }
