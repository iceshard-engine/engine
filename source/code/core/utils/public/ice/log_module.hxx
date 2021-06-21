#pragma once
#include <ice/module.hxx>

namespace ice
{

    void initialize_log_module(
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* api
    ) noexcept;

    void load_log_module(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* api
    ) noexcept;

    void unload_log_module(
        ice::Allocator* alloc
    ) noexcept;

} // namespace ice
