#pragma once
#include <ice/base.hxx>

namespace ice::render
{

    class RenderDriver;
    class RenderDevice;
    class RenderSurface;
    class RenderSwapchain;
    class RenderQueue;
    class RenderCommands;

    struct SurfaceInfo;
    struct RenderpassInfo;
    struct ResourceSetLayoutBinding;
    struct ResourceSetUpdateInfo;
    struct PipelineLayoutInfo;
    struct PipelineInfo;
    struct QueueFamilyInfo;
    struct QueueInfo;
    struct ShaderInfo;
    struct ImageInfo;
    struct BufferUpdateInfo;

    enum class Framebuffer : ice::uptr;
    enum class Renderpass : ice::uptr;
    enum class ResourceSetLayout : ice::uptr;
    enum class ResourceSet : ice::uptr;
    enum class PipelineLayout : ice::uptr;
    enum class Pipeline : ice::uptr;
    enum class Shader : ice::uptr;
    enum class Image : ice::uptr;
    enum class Buffer : ice::uptr;
    enum class CommandBuffer : ice::uptr;
    enum class Semaphore : ice::uptr;
    enum class Fence : ice::uptr;

    enum class QueueID : ice::u32;
    enum class ImageFormat : ice::u32;
    enum class BufferType : ice::u32;
    enum class ShaderStageFlags : ice::u32;

} // namespace ice::render
