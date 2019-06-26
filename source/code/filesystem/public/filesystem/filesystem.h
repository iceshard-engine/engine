#pragma once
#include <memsys/allocator.h>

#include <filesystem/filesystem_api.h>
#include <filesystem/details.h>
#include <filesystem/path.h>
#include <filesystem/file.h>

#include <functional>

namespace fs
{
    extern FILESYSTEM_API detail::find_file_func_type find_file;
    extern FILESYSTEM_API detail::open_file_func_type open_file;
    extern FILESYSTEM_API detail::move_file_func_type move_file;
    extern FILESYSTEM_API detail::copy_file_func_type copy_file;
    extern FILESYSTEM_API detail::delete_file_func_type delete_file;

    FILESYSTEM_API std::string current_working_directory();

    class FILESYSTEM_API filesystem final
    {
    public:
        static void initialize(mem::allocator& alloc);
        static void shutdown();
        static filesystem& instance();

        ~filesystem();

        void mount(std::string path);
        void unmount(std::string path);

        void for_each_file(std::function<void(std::string)> func);

    protected:
        filesystem(mem::allocator& alloc);

    private:
        mem::allocator& _allocator;

        //! File system PIMPL.
        struct filesystem_data_t;
        filesystem_data_t* _impl;

        //! Holds the global filesystem object.
        static filesystem* s_instance;
    };
}
