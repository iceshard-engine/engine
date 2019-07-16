#include <device/system.hxx>
#include <core/pod/array.hxx>

namespace device
{

    void query_devices(core::pod::Array<device::Device*>& device_list) noexcept
    {
        core::pod::array::clear(device_list);

        // #todo SDL2 device query
    }

    auto load_provider(core::allocator& alloc, core::StringView<> /*driver_module_path*/) noexcept -> core::memory::unique_pointer<DeviceProvider>
    {
        // #todo SDL2 device query

        return { nullptr, { alloc } };
    }

} // namespace device
