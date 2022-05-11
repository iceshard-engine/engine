import Command, option, flag from require "ice.command"
import Exec from require "ice.tools.exec"
import Json from require "ice.util.json"

class LicenseCommand extends Command
    @arguments {
        option 'gen_3rdparty',
            name: '--gen-3rdparty'
            default: 'thirdparty/readme.md'
            defmode: 'arg'
        flag 'check',
            name: '--check'
        flag 'clean',
            name: '--clean'
    }

    prepare: (args, project) =>
        @current_dir = os.cwd!

        @details = {}
        if details_file = io.open "thirdparty\\details.json"
            @details = Json\decode details_file\read '*a'
            details_file\close!

    search_for_license_files: (out_license_files, rootpath, dir) =>
        known_license_files = {
            'license': true
            'license.md': true
            'license.txt': true
            'license.rst': true
            'copying.txt': true
            'copyright': true
        }

        for candidate_file, mode in os.listdir "#{rootpath}/#{dir}", 'mode'
            continue if mode ~= 'file'

            if dir == "."
                if known_license_files[candidate_file\lower!] ~= nil
                    table.insert out_license_files, "#{rootpath}\\#{candidate_file}"
            else
                table.insert out_license_files, "#{rootpath}\\#{dir}\\#{candidate_file}"


    extract_recipe_info: (conanfile) =>
        allowed_fields = {
            'url': true
            'homepage': true
            'license': true
            'description': true
        }

        result = { }
        if file = io.open conanfile, "rb+"
            contents = file\read "*a"
            for field, value in contents\gmatch "(%w*) = \"([^\\\"]*)\""
                result[field] = value if allowed_fields[field]
            file\close!
        result

    execute: (args, project) =>
        -- Build conan if missing debug info or 'clean' check requested
        project.action.install_conan_dependencies! unless (not args.clean) and os.isfile 'build/conan_debug/conanbuildinfo.json'

        license_files = {}
        with file = io.open 'build/conan_debug/conanbuildinfo.json'

            if buildinfo = Json\decode file\read '*a'

                for dependency in *buildinfo.dependencies

                    found_license_files = { }
                    for subdir in *{ ".", "LICENSE", "COPYRIGHT", "LICENSES" }
                        @search_for_license_files found_license_files, dependency.rootpath, subdir

                    -- Gather license files
                    if #found_license_files > 0
                        selected_license = nil

                        if #found_license_files > 1

                            if @details[dependency.name] and @details[dependency.name].license_file
                                for license_file in *found_license_files
                                    if license_file\lower!\match @details[dependency.name].license_file\lower!
                                        selected_license = license_file

                            if selected_license == nil and args.check
                                print "Packge '#{dependency.name}' contains more than one license file."
                                print "> Please select the desired license file in 'thirdparty/details.json'."
                                continue

                        table.insert license_files, {
                            dependency,
                            "#{dependency.rootpath}\\..\\..\\export\\conanfile.py",
                            selected_license or found_license_files[1]
                        }

                    else if args.check
                        print "Packge '#{dependency.name}' is missing license file..."

            file\close!

        if license_files and #license_files > 0

            if args.gen_3rdparty
                os.mkdir "#{@current_dir}\\thirdparty"

                if licenses = io.open "#{@current_dir}\\thirdparty\\LICENSES.txt", "wb+"
                    for { dep, _, license_file } in *license_files
                        licenses\write "\n-------------------- START '#{dep.name\lower!}' --------------------\n"
                        if license_file_handle = io.open license_file, "rb+"
                            for line in license_file_handle\lines!
                                licenses\write "    #{line}\n"
                            license_file_handle\close!
                        licenses\write "-------------------- END '#{dep.name\lower!}' --------------------\n\n"
                    licenses\close!

                if readme = io.open "#{@current_dir}\\thirdparty\\README.md", "wb+"

                    readme\write "# Third Party Libraries\n\n"
                    readme\write "A file generated from all in-used conan dependencies.\n"
                    readme\write "Listed alphabetically with general information about each third party dependency.\n"
                    readme\write "For exact copies of eache license please follow the upstream link to look into [LICENSES.txt](LICENSES.txt).\n"

                    for { dep, conanfile, license_file } in *license_files
                        conaninfo = @extract_recipe_info conanfile
                        readme\write "\n## #{dep.name}\n"

                        license_info = conaninfo.license or 'not found'
                        upsteam_info = conaninfo.url or conaninfo.homepage
                        description_info = conaninfo.description or dep.description
                        if @details[dep.name]
                            description_info = @details[dep.name].description or description_info
                            license_info = @details[dep.name].license or license_info
                            upsteam_info = @details[dep.name].upstream or upsteam_info

                        readme\write "#{description_info}\n"
                        readme\write "- **upstream:** #{upsteam_info}\n" if upsteam_info
                        readme\write "- **version:** #{conaninfo.version or dep.version}\n"
                        readme\write "- **license:** #{license_info}\n"

                    readme\close!

        true

{ :LicenseCommand }
