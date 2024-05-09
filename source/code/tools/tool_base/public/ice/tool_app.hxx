/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/module_register.hxx>
#include <ice/native_file.hxx>
#include <ice/params.hxx>
#include <ice/tool.hxx>

namespace ice::tool
{

    class ToolAppBase
    {
    public:
        virtual ~ToolAppBase() noexcept = default;

        virtual auto name() const noexcept -> ice::String = 0;
        virtual auto version() const noexcept -> ice::String { return "0.0.1"; }
        virtual auto description() const noexcept -> ice::String { return ""; }

        [[nodiscard]]
        virtual bool setup(ice::Params& params) noexcept { return true; }

        [[nodiscard]]
        virtual auto run() noexcept -> ice::i32 = 0;
    };

    struct ToolAppInstancer
    {
        using CreateFn = auto (ice::Allocator&) noexcept -> ice::tool::ToolAppBase*;
        using DestroyFn = void (ice::Allocator&, ice::tool::ToolAppBase*) noexcept;

        ToolAppInstancer(CreateFn* fn_create, DestroyFn* fn_destroy) noexcept;
    };

    template<typename T>
    class ToolApp : public ToolAppBase
    {
    protected:
        inline ToolApp() noexcept;

    protected:
        ice::Allocator& _allocator;
        ice::UniquePtr<ice::ModuleRegister> _modules;

    private:
        static inline auto create_tool_app(ice::Allocator& alloc) noexcept -> ice::tool::ToolAppBase*;
        static inline void destroy_tool_app(ice::Allocator& alloc, ice::tool::ToolAppBase* app) noexcept;

    public:
        static inline ToolAppInstancer const AppInstancer{ &create_tool_app, &destroy_tool_app };
    };

    template<typename T>
    inline ToolApp<T>::ToolApp() noexcept
        : _allocator{ ice::tool::global_allocator() }
        , _modules{ ice::create_default_module_register(_allocator) }
    { }

    template<typename T>
    inline auto ToolApp<T>::create_tool_app(ice::Allocator& alloc) noexcept -> ice::tool::ToolAppBase*
    {
        return alloc.create<T>();
    }

    template<typename T>
    inline void ToolApp<T>::destroy_tool_app(ice::Allocator& alloc, ice::tool::ToolAppBase * app) noexcept
    {
        alloc.destroy(app);
    }

} // namespace ice
