# IceShard engine

## Prerequesites 
To use this engine you will need the following tools: 
* [Conan Package Manager](https://conan.io/) - for dependency management.
* \[windows\] Visual Studio 2017 15.7 or later
* \[macosx\] Not supported yet.
* \[unix\] Not supported yet.

## Installation
The installation process is really simple. 
Just add the below conan repositories and start working ;)

```bash
conan remote add conan-iceshard https://conan.iceshard.net/
conan remote add conan-bincrafters https://api.bintray.com/conan/bincrafters/public-conan
```

**Note:** If some conan packages are shown as missing you will need to build the localy.

## Project generation (currently windows only)
To generate projects for development run the following command in the root directory:
```bash
iceshard generate-projects
```

This will generate the Visual Studio solution **IceShard.sln**.

## Building 
To just build the engine run the following command in the root directory:

```bash
iceshard build
```

This will build the engine in the ReleaseDebug configuration for the host platform.
