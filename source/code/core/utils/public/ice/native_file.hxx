/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os/unix.hxx>
#include <ice/os/windows.hxx>
#include <ice/path_utils.hxx>

namespace ice::native_file
{

#if ISP_WINDOWS
    using File = ice::win32::FileHandle;
    using FilePath = ice::WString;
    using HeapFilePath = ice::HeapString<ice::wchar>;
#   define ISP_PATH_LITERAL(val) L##val
#elif ISP_UNIX
    using File = ice::unix_::FileHandle;
    using FilePath = ice::String;
    using HeapFilePath = ice::HeapString<>;
#   define ISP_PATH_LITERAL(val) val
#endif

    enum class FileOpenFlags : ice::u8
    {
        //! \brief Same as 'ReadOnly'
        None = 0b0000'0000,

        //! \brief Opens the file for read operations.
        Read = 0b0000'0000,

        //! \brief Opens the file for write operations.
        Write = 0b0000'0001,

        //! \brief Opens the file in exclusive mode.
        Exclusive = 0b0001'0000,

        //! \brief Opens the file for asynchronous read-only operations.
        Asynchronous = 0b0010'0000,
    };

    bool exists_file(
        ice::native_file::FilePath path
    ) noexcept;

    auto open_file(
        ice::native_file::FilePath path,
        ice::native_file::FileOpenFlags flags = FileOpenFlags::Read
    ) noexcept -> ice::native_file::File;

    auto sizeof_file(ice::native_file::File const& native_file) noexcept -> ice::usize;
    auto sizeof_file(ice::native_file::FilePath path) noexcept -> ice::usize;

    auto read_file(
        ice::native_file::File const& native_file,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::usize;

    auto read_file(
        ice::native_file::File const& native_file,
        ice::usize requested_read_offset,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::usize;

    auto write_file(
        ice::native_file::File const& native_file,
        ice::usize write_offset,
        ice::Data data
    ) noexcept -> ice::usize;

    auto append_file(
        ice::native_file::File const& native_file,
        ice::Data data
    ) noexcept -> ice::usize;

    enum class TraverseAction : ice::u8 { Continue, Break, SkipSubDir };
    enum class EntityType : ice::u8 { File, Directory };

    using TraversePathCallback = auto(*)(
        ice::native_file::FilePath,
        ice::native_file::FilePath,
        ice::native_file::EntityType,
        void* userdata
    ) noexcept -> ice::native_file::TraverseAction;

    bool traverse_directories(
        ice::native_file::FilePath starting_dir,
        ice::native_file::TraversePathCallback callback,
        void* userdata
    ) noexcept;

    void path_from_string(
        ice::String path_string,
        ice::native_file::HeapFilePath& out_filepath
    ) noexcept;

    void path_to_string(
        ice::native_file::FilePath path,
        ice::HeapString<>& out_string
    ) noexcept;

    void path_join_string(
        ice::native_file::HeapFilePath& path,
        ice::String string
    ) noexcept;

} // namespace ice::native_file
