# Welcome to IceShard's API Documentation                         {#mainpage}
> [!NOTE]
> __This page currently under construction, and the currently generated documentation only consists of files found under `source/code/core`.__
> I hope to provide the full documentation as soon as possible! - Dandielo

## Project Structure


Due to the amount of code this engine already had and predictably would end up with, the codebase was early on separated into multiple categories. Each category has a general purpose and set of _(still)_ unwritten rules that I try to follow to ensure some conformity.

The categories are:
- **[Core](@ref par_category_core)** - Holds libraries that provide most common types, collections or core functionality like logging or native file APIs.
- **Systems** - Holds libraries that define system APIs _(often with basic implementations)_ that abstract away large chunks of logic.
- **Platforms** - Holds implementations of all supported platforms and their supported features.
- **IceShard** - Holds implementation of IceShard which is separated into a public API project and the dynamically loaded implementation of it.
- **Modules** - Holds implementations of various systems, like Rendering or DevUI. _(If some modules are missing the engine can run without them)_
- **Framework** - Holds logic commonly seen in games and can be used to quick-start a project.
- **Examples** - Holds examples for each supported platform.
- **Tools** - Holds additional tools that are used by the project. _(ex.: Packaging tool for the HailStorm format)_


### Core libraries                {#par_category_core}

All libraries in this category provide a specific set of types or functionality for a single simple topic. All are either statically linked or header only.
In some cases the APIs' are designed with **C** like APIs' focusing on free functions, while some libraries are now utilizing C++ `concepts` and `explicit this` features to utilize member syntax, while allowing to extend types with common functionality. _(See [Collections](@ref ice::container::ContiguousContainer) library for examples)_

Some libraries utilize global state is some form, however, unless explicitly initialized from the final binary the API's fallback to implementations that do not require a global state, which might result in reduced functionality. For example:
- The logger will not resolve custom tags nor attach additional information like timestamps.
- The DevUI is fully disabled until a runtime DevUI module is loaded.

Currently the following libraries are part of this category:
- **[Core][prj_core]** - Provides most basic type aliases / defitions, constants, build defines based on the targeted platform.
- **[Math][prj_math]** - Provides types for basic math functions and vector arithmetic.
- **[Memsys][prj_memsys]** - Provides API interface for memory allocation and allocation tracking.
- **[Collections][prj_collections]** - Provides access to basic container types, including but not limited to _[String](@ref ice::BasicString)_, _[Array](@ref ice::Array)_, _[HashMap](@ref ice::HashMap)_, and others.
- **[Modules][prj_modules]** - Provides utilities to load and initlize API's across all runtime loaded modules.
- **[I18N][prj_i18n]** - Provides API interface for internationalization features.
- **[Logger][prj_logger]** - Provides API interface and implementation for advanced message logging.
- **[Tasks][prj_tasks]** - Provides API interface and implementation for multi-thread processing. _(built around C++ `coroutines`)_
- **[DevUI][prj_devui]** - Provides API interface for registering of widgets and ImGui utility functions. _(requires runtime implementation)_
- **[Utils][prj_utils]** - Library for various utility functions that are too small for their own projects yet, but require a place in `Core`.

<!-- Link definitions -->
[prj_core]: @ref source/code/core/core/public/ice
[prj_math]: @ref source/code/core/math/public/ice
[prj_memsys]: @ref source/code/core/memsys/public/ice
[prj_collections]: @ref source/code/core/collections/public/ice
[prj_modules]: @ref source/code/core/modules/public/ice
[prj_i18n]: @ref source/code/core/i18n/public/ice
[prj_logger]: @ref source/code/core/logger/public/ice
[prj_tasks]: @ref source/code/core/tasks/public/ice
[prj_devui]: @ref source/code/core/devui/public/ice
[prj_utils]: @ref source/code/core/utils/public/ice
