#pragma once
#include "memsys/memsys.h"
#include "memsys/allocators/stack_allocator.h"
#include "memsys/allocators/scratch_allocator.h"

#include <kernel/logger/logger.h>
#include <kernel/utils/scope_exit.h>
#include <kernel/compiletime/stringid.h>

#include <mutex>
#include <cassert>
#include <thread>

namespace mooned::logging
{

namespace detail
{

//! Implements a dynamic buffer using the fmt::Buffer<char> interface.
//! \notes This is needed to properly use out memory system.
class DynamicBuffer : public fmt::internal::basic_buffer<char>
{
public:
    //! The dynamic buffer takes the backing allocator.
    DynamicBuffer(mem::allocator& backing, uint32_t internal_size);

    void grow(std::size_t capacity) override;

    //! Clean Up.
    ~DynamicBuffer() override;

private:
    mem::scratch_allocator _allocator;
};

} // namespace detail

}

void mooned::log(logging::Logger& logger, logging::level lvl, const char* function, const char* format, fmt::format_args args)
{
    logger.log(lvl, function, format, args);
}

const char* mooned::logging::to_string(mooned::logging::level lvl)
{
    switch (lvl)
    {
#define CASE(arg, val) case  mooned::logging::level::##arg: return val
        CASE(Trace, "trace");
        CASE(Info, "info");
        CASE(Debug, "debug");
        CASE(Warning, "warning");
        CASE(Error, "error");
        CASE(Fatal, "fatal");
#undef CASE
    }
    return "{invalid}";
}

const char* to_string_log(mooned::logging::level lvl)
{
    switch (lvl)
    {
#define CASE(arg, val) case  mooned::logging::level::##arg: return val
        CASE(Trace, "[trace]");
        CASE(Info, "[info]");
        CASE(Debug, "[debug]");
        CASE(Warning, "[warning]");
        CASE(Error, "[error]");
        CASE(Fatal, "[fatal]");
#undef CASE
    }
    return "{invalid}";
}

namespace
{
    class DefaultLogger : public mooned::logging::Logger
    {
    public:
        virtual void log(mooned::logging::level lvl, const char* function, const char* format, fmt::format_args args) override
        {
            std::unique_lock<std::mutex>lk{ mtx };
            log_(lvl, function, format, std::move(args));
        }

        void log_(mooned::logging::level lvl, const char* function, const char* format, fmt::format_args args)
        {

            hires_timestamp ts;
            get_hires_timestamp(ts);

            int sec = ts.seconds % 60;
            ts.seconds /= 60;
            int min = ts.seconds % 60;
            ts.seconds /= 60;
            int hours = ts.seconds % 24;

            static auto& alloc = thread_local_allocator();
            alloc.clear();

            mooned::logging::detail::DynamicBuffer _fmt_buffer{ alloc, 4096 };

            fmt::vformat_to(_fmt_buffer, "{{{:0=2d}:{:0=2d}:{:0=2d}.{:0=3d}}}", fmt::make_format_args(hours, min, sec, ts.milis % 1000));
            fmt::vformat_to(_fmt_buffer, "{:>10}", fmt::make_format_args(to_string_log(lvl)));
            fmt::vformat_to(_fmt_buffer, "[{}] ", fmt::make_format_args(function != nullptr ? function : "log"));
            fmt::vformat_to(_fmt_buffer, format, args);// std::forward<Args>(args)...);

            // print the message
            printf("%.*s\n", static_cast<int>(_fmt_buffer.size()), _fmt_buffer.data());
        }

    private:
        auto thread_local_allocator() -> mem::stack_allocator_4096&
        {
            static thread_local mem::stack_allocator_4096 alloc;
            return alloc;
        }

        std::mutex mtx;
    };
}

//void fmt::format_arg(fmt::BasicFormatter<char>& f, const char*& /* format_str */, const stringid_t& uuid)
//{
//#ifdef _DEBUG
//    f.writer().write("{{ 'SID' {:#018x} `{}` }}", uuid._val, uuid._str);
//#else
//    f.writer().write("{{ 'SID' {:#018x} }}", uuid);
//#endif
//}

mooned::logging::detail::DynamicBuffer::DynamicBuffer(mem::allocator& backing, uint32_t internal_size)
    : basic_buffer{ }
    , _allocator{ backing, internal_size }
{
    set(nullptr, 0);
}

mooned::logging::detail::DynamicBuffer::~DynamicBuffer()
{
    auto* const old_buffer = this->data();
    if (old_buffer)
    {
        _allocator.deallocate(old_buffer);
    }
}

void mooned::logging::detail::DynamicBuffer::grow(std::size_t size)
{
    auto old_capacity = this->capacity();
    assert(size > old_capacity);

    auto* const old_buffer = this->data();
    auto* const new_buffer = reinterpret_cast<char*>(_allocator.allocate(static_cast<uint32_t>(size)));
    if (old_buffer)
    {
        memset(new_buffer, '\0', size);
        memcpy(new_buffer, old_buffer, old_capacity);
        new_buffer[old_capacity] = 0;
        _allocator.deallocate(old_buffer);
    }

    set(new_buffer, size);
}

mooned::logging::Logger& mooned::logging::default_logger()
{
    static DefaultLogger logger;
    return logger;
}
