#include <filesystem/file.h>

using namespace fs;

struct native_file::file_data_t {
    std::fstream file_obj;
};

file::ptr native_file::open(std::string path, fs::openmode mode)
{
    return std::make_unique<native_file>(path, mode);
}

size_t fs::native_file::size()
{
    m_File->file_obj.seekg(0, std::ios::end);
    size_t size = m_File->file_obj.tellg();
    m_File->file_obj.seekg(0, std::ios::beg);
    return size;
}

size_t native_file::read(void* buffer, size_t size, size_t count)
{
    m_File->file_obj.read(reinterpret_cast<char*>(buffer), size * count);
    return m_File->file_obj.gcount();
}

size_t native_file::write(const void* buffer, size_t size, size_t count)
{
    m_File->file_obj.write(reinterpret_cast<const char*>(buffer), size * count);
    return size * count;
}

void native_file::seekp(size_t pos, fs::seekdir origin)
{
    m_File->file_obj.seekp(pos, origin);
}

size_t fs::native_file::tellp()
{
    return static_cast<size_t>(m_File->file_obj.tellp());
}

void native_file::seekg(size_t pos, fs::seekdir origin)
{
    m_File->file_obj.seekg(pos, origin);
}

size_t fs::native_file::tellg()
{
    return static_cast<size_t>(m_File->file_obj.tellg());
}

bool native_file::eof() const
{
    return m_File->file_obj.eof();
}

bool native_file::good() const
{
    return m_File->file_obj.good();
}

void native_file::clear()
{
    m_File->file_obj.clear();
}

native_file::native_file(std::string path, fs::openmode mode) : m_File(nullptr)
{
    m_File = new file_data_t { std::fstream(path, mode) };
}

native_file::~native_file()
{
    delete m_File;
}
