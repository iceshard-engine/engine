#include <ice/render/render_command_buffer.hxx>
#include "vk_include.hxx"

static_assert(sizeof(ice::render::CommandBuffer) == sizeof(VkCommandBuffer));
static_assert(alignof(ice::render::CommandBuffer) == alignof(VkCommandBuffer));
