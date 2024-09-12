/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_filesystem_baked.hxx"
#include "resource_aio_request.hxx"
#include <ice/mem_allocator_stack.hxx>

namespace ice
{


    namespace detail
    {

        auto async_file_read(
            ice::native_file::File file,
            ice::usize fileoffset,
            ice::usize filesize,
            ice::native_aio::AIOPort aioport,
            ice::Memory data
        ) noexcept -> ice::Task<bool>
        {
            ice::detail::AsyncReadRequest request{ aioport, file, filesize, 0_B, data };
            ice::usize const bytes_read = co_await request;

            ICE_ASSERT(
                bytes_read == filesize,
                "Failed to load file into memory, requested bytes: {}!",
                filesize
            );
            co_return bytes_read == filesize;
        }

        auto async_file_load(
            ice::Allocator& alloc,
            ice::native_aio::AIOPort aioport,
            ice::String filepath,
            ice::ResourceFormatHeader const& header
        ) noexcept -> ice::Task<ice::Memory>
        {
            ice::native_file::HeapFilePath native_filepath{ alloc };
            ice::native_file::path_from_string(filepath, native_filepath);

            ice::Memory result{
                .location = nullptr,
                .size = 0_B,
                .alignment = ice::ualign::invalid,
            };

            ice::native_file::File handle = ice::native_file::open_file(
                native_filepath,
                ice::native_file::FileOpenFlags::Asynchronous
            );
            if (handle)
            {
                if (header.size > 0)
                {
                    result = alloc.allocate(ice::usize{ header.size });
                    bool const success = co_await async_file_read(ice::move(handle), { header.offset }, { header.size }, aioport, result);
                    if (success == false)
                    {
                        alloc.deallocate(result);
                    }
                }
                else
                {
                    // Allocate 1 byte just to not return a nullptr.
                    // TODO: Find a better way to handle this.
                    result = alloc.allocate(1_B);
                }
            }
            co_return result;
        }

        auto sync_file_load(
            ice::Allocator& alloc,
            ice::String filepath,
            ice::ResourceFormatHeader const& header
        ) noexcept -> ice::Memory
        {
            ice::native_file::HeapFilePath native_filepath{ alloc };
            ice::native_file::path_from_string(filepath, native_filepath);

            ice::Memory result{
                .location = nullptr,
                .size = 0_B,
                .alignment = ice::ualign::invalid,
            };

            ice::native_file::File handle = ice::native_file::open_file(native_filepath);
            if (handle)
            {
                result = alloc.allocate(ice::usize{ header.size });
                // TODO: Better error handling. Using "expected".
                ice::usize const bytes_read = ice::native_file::read_file(
                    ice::move(handle),
                    { header.offset },
                    { header.size },
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

    } // namespace detail

    BakedFileResource::BakedFileResource(
        ice::Allocator& alloc,
        ice::ResourceFormatHeader const& header,
        ice::HeapString<> origin,
        ice::HeapString<> name,
        ice::Memory metadata
    ) noexcept
        : _allocator{ alloc }
        , _header{ header }
        , _origin{ ice::move(origin) }
        , _name{ ice::move(name) }
        , _metadata{ metadata }
        , _uri{ ice::Scheme_File, _name }
    {
    }

    BakedFileResource::~BakedFileResource() noexcept
    {
        _allocator.deallocate(_metadata);
    }

    auto BakedFileResource::uri() const noexcept -> ice::URI const&
    {
        return _uri;
    }

    auto BakedFileResource::flags() const noexcept -> ice::ResourceFlags
    {
        return ice::ResourceFlags::Status_Baked;
    }

    auto BakedFileResource::name() const noexcept -> ice::String
    {
        return _name;
    }

    auto BakedFileResource::origin() const noexcept -> ice::String
    {
        return _origin;
    }

    auto BakedFileResource::load_metadata() const noexcept -> ice::Task<ice::Data>
    {
        co_return ice::data_view(_metadata);
    }

    auto BakedFileResource::size() const noexcept -> ice::usize
    {
        return { _header.size };
    }

    auto BakedFileResource::load_data(
        ice::Allocator& alloc,
        ice::TaskScheduler& scheduler,
        ice::native_aio::AIOPort aioport
    ) const noexcept -> ice::Task<ice::Memory>
    {
        if (aioport != nullptr)
        {
            co_return co_await detail::async_file_load(alloc, aioport, _origin, { _header.offset });
        }
        else
        {
            co_return detail::sync_file_load(alloc, _origin, { _header.offset });
        }
    }

    auto create_resource_from_baked_file(
        ice::Allocator& alloc,
        ice::native_file::FilePath file_path
    ) noexcept -> ice::FileSystemResource*
    {
        ice::ResourceFormatHeader header;
        ice::HeapString<> origin{ alloc };
        ice::HeapString<> name{ alloc };
        ice::Memory metadata{ };

        return alloc.create<BakedFileResource>(alloc, header, ice::move(origin), ice::move(name), metadata);
    }

} // namespace ice
