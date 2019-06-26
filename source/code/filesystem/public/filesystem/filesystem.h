#pragma once
#include <core/memory.hxx>
#include <filesystem/path.h>

namespace filesystem
{

std::string current_working_directory();

class filesystem final
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

} // namespace filesystem
