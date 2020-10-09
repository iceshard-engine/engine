# IceShard engine

[IceShard WebSite](https://iceshard.net/)

[![](https://github.com/iceshard-engine/engine/workflows/Nightly/badge.svg)](https://github.com/iceshard-engine/engine/actions?workflow=Nightly)
[![](https://github.com/iceshard-engine/engine/workflows/Validation/badge.svg)](https://github.com/iceshard-engine/engine/actions?workflow=Validation)

## Prerequesites
To use this engine you will need the following tools:
* [Conan Package Manager](https://conan.io/) - for dependency management.
* \[windows\] Visual Studio 2019 16.4 or later
* \[macosx\] Not supported yet.
* \[unix\] *(in-development)* Clang-9 and Clang-10.

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
ice generate
```

This will generate the Visual Studio solution **IceShard.sln**.

## Building
To just build the engine run the following command in the root directory:

```bash
ice build
```

This will build the engine in the ReleaseDebug configuration for the host platform.

## GitHub Actions CI

Currently this project using the **GitHub Actions** service for CI.

Builds are there mostly for validation and will be expanded with new targets as time goes.

## Coding style

[Coding Style](https://github.com/iceshard-engine/coding-style)

## Aknowledgements

This project was heavily influenced by several articles, but mostly by the BitSquid development blog.
Because of this some parts of the engine may resemble solutins which can be found in the blogs posts.

Additionally, some parts of the engine where based on the **BitSquid Foundation Library** which was discussed here:
https://bitsquid.blogspot.com/2012/11/bitsquid-foundation-library.html
