#pragma once
#include <ice/os/unix.hxx>
#include <ice/os/windows.hxx>
#include <ice/task.hxx>
#include <ice/string/string.hxx>

namespace ice::native_fileio
{

#if ISP_WINDOWS
    using File = ice::win32::FileHandle;
    using FilePath = ice::WString;
    using HeapFilePath = ice::HeapString<ice::wchar>;
#elif ISP_UNIX
    using File = ice::unix_::FileHandle;
    using FilePath = ice::String;
    using HeapFilePath = ice::HeapString<>;
#endif

    bool exists_file(
        ice::native_fileio::FilePath path
    ) noexcept;

    auto open_file(
        ice::native_fileio::FilePath path
    ) noexcept -> ice::native_fileio::File;

    auto sizeof_file(
        ice::native_fileio::File const& file
    ) noexcept -> ice::usize;

    auto read_file(
        ice::native_fileio::File const& file,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::usize;

    enum class TraverseAction : ice::u8 { Continue, Break, SkipSubDir };
    enum class EntityType : ice::u8 { File, Directory };

    using TraversePathCallback = auto(*)(
        ice::native_fileio::FilePath,
        ice::native_fileio::FilePath,
        ice::native_fileio::EntityType,
        void* userdata
    ) noexcept -> ice::native_fileio::TraverseAction;

    bool traverse_directories(
        ice::native_fileio::FilePath starting_dir,
        ice::native_fileio::TraversePathCallback callback,
        void* userdata
    ) noexcept;

    namespace path
    {

        void from_string(
            ice::String path_string,
            ice::native_fileio::HeapFilePath& out_filepath
        ) noexcept;

        void to_string(
            ice::native_fileio::FilePath path,
            ice::HeapString<>& out_string
        ) noexcept;

        void join(
            ice::native_fileio::HeapFilePath& path,
            ice::String string
        ) noexcept;

        auto length(
            ice::native_fileio::FilePath path
        ) noexcept -> ice::ucount;

    } // namespace path

} // namespace ice::native_fileio
