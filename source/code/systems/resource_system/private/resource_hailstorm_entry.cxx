#include "resource_hailstorm_entry.hxx"
#include "resource_provider_hailstorm.hxx"
#include "native/native_aio_tasks.hxx"
#include <ice/task_utils.hxx>

namespace ice
{

    namespace detail
    {

        auto sync_offset_file_read(
            ice::Allocator& alloc,
            ice::native_fileio::File const& handle,
            ice::usize offset,
            ice::usize size
        ) noexcept -> ice::Memory
        {
            ice::Memory result{
                .location = nullptr,
                .size = 0_B,
                .alignment = ice::ualign::invalid,
            };

            if (handle)
            {
                ice::usize const filesize = ice::native_fileio::sizeof_file(handle);
                ICE_ASSERT(
                    filesize.value < ice::ucount_max,
                    "Trying to load file larger than supported!"
                );

                result = alloc.allocate(filesize);
                // TODO: Better error handling. Using "expected".
                ice::usize const bytes_read = ice::native_fileio::read_file(
                    ice::move(handle),
                    offset,
                    filesize,
                    result
                );
                if (bytes_read.value == 0)
                {
                    ICE_ASSERT(
                        bytes_read.value != 0,
                        "Failed to load file!"
                    );

                    alloc.deallocate(result);
                }
            }
            return result;
        }

        auto async_offset_file_read(
            ice::NativeAIO* nativeio,
            ice::native_fileio::File const& file,
            ice::usize offset,
            ice::usize size
        ) noexcept -> ice::Task<ice::Memory>;

    } // namespace detail

    HailstormResource::HailstormResource(
        ice::URI const& uri,
        ice::hailstorm::HailstormResource const& handle
    ) noexcept
        : _handle{ handle }
        , _uri{ uri }
    {
    }

    HailstormResourceMixed::HailstormResourceMixed(
        ice::URI const& uri,
        ice::hailstorm::HailstormResource const& handle,
        ice::HailstormChunkLoader& loader
    ) noexcept
        : HailstormResource{ uri, handle }
        , _loader{ loader }
    {
    }

    auto HailstormResourceMixed::load_metadata(ice::Metadata& out_view) const noexcept -> ice::Task<bool>
    {
        ice::Memory const mem = co_await _loader.request_slice(_handle.meta_offset, _handle.meta_size, nullptr);
        if (mem.location != nullptr)
        {
            out_view = ice::meta_load(ice::data_view(mem));
            co_return out_view._meta_entries._hashes != nullptr;
        }
        co_return false;
    }

    HailstormResourceSplit::HailstormResourceSplit(
        ice::URI const& uri,
        ice::hailstorm::HailstormResource const& handle,
        ice::HailstormChunkLoader& meta_loader,
        ice::HailstormChunkLoader& data_loader
    ) noexcept
        : HailstormResource{ uri, handle }
        , _meta_loader{ meta_loader }
        , _data_loader{ data_loader }
    {
    }

    auto HailstormResourceSplit::load_metadata(ice::Metadata& out_view) const noexcept -> ice::Task<bool>
    {
        ice::Memory const mem = co_await _meta_loader.request_slice(_handle.meta_offset, _handle.meta_size, nullptr);
        if (mem.location != nullptr)
        {
            out_view = ice::meta_load(ice::data_view(mem));
            co_return out_view._meta_entries._hashes != nullptr;
        }
        co_return false;
    }

} // namespace ice
