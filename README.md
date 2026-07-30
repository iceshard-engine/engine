# IceShard

A small game engine project with the sole purpose to learn, improve and invent.
Focusing on the best solution for a given problem while not trying to solve everything.

More info about the development approach can be found in our [wiki](https://github.com/iceshard-engine/engine/wiki).

> [!IMPORTANT]
> With my recent move to full-time linux usage, I'm fixing a lot of issues that should have been fixed long time ago.
>
> Until this is finished, the project might not be fully working even on Windows.

> [!WARNING]
> The current information below is outdated and may not reflect the projects current state.

## Quick Start
Depending if you are working on windows or linux you would user `ice.sh` or `ice.bat` to run the commands below.


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
* **Android:**
    * Toolchain: NDK-28
    * AndroidAPI: 35
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


### Using the CLI

#### _(optional)_ Initialize the IBT CLI
> This happens automatically on any executed command. However executing it directly might help identify issues in the tools configuration.
```bash
./ice init
```

#### Generate projects for supported IDE's
```bash
./ice devenv --ide vstudio # Generates a `.sln` file _(and necessary project files)_ using FastBuild as the generator.
./ice devenv --ide vscode # Generates `.vscode/*.json` files that define various build, run and debug targets.
```

Once the project is generated you can build and debug the project in that specific IDE.
> _For Visual Studio Code it's necessary to have the LLDB debugger extension installed. (win:'cppvsdbg', unix:'lldb-dap')_

#### _(command-line)_ Build the project
```bash
./ice build all-x64-Debug # Builds the debug build for x64 bit windows/linux.
```

#### _(command-line)_ Start the project
```bash
./ice script start -- Debug # Starts the Debug build created in the previous step.
```

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


### Build Status
> Under reconstruction...


## Contributing
> Under reconstruction...


## Copyright Information

The engine is licensed under the [MIT License](LICENSE).

Additionally, all used third party libraries are mentioned in the [thirdparty/README.md](thirdparty/README.md).
Their licenses are available for lookup in [thirdparty/LICENSES.txt](thirdparty/LICENSES.txt)


## Acknowledgements

This project was heavily inspired by several articles, but mostly by the BitSquid development blog.

Additionally, some parts of the engine were initially based on the **BitSquid Foundation Library** which was discussed here: https://bitsquid.blogspot.com/2012/11/bitsquid-foundation-library.html.
