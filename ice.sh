#!/bin/bash

conan_profile='linux_x64_clang21'
script_path="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
saved_cwd="$(pwd)"

find_profile_arg() {

    while [ $# -gt 0 ] ; do
        if [ "$1" = "--conan_profile" ]; then
            shift
            if [ -n "$1" ]; then
                conan_profile="$1"
                shift
            else
                echo "Error: Missing value for --conan_profile argument!"
                exit 1
            fi
        else
            shift
        fi
    done
}

ice_execute() {

    # Activate the RUN enviroment
    . "$script_path/build/tools/conanrun.sh"

    # Update fastbuild executables so we can run them actually
    # I have currently no idea, how I can set them executable from conan
    chmod +x $FBUILD_EXE

    # Run any moonscript 'script'
    cd $script_path
    lua $MOON_SCRIPT workspace.moon --build-profile $conan_profile $*
    ret_code=$?
    cd $saved_cwd

    # Deactivate the RUN enviroment
    . "$script_path/build/tools/deactivate_conanrun.sh"

    if [ $ret_code != 0 ]; then
        exit $ret_code
    fi

    exit 0
}

ice_initialize() {
    find_profile_arg $*

    cd $script_path/build/tools
    conan install ../../tools -of . --build=missing -pr:h $conan_profile -pr:b $conan_profile
    cd $saved_cwd

    # Continue execution normally
    ice_execute $*
}

# Ensure the build dir exists
[ ! -d "$script_path/build" ] && mkdir -p "$script_path/build"

[ ! -d "$script_path/build/tools" ] && mkdir -p "$script_path/build/tools"

[ ! -f "$script_path/build/tools/conanrun.sh" ] && ice_initialize $*

[ "$1" = "init" ] && ice_initialize $*

# Execute any of the commands given
ice_execute $*
