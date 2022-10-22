import Command, option, flag from require "ice.command"
import Exec from require "ice.tools.exec"
import Json from require "ice.util.json"

lfs = require 'lfs'

class LicenseCommand extends Command
    @arguments {
        option 'mode',
            name: '-m --mode'
            default: 'source'
            choices: { 'source', '3rdparty' }
        flag 'fix_headers'
            name: '--fix-headers'
            description: 'Fixes copyright and SPDX identifier in source file.'
        option 'gen_3rdparty',
            name: '--gen-3rdparty'
            description: 'Generates license files for consumed 3rd party libraries. Only usable with mode == 3rdparty.'
            default: 'thirdparty/readme.md'
            defmode: 'arg'
        flag 'check',
            name: '--check'
        flag 'clean',
            description: 'Regenerates all 3rd-party dependencies. Only usable with mode == 3rdparty.'
            name: '--clean'
    }

    prepare: (args, project) =>
        @current_dir = os.cwd!

        if args.mode == '3rdparty'
            @details = {}
            if details_file = io.open "thirdparty/details.json"
                @details = Json\decode details_file\read '*a'
                details_file\close!

    execute: (args, project) =>
        if not args.check and not args.fix_headers and not args.gen_3rdparty
            print "Licensing tools for IceShard. Check help for more information."

        return @execute_mode_source args, project if args.mode == 'source'
        return @execute_mode_3rdparty args, project if args.mode == '3rdparty'


    --[[ source code licensing tools ]]

    check_license_header: (file, args) =>
        pat_cr = '[/!*]+ Copyright (%d+) %- (%d+), ([%s%S]+)' -- and the (%w+) contributors'
        pat_spdx = '[/!*]+ SPDX%-License%-Identifier: (%w+)'

        if f = io.open file
            lines_it = f\lines!

            line_cr = lines_it!
            line_spdx = lines_it!
            f\close!

            -- Empty file?
            if not line_cr or not line_spdx return

            y_start, y_end, author, project = line_cr\match pat_cr
            spdx_identifier = line_spdx\match pat_spdx

            y_start = y_start and tonumber y_start or nil
            y_end = y_end and tonumber y_end or nil

            if args.check
                if not spdx_identifier or not author
                    print "Missing copyright and/or SPDX header in file: #{file}"

                else
                    y_modified = os.date("*t", lfs.attributes 'modification').year
                    if y_end < y_modified
                        print string.format("Modification year (found: %d, current: %d) is outdated in #{file}", y_end, y_modified)

            else if args.fix_headers
                res = lfs.attributes file
                y_start = y_start or os.date("*t", res.change).year
                y_modified = os.date("*t", res.modification).year
                y_start = y_modified if y_start > y_modified -- Make sure the second year is never newer than the first one

                newline = os.iswindows and "\r\n" or "\n"

                -- We start with year 2022 (TODO: Move to a config file or project setup)
                author = author or "Dandielo <dandielo@iceshard.net>"
                spdx_id = "MIT"

                if spdx_identifier == nil or author == nil
                    print "Adding missing copyright and SPDX header in file: #{file}"
                    line_header = "/// Copyright #{y_start} - #{y_modified}, #{author}"
                    line_spdx = "/// SPDX-License-Identifier: #{spdx_id}"

                    if f = io.open file, "rb"
                        contents = f\read "*a"
                        contents = line_header .. newline .. line_spdx .. newline .. newline .. contents
                        f\close!

                        if f = io.open file, "wb"
                            f\write contents
                            f\close!
                        else
                            print "Failed to update file #{file}"

                else if y_end < y_modified
                    print "Updating modification year value in file: #{file}"
                    line_header = "/// Copyright #{y_start} - #{y_modified}, #{author}"

                    if f = io.open file, "rb"
                        contents = f\read "*a"
                        contents = line_header .. contents\sub (#line_cr + 1)
                        f\close!

                        if f = io.open file, "wb"
                            f\write contents
                            f\close!
                        else
                            print "Failed to update file #{file}"



        else
            print "Failed to open file #{file}"

    search_dir: (dir, args) =>
        sdpx_extensions = {
            ".hxx": true
            ".cxx": true
            ".bff": true
            ".inl": true
            -- We check additional source files only during 'check' runs
            ".h": args.check
            ".c": args.check
            ".hpp": args.check
            ".cpp": args.check
        }

        for candidate_path, mode in os.listdir dir, 'mode'
            continue if candidate_path == "." or candidate_path == ".."
            @search_dir "#{dir}/#{candidate_path}", args if mode == 'directory'

            if mode == 'file'
                file_path = "#{dir}/#{candidate_path}"
                file_name = candidate_path
                file_name_len = #file_name
                file_ext = file_name\sub (file_name\find "%."), file_name_len


                @check_license_header file_path, args if sdpx_extensions[file_ext]


    execute_mode_source: (args, project) =>
        print "Warning: Flag '--clean' has no effect in 'source' mode." if args.clean
        print "Warning: Argument '--gen-3rdparty' has no effect in 'source' mode." if args.gen_3rdparty

        @search_dir "#{os.cwd!}/#{project.source_dir}", args
        print "Checks finished." if args.check
        return true


    --[[ 3rd party licensing tools ]]

    search_for_license_files: (out_license_files, rootpath, dir) =>
        known_license_files = {
            'license': true
            'license.md': true
            'license.txt': true
            'license.rst': true
            'copyright': true
            'copyright.txt': true
        }

        found_license = false
        if os.isdir "#{rootpath}/#{dir}"
            for candidate_file, mode in os.listdir "#{rootpath}/#{dir}", 'mode'
                continue if mode ~= 'file'

                if dir == "."
                    if known_license_files[candidate_file\lower!] ~= nil
                        table.insert out_license_files, "#{rootpath}/#{candidate_file}"
                        found_license = true
                else
                    table.insert out_license_files, "#{rootpath}/#{dir}/#{candidate_file}"
                    found_license = true
        found_license


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

    execute_mode_3rdparty: (args, project) =>
        print "Warning: Flag '--fix-headers' has no effect in 'source' mode." if args.fix_headers

        -- Build conan if missing debug info or 'clean' check requested
        project.action.install_conan_dependencies! unless (not args.clean) and os.isfile 'build/conan_debug/conanbuildinfo.json'

        license_files = {}
        with file = io.open 'build/conan_debug/conanbuildinfo.json'

            if buildinfo = Json\decode file\read '*a'

                for dependency in *buildinfo.dependencies

                    found_license_files = { }
                    for subdir in *{ ".", "LICENSE", "COPYRIGHT", "LICENSES" }
                        if not @search_for_license_files found_license_files, dependency.rootpath, subdir
                            @search_for_license_files found_license_files, dependency.rootpath, subdir\lower!

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
                                for license_file in *found_license_files
                                    print "- #{license_file}"
                                continue

                        table.insert license_files, {
                            dependency,
                            "#{dependency.rootpath}/../../export/conanfile.py",
                            selected_license or found_license_files[1]
                        }

                    else if args.check
                        print "Packge '#{dependency.name}' is missing license file..."

            file\close!

        if license_files and #license_files > 0

            if args.gen_3rdparty
                os.mkdir "#{@current_dir}\\thirdparty"

                if licenses = io.open "#{@current_dir}/thirdparty/LICENSES.txt", "wb+"
                    for { dep, _, license_file } in *license_files
                        licenses\write "\n-------------------- START '#{dep.name\lower!}' --------------------\n"
                        if license_file_handle = io.open license_file, "rb+"
                            for line in license_file_handle\lines!
                                licenses\write "    #{line}\n"
                            license_file_handle\close!
                        licenses\write "-------------------- END '#{dep.name\lower!}' --------------------\n\n"
                    licenses\close!

                if readme = io.open "#{@current_dir}/thirdparty/README.md", "wb+"

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

        print "Checks finished." if args.check
        true

{ :LicenseCommand }
