#pragma once
#include <ice/unique_ptr.hxx>
#include <ice/string.hxx>
#include <ice/stringid.hxx>
#include <ice/pod/collections.hxx>
#include <ice/module.hxx>

namespace ice
{

    class ModuleRegister
    {
    public:
        virtual ~ModuleRegister() noexcept = default;

        virtual bool load_module(
            ice::Allocator& alloc,
            ice::String path
        ) noexcept = 0;

        virtual bool load_module(
            ice::Allocator& alloc,
            ice::ModuleProcLoad* load_fn,
            ice::ModuleProcUnload* unload_fn
        ) noexcept = 0;

        virtual bool find_module_api(
            ice::StringID_Arg api_name,
            ice::u32 version,
            void** api_ptr
        ) const noexcept = 0;

        virtual bool find_module_apis(
            ice::StringID_Arg api_name,
            ice::u32 version,
            ice::pod::Array<void*>& api_ptrs_out
        ) const noexcept = 0;
    };

    auto create_default_module_register(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::ModuleRegister>;

} // namespace ice
