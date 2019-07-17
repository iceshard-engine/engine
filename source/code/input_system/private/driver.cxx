#include <input_system/module.hxx>
#include <core/string_view.hxx>

#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace input
{
    namespace detail
    {
        using MediaDriverCreateFunc = InputQuery*(core::allocator&);
        using MediaDriverReleaseFunc = void(core::allocator&, InputQuery*);

        class InputModuleDLL final : public InputModule
        {
        public:
            InputModuleDLL(core::allocator& alloc, HMODULE module_handle, InputQuery* driver_object, MediaDriverReleaseFunc* release_func) noexcept
                : _allocator{ alloc }
                , _module_handle{ module_handle }
                , _media_driver_object{ std::move(driver_object) }
                , _media_driver_release_func{ release_func }
            { }

            ~InputModuleDLL() noexcept
            {
                _media_driver_release_func(_allocator, _media_driver_object);
                FreeLibrary(_module_handle);
            }

            auto media_driver() noexcept -> InputQuery* override
            {
                return _media_driver_object;
            }

            auto media_driver() const noexcept -> InputQuery* override
            {
                return _media_driver_object;
            }

        private:
            core::allocator& _allocator;

            HMODULE _module_handle;

            InputQuery* _media_driver_object;

            MediaDriverReleaseFunc* _media_driver_release_func;
        };

    } // namespace detail

    auto load_driver_module(core::allocator& alloc, core::StringView<> driver_module_path) noexcept -> core::memory::unique_pointer<InputModule>
    {
        auto module_path = std::filesystem::canonical(core::string::begin(driver_module_path));

        // The result object
        core::memory::unique_pointer<InputModule> result{ nullptr, { alloc } };

        // Try to load the module.
        HMODULE module_handle = LoadLibraryEx(module_path.generic_string().c_str(), NULL, NULL);
        if (module_handle != nullptr)
        {
            void* create_driver_addr = GetProcAddress(module_handle, "create_driver");
            void* release_driver_addr = GetProcAddress(module_handle, "release_driver");

            // Check both functions.
            if (create_driver_addr && release_driver_addr)
            {
                auto create_driver_func = reinterpret_cast<detail::MediaDriverCreateFunc*>(create_driver_addr);
                auto release_driver_func = reinterpret_cast<detail::MediaDriverReleaseFunc*>(release_driver_addr);

                result = { alloc.make<detail::InputModuleDLL>(alloc, module_handle, create_driver_func(alloc), release_driver_func), alloc };
            }
        }

        return result;
    }

} // namespace input
