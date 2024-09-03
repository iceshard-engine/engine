#include "resource_filesystem_baked.hxx"
#include <ice/mem_allocator_stack.hxx>

namespace ice
{


    namespace detail
    {

        auto async_file_read(
            ice::native_file::File file,
            ice::usize fileoffset,
            ice::usize filesize,
            ice::NativeAIO* nativeio,
            ice::Memory data
        ) noexcept -> ice::Task<bool>
        {
            ice::AsyncReadFile asyncio{ nativeio, ice::move(file), data, filesize, fileoffset };
            ice::AsyncReadFile::Result const read_result = co_await asyncio;
            ICE_ASSERT(
                read_result.bytes_read == filesize,
                "Failed to load file into memory, requested bytes: {}!",
                filesize
            );
            co_return read_result.bytes_read == filesize;
        }

        auto async_file_load(
            ice::Allocator& alloc,
            ice::NativeAIO* nativeio,
            ice::String filepath,
            ice::ResourceFormatHeader const& header
        ) noexcept -> ice::Task<ice::Memory>
        {
            ice::native_file::HeapFilePath native_filepath{ alloc };
            ice::native_file::path_from_string(native_filepath, filepath);

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
                    bool const success = co_await async_file_read(ice::move(handle), { header.offset }, { header.size }, nativeio, result);
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
            ice::native_file::path_from_string(native_filepath, filepath);

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
        , _uri{ _name }
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
        return ice::string::substr(_uri.path(), 1);
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
        ice::NativeAIO* nativeio
    ) const noexcept -> ice::Task<ice::Memory>
    {
        if (nativeio != nullptr)
        {
            co_return co_await detail::async_file_load(alloc, nativeio, _origin, _header);
        }
        else
        {
            co_return detail::sync_file_load(alloc, _origin, _header);
        }
    }

    auto create_resource_from_baked_file(
        ice::Allocator& alloc,
        ice::String uri_base,
        ice::native_file::FilePath file_path
    ) noexcept -> ice::FileSystemResource*
    {
        IPT_ZONE_SCOPED;
        using enum ice::native_file::FileOpenFlags;

        ice::BakedFileResource* main_resource = nullptr;
        ice::native_file::File file = ice::native_file::open_file(file_path, Read);
        if (file == false)
        {
            return main_resource;
        }

        ice::ResourceFormatHeader header{};
        ice::usize read = ice::native_file::read_file(
            file, ice::size_of<ice::ResourceFormatHeader>, ice::memory_from(header)
        );

        if (read < 8_B
            || header.magic != ice::Constant_ResourceFormatMagic
            || header.version != ice::Constant_ResourceFormatVersion)
        {
            return main_resource;
        }

        ice::HeapString<> utf8_file_path{ alloc };
        ice::native_file::path_to_string(file_path, utf8_file_path);
        ice::path::normalize(utf8_file_path);
        IPT_ZONE_TEXT_STR(utf8_file_path);

        ice::HeapString<> utf8_uri{ alloc };
        ice::string::push_back(utf8_uri, uri_base);

        char temp[128];
        read = ice::native_file::read_file(
            file, ice::size_of<ResourceFormatHeader>, ice::usize{ header.name_size }, Memory{ temp, 128, ualign::b_1 }
        );

        ICE_ASSERT_CORE(read >= 0_B);
        ice::string::push_back(utf8_uri, { temp, header.name_size });

        ice::Memory metadata = alloc.allocate(ice::usize{ header.meta_size });
        read = ice::native_file::read_file(
            file, {header.meta_offset}, {header.meta_size}, metadata
        );
        ICE_ASSERT_CORE(read >= 0_B);

        return alloc.create<BakedFileResource>(alloc, header, ice::move(utf8_file_path), ice::move(utf8_uri), metadata);
    }

} // namespace ice
