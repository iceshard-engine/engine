/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "module_native.hxx"
#include <ice/string/heap_string.hxx>
#include <ice/mem_allocator_stack.hxx>

namespace ice::native_module
{

#if ISP_WINDOWS

    bool utf8_to_wide_append_module(ice::String path, ice::HeapString<ice::wchar>& out_str) noexcept
    {
        ice::i32 const required_size = MultiByteToWideChar(CP_UTF8, 0, ice::string::begin(path), ice::string::size(path), NULL,  0);

        if (required_size != 0)
        {
            ice::u32 const current_size = ice::string::size(out_str);
            ice::u32 const total_size = static_cast<ice::u32>(required_size) + ice::string::size(out_str);
            ice::string::resize(out_str, total_size);

            [[maybe_unused]]
            ice::i32 const chars_written = MultiByteToWideChar(
                CP_UTF8,
                0,
                ice::string::begin(path),
                ice::string::size(path),
                ice::string::begin(out_str) + current_size,
                ice::string::size(out_str) - current_size
            );
        }

        return required_size != 0;
    }

    auto module_open(ice::String path) noexcept -> ice::native_module::ModuleHandle
    {
        ice::StackAllocator<512_B> temp_alloc;
        ice::HeapString<ice::wchar> wide_path{ temp_alloc };
        if (utf8_to_wide_append_module(path, wide_path))
        {
            return ice::native_module::ModuleHandle{ LoadLibraryExW(ice::string::begin(wide_path), NULL, NULL) };
        }
        return {};
    }

    void module_close(ice::native_module::ModuleHandle module) noexcept
    {
        module.close();
    }

    auto module_find_address(ice::native_module::ModuleHandle const& module, ice::String symbol_name) noexcept -> void*
    {
        return ::GetProcAddress(module.native(), ice::string::begin(symbol_name));
    }

#elif ISP_UNIX

    auto module_open(ice::String path) noexcept -> ice::native_module::ModuleHandle
    {
        return ice::native_module::ModuleHandle{ ::dlopen(ice::string::begin(path), RTLD_NOW) };
    }

    void module_close(ice::native_module::ModuleHandle module) noexcept
    {
        module.close();
    }

    auto module_find_address(ice::native_module::ModuleHandle const& module, ice::String symbol_name) noexcept -> void*
    {
        return ::dlsym(module.native(), ice::string::begin(symbol_name));
    }

#endif

} // namespace ice
