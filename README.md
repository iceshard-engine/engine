# IceShard

The project is a game engine with a heavy focus on finding the best solution for a given problem, but not limiting to a single design approach.
More info about the development approach can be found in our [wiki]().

## Features

Currently the codebase features simple implementations of:
* Allocator oriented memory access.
    * Only `placement new` operators used.
* Support for **Tracy** profiler.
* Device inputs and events.
* Resource and asset access.
    * Runtime building of some assets.
* Abstracted API for rendering.
    * With an working implementation for vulkan.
* An extensive engine API.
    * Heavily data-oriented ECS implementation.
    * **World** design with user **Traits** as extension points.
    * Graphics layer build on top of the Render API and world traits.
    * Fully removable DevUI API.
* ...

## Building the engine

A quick overview how to build the engine localy on your machine.

### Prerequesites
To build this engine you will need the following tools on your PC installed:
* [Conan Package Manager](https://conan.io/) - Used to access project dependencies.
    * This also requires python3 as a dependency.
* **Windows:**
    * Required: Visual Studio 2019 _(16.10 or later)_
    * Required: Windows Kit (10.0.19041.0 or later)
    * Required: Vulkan SDK _(1.2.170.0 or later)_
        * Optional when a DX1* implementation is available.
* **Linux** - _(Under heavy development, currently not available in the repository)_
    * Preferred: Ubuntu 20.0 distribution or later
    * Required: Clang-10 C++ compiler or later
    * Required: standard library libc++-10 or later
    * Required: Vulkan SDK _(1.2.170.0 or later)_
        * Optional when a OpenGL implementation is available.
* **MacOS:**
    * No support

### Configuring Conan

To properly initialize the workspace you will need to download Conan configuration from the [Conan Config](https://github.com/iceshard-engine/conan-config.git) repository.
This contains the Conan clang profiles and remotes that should be used with this project.

The quickest way to setup Conan for this project is to use the following command:

```
conan config install https://github.com/iceshard-engine/conan-config.git
```

### Ice Build Tools

This project uses it's own command line tool named **Ice Build Tools** to provide various tools that can be used during development.

It is a Conan package that will be installed on first use. It can be easily updated at any point or by removing the `./build/` entire directory when something goes wrong.

---
#### The `build` command

Building the engine is straight forward, all you need to do is to call this command in your terminal and you have build the engine.

    ./ice.sh build
    ./ice.bat build

You can further specifiy the target you want to build by using the `-t --target` option.

    ice build -t all-x64-Debug

---
#### The `vstudio` command

This command allows to generate project files for Visual Studio.

    ./ice.bat vstudio

---
#### The `run` command

This command allows to execute pre-defined lists of other commands or tools in order. These execution `scenarios` are stored in the `scenarios.json` file and currently provide a quick way to run the test application and to build shaders on the Windows platform.

    :: Build all shaders into Vulkan SPIR-V
    ./ice.bat run -s shaders

    :: Run the default built test application executable (all-x64-Develop)
    ./ice.bat run


## Contributing

Contributions are welcome, however they need to follow the
[Coding Style](https://github.com/iceshard-engine/coding-style) of the engine and pass the review process.

Additionally some contributions might also require additional changes if the implementation does not follow the design principles of this project.

It is however possible to ask for a separate repository that will and provide new features via modules API.This would only require to follow the aformentioned coding style.


## License

The engine is licensed under [BSD 3-Clause Clear License](https://github.com/iceshard-engine/engine/blob/master/LICENSE).

## Aknowledgements

This project was heavily influenced by several articles, but mostly by the BitSquid development blog.
Because of this some parts of the engine may resemble solutins which can be found in the blogs posts.

Additionally, some parts of the engine where based on the **BitSquid Foundation Library** which was discussed here:
https://bitsquid.blogspot.com/2012/11/bitsquid-foundation-library.html
