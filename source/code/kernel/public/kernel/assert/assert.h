#pragma once
#include <string>
#include <vector>
#include <mutex>

#define DEBUG_BREAK() __debugbreak()
#define ASSERT_UNUSED(x) do { (void)sizeof(x); } while(0)

namespace assert
{
    enum class FailBehaviour
    {
        BREAK,
        ABORT,
    };

    bool AssertionInfo(const std::string& file_and_line, bool update = false);

    class SmartAssertData;
    using SmartAssertHandler = FailBehaviour(*)(const SmartAssertData&);

    class SmartAssert
    {
    public:
        static SmartAssertHandler& get_handler();
        static SmartAssertHandler set_handler(SmartAssertHandler handler);

        SmartAssert() = default;
        ~SmartAssert() = default;

        bool report_failure(const char* file, int line, const char* condition, const char* message, ...);

    protected:
        FailBehaviour report_failure(const SmartAssertData& data);

    private:
        std::mutex _mtx;
    };

    struct StackTraceSymbol
    {
        std::string name;
        uint64_t address;
    };

    class SmartAssertData
    {
    public:
        SmartAssertData() = default;
        ~SmartAssertData() = default;

        void set_message(std::string mess);
        void set_condition(std::string cond);
        void set_source(std::string filename, int line);
        void set_stacktrace(std::vector<StackTraceSymbol> trace);

        std::string message() const;
        std::string condition() const;
        std::string file_name() const;
        int file_line() const;

        std::vector<StackTraceSymbol> stacktrace() const;

    private:
        std::string _message;
        std::string _condition;
        std::string _file;
        int32_t _line;

        std::vector<StackTraceSymbol> _stacktrace;
    };
}
