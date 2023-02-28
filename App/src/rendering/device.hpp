#pragma once

#include "core/core.hpp"

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

struct GLFWwindow;

namespace app::gfx
{
    class Buffer;

    class Device
    {
    public:
        struct DevicePimpl;

        Device();
        ~Device();

        /* Operators */

        operator bool() const;

        /* Initialisation/Shutdown */

        void init(void* native_window_handle);
        void shutdown();

        /* Getters */

        auto get_instance() const -> vk::Instance;
        auto get_physical_device() const -> vk::PhysicalDevice;
        auto get_device() const -> vk::Device;

        auto get_graphics_family() -> u32;
        auto get_graphics_queue() -> vk::Queue;

        auto get_allocator() const -> VmaAllocator;

        auto get_descriptor_pool() -> vk::DescriptorPool;

        auto get_swapchain_format() -> vk::Format;
        auto get_swapchain_image_count() -> u32;

        auto get_current_cmd() const -> vk::CommandBuffer;

        /* Commands */

        void wait_idle();

        auto begin_single_use_cmd() -> vk::CommandBuffer;
        void end_single_use_cmd(vk::CommandBuffer cmd);

        void upload_to_image(vk::Image image, vk::Extent3D image_extent, sizet size, const void* data);

        void new_frame();
        void flush_frame();

        void begin_backbuffer_pass(const glm::vec4& clear_color);
        void end_backbuffer_pass();

    private:
        Owned<DevicePimpl> m_pimpl;
    };
}