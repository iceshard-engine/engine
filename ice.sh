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

ice_initialize() {
    find_profile_arg $*

    cd build/tools
    conan install ../../tools --build=missing --profile $conan_profile
    cd ../..
}

# Ensure the build dir exists
[ ! -d "build" ] && mkdir -p "build"

[ ! -d "build/tools" ] && mkdir -p "build/tools"

[ ! -f "build/tools/activate.sh" ] && ice_initialize $*

[ "$1" = "init" ] && ice_initialize $*

# Activate the enviroment
. ./build/tools/activate.sh

# Update fastbuild executables so we can run them actually
# I have currently no idea, how I can set them executable from conan
chmod +x $FBUILD_EXE

# Run any moonscript 'script'
lua $MOON_SCRIPT workspace.moon $*
ret_code=$?

# Deactivate the enviroment
. ./build/tools/deactivate.sh

if [ $ret_code != 0 ]; then
    exit $ret_code
fi

exit 0
