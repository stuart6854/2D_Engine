#include "buffer.hpp"

#include "device.hpp"

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace app::gfx
{
    struct Buffer::BufferPimpl
    {
        Device* device = nullptr;

        vk::Buffer buffer{};
        VmaAllocation allocation{};
        sizet size = 0;
        bool isMappable = false;
    };

    Buffer::Buffer(Device* device) : m_pimpl(new BufferPimpl)
    {
        m_pimpl->device = device;
    }

    Buffer::~Buffer()
    {
        destroy();
    }

    void Buffer::init(sizet size, vk::BufferUsageFlags usage, bool force_mappable)
    {
        destroy();

        m_pimpl->size = size;

        auto allocator = m_pimpl->device->get_allocator();

        VkBufferCreateInfo buffer_info = vk::BufferCreateInfo();
        buffer_info.size = size;
        buffer_info.usage = static_cast<VkBufferUsageFlags>(usage);

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        if (force_mappable || usage & vk::BufferUsageFlagBits::eTransferSrc)
        {
            alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            m_pimpl->isMappable = true;
        }

        VkBuffer vkBuffer = nullptr;
        vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &vkBuffer, &m_pimpl->allocation, nullptr);

        m_pimpl->buffer = vkBuffer;
    }

    void Buffer::destroy()
    {
        if (!m_pimpl->buffer)
        {
            return;
        }

        auto allocator = m_pimpl->device->get_allocator();

        vmaDestroyBuffer(allocator, m_pimpl->buffer, m_pimpl->allocation);

        m_pimpl->buffer = nullptr;
        m_pimpl->allocation = nullptr;
        m_pimpl->size = 0;
    }

    auto Buffer::get_buffer() const -> vk::Buffer
    {
        return m_pimpl->buffer;
    }

    auto Buffer::get_size() const -> sizet
    {
        return m_pimpl->size;
    }

    void Buffer::write_data(sizet offset, sizet size, const void* data)
    {
        if (m_pimpl->isMappable)
        {
            auto allocator = m_pimpl->device->get_allocator();

            byte* mapped_ptr = nullptr;
            vmaMapMemory(allocator, m_pimpl->allocation, reinterpret_cast<void**>(&mapped_ptr));
            std::memcpy(mapped_ptr + offset, data, size);
            vmaUnmapMemory(allocator, m_pimpl->allocation);
        }
        else
        {
            // Staging Buffer

            // #TODO: Upload using staging buffer
        }
    }

}
