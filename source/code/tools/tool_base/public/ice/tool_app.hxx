/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/module_register.hxx>
#include <ice/native_file.hxx>
#include <ice/param_list.hxx>
#include <ice/tool.hxx>

namespace ice::tool
{

    class ToolAppBase
    {
    public:
        static constexpr ice::String Constant_ToolName = "tool";
        static constexpr ice::String Constant_ToolVersion = "0.0.1";
        static constexpr ice::String Constant_ToolDescription = "";

    public:
        virtual ~ToolAppBase() noexcept = default;

        virtual void setup(ice::ParamList& params) noexcept = 0;
        virtual auto run(ice::ParamList const& params) noexcept -> ice::i32 = 0;
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
