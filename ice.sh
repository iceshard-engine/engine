#!/bin/sh

find_profile_arg() {
    conan_profile='default'

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
    . ./build/tools/conanrun.sh

    # Update fastbuild executables so we can run them actually
    # I have currently no idea, how I can set them executable from conan
    chmod +x $FBUILD_EXE

    # Run any moonscript 'script'
    lua $MOON_SCRIPT workspace.moon $*
    ret_code=$?

    # Deactivate the RUN enviroment
    . ./build/tools/deactivate_conanrun.sh

    if [ $ret_code != 0 ]; then
        exit $ret_code
    fi

    exit 0
}

ice_initialize() {
    find_profile_arg $*

    cd build/tools
    conan install ../../tools -of . --build=missing --profile $conan_profile
    cd ../..

    # Continue execution normally
    ice_execute $*
}

# Ensure the build dir exists
[ ! -d "build" ] && mkdir -p "build"

[ ! -d "build/tools" ] && mkdir -p "build/tools"

[ ! -f "build/tools/conanrun.sh" ] && ice_initialize $*

[ "$1" = "init" ] && ice_initialize $*

# Execute any of the commands given
ice_execute $*
