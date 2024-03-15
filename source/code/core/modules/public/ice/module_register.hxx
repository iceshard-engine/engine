/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/string_types.hxx>
#include <ice/stringid.hxx>
#include <ice/container_types.hxx>
#include <ice/container/array.hxx>
#include <ice/module_negotiator.hxx>
#include <ice/module_query.hxx>
#include <ice/module.hxx>

namespace ice
{

    //! \brief Type to manage loading and unloading of modules and their APIs.
    class ModuleRegister : public ice::ModuleQuery
    {
    public:
        virtual ~ModuleRegister() noexcept = default;

        //! \brief Loads a module from a given path.
        //! \note This path needs to be a valid .dll, .so or .dylib file depending on the platform.
        //! \param[in] alloc The allocator to use for the module.
        //! \param[in] path The path to the module file.
        virtual bool load_module(
            ice::Allocator& alloc,
            ice::String path
        ) noexcept = 0;

        //! \brief Loads a module using explicit load and unload functions.
        //!   This is a less preferred way of loading modules, use load_module<T>(ice::Allocator&) instead.
        //! \param[in] alloc The allocator to use for the module.
        //! \param[in] load_fn The function to call when loading the module.
        //! \param[in] unload_fn The function to call when unloading the module.
        virtual bool load_module(
            ice::Allocator& alloc,
            ice::FnModuleLoad* load_fn,
            ice::FnModuleUnload* unload_fn
        ) noexcept = 0;

        //! \brief Loads a module using a module info structure.
        virtual bool load_module(ice::Allocator& alloc, ice::ModuleInfo const& module_info) noexcept
        {
            return this->load_module(alloc, module_info.fn_load, module_info.fn_unload);
        }

        //! \brief Loads a module using a module type. This is the preferred way of loading modules implicitly.
        template<typename T> requires (ice::concepts::ModuleType<T> || ice::concepts::ModuleLoadable<T>)
        bool load_module(ice::Allocator& alloc) noexcept
        {
            return this->load_module(alloc, T::module_info());
        }
    };

    //! \brief Loads all modules available in the current executable.
    //! \note This can be done automatically when creating the default module register.
    //! \note As of now, loading modules twice might result in unexpected issues.
    auto load_global_modules(
        ice::Allocator& alloc,
        ice::ModuleRegister& modules_register
    ) noexcept -> ice::ucount;

    //! \brief Creates a default module register.
    //! \param[in] alloc The allocator to use for the module register.
    //! \param[in] load_global_modules If true, all modules available in the current executable will be loaded.
    auto create_default_module_register(
        ice::Allocator& alloc,
        bool load_global_modules = true
    ) noexcept -> ice::UniquePtr<ice::ModuleRegister>;

} // namespace ice
