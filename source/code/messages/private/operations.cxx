#include <core\message\operations.hxx>
#include <core\message\buffer.hxx>
#include <numeric>

namespace core::message
{

    bool empty(const MessageBuffer& buffer) noexcept
    {
        return buffer.count() == 0;
    }

    auto count(const MessageBuffer& buffer) noexcept -> uint32_t
    {
        return buffer.count();
    }

    void push(MessageBuffer& buffer, core::stringid_arg_type message_type) noexcept
    {
        buffer.push(message_type);
    }

    void push(MessageBuffer& buffer, core::stringid_arg_type message_type, core::data_view_aligned message_data) noexcept
    {
        buffer.push(message_type, message_data);
    }

    void clear(MessageBuffer& buffer) noexcept
    {
        buffer.clear();
    }

    void for_each(const MessageBuffer& buffer, std::function<void(const core::Message&)> callback) noexcept
    {
        std::for_each(buffer.begin(), buffer.end(), callback);
    }

    void filter(const MessageBuffer& buffer, const std::vector<core::stringid_type>& valid_types, std::function<void(const core::Message&)> callback) noexcept
    {
        const auto is_valid_type = [&](core::stringid_arg_type message_type) noexcept -> bool
        {
            return std::find_if(valid_types.begin(), valid_types.end(), [&](core::stringid_arg_type valid_type) noexcept
                {
                    return valid_type == message_type;
                }) != valid_types.end();
        };

        core::message::for_each(buffer, [&](const core::Message& msg) noexcept
            {
                if (is_valid_type(msg.header.type))
                {
                    callback(msg);
                }
            });
    }

    void filter(const MessageBuffer& buffer, core::stringid_arg_type type, std::function<void(const core::Message&)> callback) noexcept
    {
        core::message::for_each(buffer, [&](const core::Message& msg) noexcept
            {
                if (msg.header.type == type)
                {
                    callback(msg);
                }
            });
    }

} // namespace core::message
