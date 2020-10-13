#!/bin/sh

ice_initialize() {
    cd build/tools
    conan install ../../tools --build=missing
    cd ../..
}

ice_run_application() {
  typeset cmnd="lua $MOON_SCRIPT workspace.moon $*"
  typeset ret_code

  eval $cmnd
  ret_code=$?
  if [ $ret_code != 0 ]; then
    printf "Error : [%d] when executing command: '$cmnd'" $ret_code
    exit $ret_code
  fi
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
ice_run_application()

# Deactivate the enviroment
. ./build/tools/deactivate.sh
