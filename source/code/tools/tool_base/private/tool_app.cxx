/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/tool_app.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/heap_string.hxx>
#include <ice/assert.hxx>

namespace ice::tool
{

    static ToolAppInstancer::CreateFn* fn_app_create;
    static ToolAppInstancer::DestroyFn* fn_app_destroy;

    ToolAppInstancer::ToolAppInstancer(CreateFn* fn_create, DestroyFn* fn_destroy) noexcept
    {
        ICE_ASSERT(
            fn_app_destroy == nullptr && fn_app_create == nullptr,
            "Only one type can derive from 'ToolApp<T>' base class in a single application."
        );
        fn_app_create = fn_create;
        fn_app_destroy = fn_destroy;
    }

} // namespace ice::tool

int main(int argc, char** argv)
{
    using namespace ice::tool;

    ICE_ASSERT(
        fn_app_create != nullptr && fn_app_destroy != nullptr,
        "Application does not provide 'main' or derives from the 'ToolApp<T>' base class."
    );

    ice::Allocator& alloc = global_allocator();
    ToolAppBase* const app = fn_app_create(alloc);

    ice::Params params_v2 = ice::create_params(alloc, app->name(), app->version(), app->description());

    ice::i32 result = 0;
    if (app->setup(params_v2))
    {
        if (result = ice::params_process(params_v2, argc, argv); result == 0)
        {
            result = app->run();
        }
    }

    fn_app_destroy(alloc, app);
    return result;
}
