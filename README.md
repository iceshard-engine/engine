# IceShard

> IceShard is a small game engine project for the sole purpose of learning, improving, and inventing. The engine focuses on solving the most significant problem rather than trying to achieve everything.

More info about the development approach can be found in our [wiki](https://github.com/iceshard-engine/engine/wiki).

## Features

Current list of advertisement points:
* Allocator based architecture.
* Support for basic input devices.
* Almost all APIs designed with 'RAII' principle at the core.
    * _Exceptions are some debug tools and utilities._
* Simple resource and asset systems.
* Data-oriented ECS implementation.
* Abstracted API for rendering.
    * _With a working implementation for Vulkan._
* Multi-threaded logic and graphics frames using C++ coroutines.

Thid party tools and features:
* Support for **Tracy** profiler.
* Optional DevUI API based on **ImGui**. _(ex. disabled in Release builds)_
* Simple 2D Physics implemented with **Box2D**
* Loading of common file formats supported with **RapidXML**, **RapidJSON** and **Assimp**.
* Logging using the **fmt** library.
* Unit tests written in the **Catch2** framework.

## Building the engine

A quick overview how to build the engine on your machine.

### Prerequisites
To build this engine you will need the following tools and SDKs installed:
* [Conan Package Manager](https://conan.io/) - Used to manage project dependencies.
   * This also requires python3 as a dependency.
* **Windows:**
   * Required: Visual Studio 2019 _(16.10 or later)_
   * Required: Windows Kit (10.0.19041.0 or later)
   * Required: Vulkan SDK _(1.2.170.0 or later)_
* **Linux:** _(Compilation Only)_
   * Tested On:
      * Manjarno Linux (KDE Plasma) (Kernel 5.15.6-2-MANJARNO x64)
      * GitHub Runner: Ubuntu-Latest
   * Required: GCC-11 C++ compiler or later
   * Required: standard library libstdc++ v6 or later
   * ~~Required: Vulkan SDK _(1.2.170.0 or later)_~~ - Not implemented yet.
* **MacOS:**
   * No support

### Configuring Conan

To properly initialize the workspace, you will need to setup Conan with configurations from the [IceShard-Conan-Config](https://github.com/iceshard-engine/conan-config.git) repository.
This contains the Conan clang profiles and remotes that should be used with this project.

The quickest way to setup Conan for this project is to use the following command:

```
conan config install https://github.com/iceshard-engine/conan-config.git
```

### Ice Build Tools

This project uses its own command line tool named **Ice Build Tools** to provide various utilities that can be used during development.

It is a Conan package that will be installed on first use. It can be easily updated at any point or reset by removing the `./build/` directory when something goes wrong.

---
#### The `build` command

Building the engine is straight forward, all you need to do is to call this command in your terminal and you have built the engine.

    ./ice.sh build
    ./ice.bat build

You can further specify the target you want to build by using the `-t --target` option.

    ice build -t all-x64-Debug

---
#### The `vstudio` command

This command allows to generate project files for Visual Studio.

    ice vstudio

---
#### The `run` command

This command allows to execute pre-defined lists of other commands or tools in order. These execution `scenarios` are stored in the `scenarios.json` file and currently provide a quick way to run the test application and to build shaders on the Windows platform.

    :: Build all shaders into Vulkan SPIR-V
    ice run -s shaders

    :: Run the default-built test application executable (all-x64-Develop)
    ice run


## Contributing

Contributions are welcome, however following our [Coding Style](https://github.com/iceshard-engine/coding-style) is mandatory. The contributions will be reviewed and might be required additional changes to meet the design principles of this project. Creating a separate repository and integrating them to this repository as modules API would also work.

## Copyright Information

The engine is licensed under the [MIT License](LICENSE).

Additionally, all used third party libraries are mentioned in the [thirdparty/README.md](thirdparty/README.md).
Their licenses are available for lookup in [thirdparty/LICENSES.txt](thirdparty/LICENSES.txt)


## Acknowledgements

This project was heavily inspired by several articles, but mostly by the BitSquid development blog.

Additionally, some parts of the engine were initially based on the **BitSquid Foundation Library** which was discussed here: https://bitsquid.blogspot.com/2012/11/bitsquid-foundation-library.html.
