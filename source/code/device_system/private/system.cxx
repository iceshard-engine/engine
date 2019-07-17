#include <device/system.hxx>
#include <core/pod/array.hxx>

#include <core/string_view.hxx>
#include <core/stack_string.hxx>

#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace device
{
    namespace detail
    {
        class ProviderDll final : public DeviceProvider
        {
        public:
            ProviderDll(HMODULE module_handle, ProviderAPI* api) noexcept
                : _module_handle{ module_handle }
                , _api_table{ api }
            { }

            ~ProviderDll() noexcept
            {
                FreeLibrary(_module_handle);
            }

            //! \brief Queries the current device driver for supported devices.
            void query_devices(core::pod::Array<device::Device*>& device_list) noexcept override
            {
                //core::pod::array::clear(device_list);
                _api_table->query_devices_func(device_list);
            }

        private:
            HMODULE _module_handle;
            ProviderAPI* _api_table;
        };
    }

    auto load_provider(core::allocator& alloc, core::StringView<> driver_module_path) noexcept -> core::memory::unique_pointer<DeviceProvider>
    {
        auto module_path = std::filesystem::canonical(core::string::begin(driver_module_path));

        // The result object
        core::memory::unique_pointer<DeviceProvider> result{ nullptr, { alloc } };

        // Try to load the module.
        HMODULE module_handle = LoadLibraryEx(module_path.generic_string().c_str(), NULL, NULL);
        if (module_handle != nullptr)
        {
            void* proc_addr = GetProcAddress(module_handle, "get_api");
            if (proc_addr != nullptr)
            {
                auto proc_func = reinterpret_cast<ProviderAPI*(*)()>(proc_addr);

                result = { alloc.make<detail::ProviderDll>(module_handle, proc_func()), alloc };
            }
        }


        return result;
    }

} // namespace device
