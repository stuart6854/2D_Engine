#pragma once

#include "core/core.hpp"

#include <vulkan/vulkan.hpp>

namespace app::gfx
{
    class Device;

    class Buffer
    {
    public:
        explicit Buffer(Device* device);
        ~Buffer();

        /* Initialization/Destruction */

        void init(sizet size, vk::BufferUsageFlags usage, bool force_mappable = false);
        void destroy();

        /* Getters */

        auto get_buffer() const -> vk::Buffer;

        auto get_size() const -> sizet;

        /* Commands */

        void write_data(sizet offset, sizet size, const void* data);

    private:
        struct BufferPimpl;
        Owned<BufferPimpl> m_pimpl = nullptr;
    };
}