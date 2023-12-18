#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/module_register.hxx>

namespace ice::framework
{

    void load_framework_module(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* api
    ) noexcept;

    void unload_framework_module(
        ice::Allocator* alloc
    ) noexcept;

    void register_assetype_modules(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry
    ) noexcept;

} // namespace ice::framework
