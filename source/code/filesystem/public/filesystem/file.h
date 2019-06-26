#pragma once
#include <filesystem/filesystem_api.h>
#include <filesystem/details.h>

namespace fs
{
    // The general file interface
    class FILESYSTEM_API file abstract
    {
    public:
        using ptr = fs::file_ptr;
        using size_t = std::size_t;

        // Constant fields
        static const size_t nsize;

        // Default virtual destructor
        virtual ~file() = default;

        // Return the file size or file::nsize if invalid
        virtual size_t size() = 0;

        // Reads 'size' * 'count' bytes into the given 'buffer'
        //    and returns the number of bytes read [sync]
        virtual size_t read(void* buffer, size_t size, size_t count) = 0;

        // Writes 'size' * 'count' bytes from the given 'buffer'
        //    and returns the number of bytes written [sync]
        virtual size_t write(const void* buffer, size_t size, size_t count) = 0;

        // Sets the current position in the file
        virtual void seekp(size_t pos, fs::seekdir origin = fs::ios::beg) = 0;

        // Returns the current position in the file
        virtual size_t tellp() = 0;

        // Sets the current position in the file
        virtual void seekg(size_t pos, fs::seekdir origin = fs::ios::beg) = 0;

        // Returns the current position in the file
        virtual size_t tellg() = 0;

        // Returns true if eof was reached
        virtual bool eof() const = 0;

        // Returns true if no error is set
        virtual bool good() const = 0;

        // Clears all error flags on the file object
        virtual void clear() = 0;
    };

    // A synchronous file object
    class FILESYSTEM_API native_file : public file
    {
        friend class filesystem;
    public:
        static file::ptr open(std::string path, fs::openmode = std::ios::binary | std::ios::in);
        native_file(std::string path, fs::openmode mode);
        ~native_file();

        size_t size() override;

        size_t read(void* buffer, size_t size, size_t count) override;
        size_t write(const void* buffer, size_t size, size_t count) override;

        void seekp(size_t pos, fs::seekdir origin = std::ios::beg) override;
        size_t tellp() override;

        void seekg(size_t pos, fs::seekdir origin = std::ios::beg) override;
        size_t tellg() override;

        bool eof() const override;
        bool good() const override;
        void clear() override;

    private:
        struct file_data_t;
        file_data_t* m_File;
    };

    class cache_file : public file
    {
    public:
        void* data() const;
        size_t size() override;

        size_t read(void* buffer, size_t size, size_t count) override;
        size_t write(const void* buffer, size_t size, size_t count) override;

        void seekp(size_t pos, fs::seekdir origin = std::ios::beg) override;
        size_t tellp() override;

        bool eof() const override;

    protected:
        cache_file();
        cache_file(void* data, size_t size);
        ~cache_file();

        // data modification helpers
        void set_data(void* data, size_t size);
        void append_data(void* data, size_t size);
        void release_data();

    private:
        void* m_Data;
        size_t m_Size;
    };

    class pack
    {
    public:
    };

    class pack_file : public cache_file
    {
    public:


    private:
        std::weak_ptr<pack> m_Pack;
    };
}
