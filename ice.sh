#!/bin/sh

ice_initialize() {
    cd build/tools
    conan install ../../tools --build=missing
    cd ../..
}

# Ensure the build dir exists
[ ! -d "build" ] && mkdir -p "build"

[ ! -d "build/tools" ] && mkdir -p "build/tools"

[ ! -f "build/tools/activate.sh" ] && ice_initialize

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
