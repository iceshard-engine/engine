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
            ice::usize offset,
            ice::Memory target_memory,
            ice::native_aio::AIOPort aioport,
            ice::String filepath
        ) noexcept -> ice::Task<ice::Result>
        {
            if (target_memory.size == 0_B)
            {
                co_return S_Ok;
            }

            ice::native_file::HeapFilePath native_filepath{ alloc };
            ice::native_file::path_from_string(filepath, native_filepath);
            ice::Expected<ice::native_file::File> handle = ice::native_file::open_file(aioport, native_filepath);
            if (handle)
            {
                co_await async_file_read(ice::move(handle).value(), offset, target_memory.size, aioport, target_memory);
            }
            co_return handle.error();
        }

        auto sync_file_load(
            ice::Allocator& alloc,
            ice::usize offset,
            ice::Memory target_memory,
            ice::String filepath
        ) noexcept -> ice::Result
        {
            if (target_memory.size == 0_B)
            {
                return S_Ok;
            }

            ice::native_file::HeapFilePath native_filepath{ alloc };
            ice::native_file::path_from_string(filepath, native_filepath);
            ice::native_file::File handle = ice::native_file::open_file(native_filepath);
            if (handle == false)
            {
                return E_Fail;
            }

            ice::usize const bytes_read = ice::native_file::read_file(
                ice::move(handle),
                offset,
                target_memory.size,
                target_memory
            );
            return bytes_read == 0_B;
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
        , _uri{ ice::Scheme_File, _name }
    {
    }

    BakedFileResource::~BakedFileResource() noexcept
    {
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

    auto BakedFileResource::size() const noexcept -> ice::usize
    {
        return { _header.size };
    }

    auto BakedFileResource::load_data(
        ice::Allocator& alloc,
        ice::Memory& memory,
        ice::String fragment,
        ice::native_aio::AIOPort aioport
    ) const noexcept -> ice::TaskExpected<ice::Data>
    {
        ice::usize const datasize = ice::usize{(_header.offset - _header.meta_offset) + _header.size};
        ICE_ASSERT(
            memory.location == nullptr || memory.size >= datasize,
            "Allocated memory is not large enough to store resource data and metadata!"
        );
        if (memory.location == nullptr)
        {
            memory = alloc.allocate(datasize);

            if (aioport != nullptr)
            {
                co_await detail::async_file_load(alloc, { _header.meta_offset }, memory, aioport, _origin);
            }
            else
            {
                detail::sync_file_load(alloc, { _header.meta_offset }, memory, _origin);
            }
        }

        if (fragment == "meta")
        {
            co_return Data{ memory.location, {_header.meta_size}, ice::ualign::b_8 };
        }
        else
        {
            co_return Data{
                ice::ptr_add(memory.location, {_header.offset - _header.meta_offset}),
                {_header.size},
                ice::ualign::b_default
            };
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
