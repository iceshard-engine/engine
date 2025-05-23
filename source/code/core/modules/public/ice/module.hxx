/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/module_negotiator.hxx>
#include <ice/module_concepts.hxx>

namespace ice
{

    //! \brief Base class for modules that will automatically register them to be loaded by the module manager.
    //! \details Loading is done either by instantiating a module register object or by loading a dynamic module using the module regsiter.
    //! \tparam Type The module type that devices from this type.
    template <typename Type>
    class Module
    {
    public:
        virtual ~Module() noexcept = default;

        //! \returns Module info that can be used to manually load selected modules using the module register.
        static inline auto module_info() noexcept -> ice::ModuleInfo const& { return _module_info; }

    private:
        static inline void internal_load(
            ice::Allocator* alloc,
            ice::ModuleNegotiatorAPIContext* context,
            ice::ModuleNegotiatorAPI* negotiator
        ) noexcept
        {
            static_assert(
                ice::concepts::ModuleLoadable<Type>,
                "Module types need to provide a static 'on_load' function!"
            );

            // We use a tagged negotiator so we can register the same API type from multiple modules in the same local execution unit.
            ice::ModuleNegotiatorTagged<Type> const negotiator_instance{ negotiator, context };
            if (Type::on_load(*alloc, negotiator_instance) == false)
            {
                return;
            }
        }

        static inline void internal_unload(ice::Allocator* alloc) noexcept
        {
            // Not all modules need to be explicitly unloaded.
            if constexpr (ice::concepts::ModuleUnloadable<Type>)
            {
                Type::on_unload(*alloc);
            }
        }

        //! \brief This entry allows to register module load/unload functions without additional marcos or register functions.
        //! \details "static inline" forces the variable to be always instantiated, and the constructor will register the module
        //!   in a global list. This also acts as the "ModuleInfo" object.
        static inline ModulesEntry const _module_info{ &internal_load, &internal_unload };
    };

#if ISP_COMPILER_CLANG || ISP_COMPILER_GCC \
    // We access 'module_info' here to force Clang and Gcc to initialize the 'static inline' variable.
#   define IS_WORKAROUND_MODULE_INITIALIZATION(type) \
        private: static inline ice::ModuleInfo const& _workaround_info_instance = type::module_info()
#else
#   define IS_WORKAROUND_MODULE_INITIALIZATION(type)
#endif

} // namespace ice
