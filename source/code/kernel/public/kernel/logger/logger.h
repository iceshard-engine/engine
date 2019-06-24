#pragma once
#include <kernel/types.h>
#include <kernel/platform_defs.h>
#include <kernel/compiletime/stringid.h>

#include <fmt/format.h>
#include <chrono>

namespace mooned::logging
{
    /** \brief Logging level severity
     */
    enum class level
    {
        Trace, Debug, Info, Warning, Error, Fatal
    };

    /** \brief Returns a string representation of the level enum values.
    * \returns The a string describing the severity level or '{invalid}'.
    */
    const char* to_string(level lvl);

    /** \brief A logger interface used by all engine log functions.
     */
    class Logger
    {
    public:
        virtual ~Logger() = default;

        /** \brief Saves or displays the given message.
         * \remark Some messages may not be saved/displayed depending on the logger and the severity level.
         * \remark This method shouldn't be called directly, you should pass the logger object to one of the log functions.
         * \remark This method uses the FMT syntax \see http://fmtlib.net/latest/syntax.html for more information.
         * \param[in] lvl The severity level of the given message.
         * \param[in] function The function invoking a particular log method.
         * \param[in] format The format of the given message.
         * \param[in] args A list of arguments to be used by the format message.
         */
        virtual void log(mooned::logging::level lvl, const char* function, const char* format, fmt::format_args args) = 0;

    };

    /** \brief Returns a logger instance to the standard output.
    * \remark The returned object can be used to log messages during global values initialization.
    * \remark The objects is not guaranteed to be thread safe.
    * \returns A logger printing to `stdout`.
    */
    Logger& default_logger();
}

namespace mooned
{
    /** \brief Submits all log params into the given logger.
     */
    void log(logging::Logger& logger, logging::level lvl, const char* function, const char* format, fmt::format_args args);

    /** \brief Submits all log params into the standard output logger.
     */
    template<class... Args>
    inline void log(logging::level lvl, const char* function, const char* format, const Args&... args)
    {
        log(logging::default_logger(), lvl, function, format, fmt::make_format_args(args...));
    }

    /** \brief Logs the message with `debug` severity into the given logger.
     */
    template<class... Args>
    inline void log_debug(logging::Logger& logger, const char* format, const Args&... args)
    {
        log(logger, logging::level::Debug, nullptr, format, fmt::make_format_args(args...));
    }

    /** \brief Logs the message with `debug` severity into the standard output logger.
    */
    template<class... Args>
    inline void log_debug(const char* format, const Args&... args)
    {
        log_debug(logging::default_logger(), format, fmt::make_format_args(args...));
    }

    /** \brief Logs the message with `info` severity into the given logger.
    */
    template<class... Args>
    inline void log_info(logging::Logger& logger, const char* format, const Args&... args)
    {
        log(logger, logging::level::Info, nullptr, format, fmt::make_format_args(args...));
    }

    /** \brief Logs the message with `info` severity into the standard output logger.
    */
    template<class... Args>
    inline void log_info(const char* format, const Args&... args)
    {
        log_info(logging::default_logger(), format, fmt::make_format_args(args...));
    }

    /** \brief Logs the message with `warning` severity into the given logger.
    */
    template<class... Args>
    inline void log_warning(logging::Logger& logger, const char* format, const Args&... args)
    {
        log(logger, logging::level::Warning, nullptr, format, fmt::make_format_args(args...));
    }

    /** \brief Logs the message with `warning` severity into the standard output logger.
    */
    template<class... Args>
    inline void log_warning(const char* format, const Args&... args)
    {
        log_warning(logging::default_logger(), format, fmt::make_format_args(args...));
    }

    /** \brief Logs the message with `error` severity into the given logger.
    */
    template<class... Args>
    inline void log_error(logging::Logger& logger, const char* format, const Args&... args)
    {
        log(logger, logging::level::Error, nullptr, format, fmt::make_format_args(args...));
    }

    /** \brief Logs the message with `error` severity into the standard output logger.
    */
    template<class... Args>
    inline void log_error(const char* format, const Args&... args)
    {
        log_error(logging::default_logger(), format, fmt::make_format_args(args...));
    }
}

/** \brief Logs the message with `info` severity and the calling function name to the standard output logger.
 */
#define MLogInfo(message, ...) mooned::log(mooned::logging::level::Info, __PRETTYFUNCTION__, message, ##__VA_ARGS__)

 /** \brief Logs the message with `debug` severity and the calling function name to the standard output logger.
 */
#define MLogDebug(message, ...) mooned::log(mooned::logging::level::Debug, __PRETTYFUNCTION__, message, ##__VA_ARGS__)

 /** \brief Logs the message with `warning` severity and the calling function name to the standard output logger.
 */
#define MLogWarning(message, ...) mooned::log(mooned::logging::level::Warning, __PRETTYFUNCTION__, message, ##__VA_ARGS__)

 /** \brief Logs the message with `error` severity and the calling function name to the standard output logger.
 */
#define MLogError(message, ...) mooned::log(mooned::logging::level::Error, __PRETTYFUNCTION__, message, ##__VA_ARGS__)


//////////////////////////////////////////////////////////////////////////
// To successful support the `stringid_t` type we need to disable the ConvertToInt check and define the format_arg function in the fmt namespace
namespace fmt
{
    //namespace internal
    //{
    //    template <>
    //    struct ConvertToInt<stringid_t> {
    //        enum { value = ConvertToIntImpl2<stringid_t, false>::value };
    //    };
    //}

    //void format_arg(fmt::BasicFormatter<char> &f, const char *&format_str, const stringid_t& uuid);

#ifdef _DEBUG

namespace v5::internal
{

template <typename C>
FMT_CONSTEXPR typed_value<C, fmt::v5::internal::type::custom_type> make_value(stringid_t val) {
    return val;
}

}

template <>
struct formatter<stringid_t> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const stringid_t &uuid, FormatContext &ctx) {
#ifdef _DEBUG
        return fmt::format_to(ctx.begin(), "{{ 'SID' {:#018x} `{}` }}", uuid._val, uuid._str);
#else
        return fmt::format_to(ctx.begin(), "{{ 'SID' {:#018x} }}", uuid);
#endif
        //return format_to(ctx.begin(), "({:.1f}, {:.1f})", p.x, p.y);
    }
};

#endif

}

//////////////////////////////////////////////////////////////////////////
// Profiling helper struct
namespace mooned
{

class ScopedProfiler
{
public:
    ScopedProfiler(std::string_view name) : _name{ name } {
        _begin = std::chrono::high_resolution_clock::now();
        mooned::log(mooned::logging::level::Info, "profiler", "beg '{}' profiling: {}ms", _name, std::chrono::duration_cast<std::chrono::milliseconds>(_begin.time_since_epoch()).count());
    }
    ~ScopedProfiler() {
        auto end = std::chrono::high_resolution_clock::now();
        mooned::log(mooned::logging::level::Info, "profiler", "end '{}' profiling: {}ms", _name, std::chrono::duration_cast<std::chrono::milliseconds>(end - _begin).count());
    }

private:
    std::chrono::high_resolution_clock::time_point _begin;
    std::string_view _name;
};

}
