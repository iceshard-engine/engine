# IceShard

A small game engine project with the sole purpose to learn, improve and invent.
Focusing on the best solution for a given problem while not trying to solve everything.

More info about the development approach can be found in our [wiki](https://github.com/iceshard-engine/engine/wiki).

## Features

Current list of advertisement:
* Dependency injection based architecture.
* Support for basic input devices.
* APIs designed with the 'RAII' principle.
    * _Exceptions are debug tools and utilities._
* Inovative resource and asset systems.
* Data-oriented ECS implementation.
* Abstracted API for rendering. (Vulkan)
* Multi-threaded logic and graphics using C++ coroutines.

Thid party tools and features:
* Support for **Tracy** profiler.
* Optional DevUI API based on **ImGui**. _(ex. disabled in Releaseand Profile builds)_
* Simple 2D Physics implemented with **Box2D**
* Loading of common file formats supported with **RapidXML**, **RapidJSON** and **Assimp**.
* Logging using the **fmt** library.
* Unit tests written in the **Catch2** framework.


### Build status

#### Windows _(Tested)_
![Code validation result for Windows targets.](https://github.com/iceshard-engine/engine/actions/workflows/build-validate-windows.yaml/badge.svg)

#### Linux _(Tested)_
![Code validation result for Linux targets.](https://github.com/iceshard-engine/engine/actions/workflows/build-validate-linux.yaml/badge.svg)

#### Android _(Untested)_
![Code validation result for Android targets.](https://github.com/iceshard-engine/engine/actions/workflows/build-validate-android.yaml/badge.svg)

##### Known issues
* The x64 binaries do not load properly in emulators
* The binaries use outdated API Levels and NKD version
* The binaries are still build with 4 KiB page support instead of required 16 KiB

#### Emscripten _(Tested)_
![Code validation result for WebAssembly targets.](https://github.com/iceshard-engine/engine/actions/workflows/build-validate-emscripten.yaml/badge.svg)

## Building the engine

A quick overview how to build the engine on your machine.

### Prerequisites
To build this engine you will need the following tools and SDKs installed:
* [Conan Package Manager](https://conan.io/) - Used to manage project dependencies.
   * This also requires python3 as a dependency.
* **Windows:**
   * Visual Studio 2022 _(17.13 or later)_
   * Windows Kit (10.0.19041.0 or later)
   * Vulkan SDK _(1.4.313.0 or later)_
* **Linux:**
    * Toolchain: Clang-20
    * Vulkan SDK _(1.4.313.0 or later)_
* **Android:** _(Outdated)_
    * Toolchain: NDK-27
    * AndroidAPI: 29
* **Web:**
    * Toolchain: Emscripten-v4.0.9
* **MacOS:** _(No plans)_

### Configuring Conan

To properly initialize the workspace, you will need to setup Conan with configurations from the [IceShard-Conan-Config](https://github.com/iceshard-engine/conan-config.git) repository.
This contains the Conan clang profiles and remotes that should be used with this project.

The quickest way to setup Conan for this project is to use the following command:

```
conan config install https://github.com/iceshard-engine/conan-config.git
```

### Ice Build Tools

This project uses its own command line tool named **Ice Build Tools** to provide various utilities that can be used during development.


## Contributing

Contributions are welcome, however they need to follow the
[Coding Style](https://github.com/iceshard-engine/coding-style) of the engine and pass the review process.

Additionally, some contributions might also require additional changes if the implementation does not follow the design principles of this project.

It is however possible to ask for a separate repository that will and provide new features via modules API. This would only require to follow the aforementioned coding style.


## Copyright Information

The engine is licensed under the [MIT License](LICENSE).

Additionally, all used third party libraries are mentioned in the [thirdparty/README.md](thirdparty/README.md).
Their licenses are available for lookup in [thirdparty/LICENSES.txt](thirdparty/LICENSES.txt)


## Acknowledgements

This project was heavily inspired by several articles, but mostly by the BitSquid development blog.

Additionally, some parts of the engine were initially based on the **BitSquid Foundation Library** which was discussed here: https://bitsquid.blogspot.com/2012/11/bitsquid-foundation-library.html.
