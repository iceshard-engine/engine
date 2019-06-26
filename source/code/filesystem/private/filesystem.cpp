#include <filesystem/filesystem.h>

#include <kernel/logger/logger.h>

#include <unordered_map>
#include <filesystem>
#include <cassert>

namespace std_fs = std::experimental::filesystem;

using namespace fs;
using namespace fs::detail;

file::ptr find_file_default_function(std::string path)
{
    return fs::native_file::open(path, ios::binary | ios::in);
}

file::ptr open_file_default_function(std::string path, fs::openmode mode)
{
    return fs::native_file::open(path, mode);
}

file::ptr move_file_default_function(std::string from, std::string to)
{
    file::ptr result = nullptr;
    std::error_code err;
    std_fs::rename(from.c_str(), to.c_str(), err);
    if (!err)
        result = open_file(to, ios::binary | ios::in);
    return result;
}

file::ptr copy_file_default_function(std::string from, std::string to)
{
    file::ptr result = nullptr;
    if (std_fs::is_regular_file(from.c_str()))
    {
        // If the input file is a regular file and the output file does not exists and was sucessful copied
        if (!std_fs::exists(to.c_str()) && std_fs::copy_file(from.c_str(), to.c_str()))
            result = open_file(to, ios::binary | ios::in);
    }
    return result;
};

bool delete_file_default_function(std::string path)
{
    return std_fs::remove(path.c_str());
}

find_file_func_type fs::find_file = find_file_default_function;
open_file_func_type fs::open_file = open_file_default_function;
move_file_func_type fs::move_file = move_file_default_function;
copy_file_func_type fs::copy_file = copy_file_default_function;
delete_file_func_type fs::delete_file = delete_file_default_function;

std::string fs::current_working_directory()
{
    return std_fs::current_path().generic_string();
}

filesystem* filesystem::s_instance = nullptr;

class file_entry_t {
public:
    virtual ~file_entry_t() = default;
    virtual file::ptr get_file_handle(fs::openmode mode) const = 0;
};

class native_file_entry_t : public file_entry_t
{
public:
    native_file_entry_t(const std::string& path) : m_Path(path) { }
    file::ptr get_file_handle(fs::openmode mode) const override
    {
        return fs::native_file::open(m_Path.c_str(), mode);
    }

private:
    std::string m_Path;
};

struct filesystem::filesystem_data_t
{
    using file_map = std::unordered_map<std::string, file_entry_t*>;
    using file_map_entry = std::unordered_map<std::string, file_entry_t*>::value_type;

    file_map files;
};

void filesystem::initialize(mem::allocator& alloc)
{
    assert(s_instance == nullptr);
    s_instance = MAKE_NEW(alloc, filesystem, alloc);
}

void filesystem::shutdown()
{
    mem::allocator& alloc = s_instance->_allocator;
    MAKE_DELETE(alloc, filesystem, s_instance);
    s_instance = nullptr;
}

filesystem& filesystem::instance()
{
    assert(s_instance != nullptr);
    return *s_instance;
}

filesystem::filesystem(mem::allocator& alloc)
    : _allocator{ alloc }
    , _impl{ nullptr }
{
    MLogInfo("Filesystem initialized...");
    _impl = _allocator.make<filesystem_data_t>();

    // Replace some function pointers
    find_file = [](std::string path) -> file::ptr
    {
        file::ptr result = nullptr;
        if (s_instance->_impl->files.count(path) > 0)
        {
            result = s_instance->_impl->files[path]->get_file_handle(fs::ios::in | fs::ios::binary);
        }
        else if (std_fs::is_regular_file(path))
        {
            result = find_file_default_function(path);
        }
        return result;
    };
}

void filesystem::mount(std::string dir_path)
{
    auto& file_list = _impl->files;

    std_fs::recursive_directory_iterator directory(dir_path.c_str());
    for (auto&& entry : directory)
    {
        if (std_fs::is_regular_file(entry))
        {
            auto filepath = entry.path();
            auto fullpath = std_fs::canonical(filepath).string();
            auto filename = filepath.filename().string();

            // Save the file
            file_list[filename.c_str()] = _allocator.make<native_file_entry_t>(fullpath.c_str());
        }
    }
}

void fs::filesystem::for_each_file(std::function<void(std::string)> func)
{
    for (const auto& entry : _impl->files)
    {
        func(entry.first);
    }
}

void filesystem::unmount(std::string path)
{
    // nothing?
}

filesystem::~filesystem()
{
    open_file = open_file_default_function;

    for (auto&& entry : _impl->files)
    {
        _allocator.destroy(entry.second);
    }
    _allocator.destroy(_impl);

    MLogInfo("Filesystem shutdown...");
}
