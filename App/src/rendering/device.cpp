#include "device.hpp"

#include "buffer.hpp"

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace app::gfx
{
    constexpr u32 FramesInFlight = 2;

    struct Frame
    {
        vk::CommandBuffer cmd{};

        vk::Semaphore imageReadySemaphore{};
        vk::Semaphore renderDoneSemaphore{};
        vk::Fence cmdFence{};
    };

    struct BackBuffer
    {
        vk::Image image{};
        vk::ImageView view{};
    };

    struct Device::DevicePimpl
    {
        vk::DynamicLoader dynamicLoader{};
        vk::Instance instance{};
        vk::PhysicalDevice physicalDevice{};
        vk::Device device{};
        VmaAllocator allocator{};

        u32 graphicsQueueFamily = 0;
        vk::Queue graphicsQueue{};

        vk::DescriptorPool descriptorPool{};

        vk::CommandPool cmdPool{};

        std::array<Frame, FramesInFlight> frames{};
        u32 frameIndex = 0;

        auto get_frame() -> Frame&
        {
            return frames[frameIndex];
        }

        vk::SurfaceKHR surface{};

        vk::Format surfaceFormat{};
        vk::Extent2D extent{};
        vk::SwapchainKHR swapchain{};
        std::vector<BackBuffer> backBuffers{};
        u32 imageIndex = 0;
        bool recreateSwapchain = false;

        auto get_backbuffer() -> BackBuffer&
        {
            return backBuffers[imageIndex];
        }
    };

    namespace
    {
        auto choose_surface_format(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) -> vk::SurfaceFormatKHR
        {
            auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
            ASSERT(surfaceFormats.empty() == false);

            if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
            {
                // There is no preferred format, so any can be chosen
                return { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear };
            }

            // Check if most widely used format is available - R8G8B8A8Srgb
            for (auto& format : surfaceFormats)
            {
                if (format.format == vk::Format::eR8G8B8A8Srgb)
                {
                    return format;
                }
            }

            // Check if an alternative format is available - B8G8R8A8Srgb
            for (auto& format : surfaceFormats)
            {
                if (format.format == vk::Format::eB8G8R8A8Srgb)
                {
                    return format;
                }
            }

            // Return the first available format
            return surfaceFormats[0];
        }

        auto choose_present_mode(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, bool vsync) -> vk::PresentModeKHR
        {
            auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

            if (!vsync)
            {
                // Check if Immediate (not vsync) is available
                for (auto mode : presentModes)
                {
                    if (mode == vk::PresentModeKHR::eImmediate)
                    {
                        return vk::PresentModeKHR::eImmediate;
                    }
                }
            }
            else
            {
                // Check if Mailbox (vsync) mode is available
                for (auto mode : presentModes)
                {
                    if (mode == vk::PresentModeKHR::eMailbox)
                    {
                        return vk::PresentModeKHR::eMailbox;
                    }
                }
            }

            // Fifo (vsync) is always available
            return vk::PresentModeKHR::eFifo;
        }

        auto choose_extent(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, u32 width, u32 height) -> vk::Extent2D
        {
            auto capabilties = physicalDevice.getSurfaceCapabilitiesKHR(surface);

            vk::Extent2D extent{ width, height };

            extent.width = std::clamp(extent.width, capabilties.minImageExtent.width, capabilties.maxImageExtent.width);
            extent.height = std::clamp(extent.height, capabilties.minImageExtent.height, capabilties.maxImageExtent.height);

            return extent;
        }

        void clean_swapchain(Device::DevicePimpl& pimpl)
        {
            for (auto& backbuffer : pimpl.backBuffers)
            {
                pimpl.device.destroy(backbuffer.view);
            }
            pimpl.backBuffers.clear();
        }

        void create_swapchain(Device::DevicePimpl& pimpl, u32 width, u32 height)
        {
            auto capabilities = pimpl.physicalDevice.getSurfaceCapabilitiesKHR(pimpl.surface);

            auto surfaceFormat = choose_surface_format(pimpl.physicalDevice, pimpl.surface);
            auto extent = choose_extent(pimpl.physicalDevice, pimpl.surface, width, height);
            auto presentMode = choose_present_mode(pimpl.physicalDevice, pimpl.surface, true);

            auto transform = capabilities.currentTransform;

            u32 minImageCount = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount != -1 && capabilities.maxImageCount < minImageCount)
            {
                minImageCount = capabilities.maxImageCount;
            }

            auto oldSwapchain = pimpl.swapchain;

            vk::SwapchainCreateInfoKHR swapchain_info{};
            swapchain_info.setSurface(pimpl.surface);
            swapchain_info.setMinImageCount(minImageCount);
            swapchain_info.setImageFormat(surfaceFormat.format);
            swapchain_info.setImageColorSpace(surfaceFormat.colorSpace);
            swapchain_info.setImageExtent(extent);
            swapchain_info.setImageArrayLayers(1);
            swapchain_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
            swapchain_info.setPreTransform(transform);
            swapchain_info.setPresentMode(presentMode);
            swapchain_info.setClipped(true);
            swapchain_info.setOldSwapchain(oldSwapchain);

            pimpl.swapchain = pimpl.device.createSwapchainKHR(swapchain_info);

            if (oldSwapchain)
            {
                pimpl.device.destroy(oldSwapchain);
                clean_swapchain(pimpl);
            }

            pimpl.surfaceFormat = surfaceFormat.format;
            pimpl.extent = extent;

            auto images = pimpl.device.getSwapchainImagesKHR(pimpl.swapchain);
            for (auto image : images)
            {
                auto& backbuffer = pimpl.backBuffers.emplace_back();
                backbuffer.image = image;

                vk::ImageViewCreateInfo view_info{};
                view_info.setImage(image);
                view_info.setFormat(surfaceFormat.format);
                view_info.setViewType(vk::ImageViewType::e2D);
                view_info.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
                view_info.subresourceRange.setBaseArrayLayer(0);
                view_info.subresourceRange.setLayerCount(1);
                view_info.subresourceRange.setBaseMipLevel(0);
                view_info.subresourceRange.setLevelCount(1);
                backbuffer.view = pimpl.device.createImageView(view_info);
            }
        }

        void transition_image(vk::CommandBuffer cmd,
                              vk::Image image,
                              vk::ImageLayout oldLayout,
                              vk::ImageLayout newLayout,
                              vk::AccessFlags srcAccess,
                              vk::AccessFlags dstAccess,
                              vk::PipelineStageFlags srcStage,
                              vk::PipelineStageFlags dstStage)
        {
            vk::ImageSubresourceRange range{};
            range.setAspectMask(vk::ImageAspectFlagBits::eColor);
            range.setBaseArrayLayer(0);
            range.setLayerCount(1);
            range.setBaseMipLevel(0);
            range.setLevelCount(1);

            vk::ImageMemoryBarrier barrier{};
            barrier.setImage(image);
            barrier.setOldLayout(oldLayout);
            barrier.setNewLayout(newLayout);
            barrier.setSubresourceRange(range);
            barrier.setSrcAccessMask(srcAccess);
            barrier.setDstAccessMask(dstAccess);

            cmd.pipelineBarrier(srcStage, dstStage, {}, {}, {}, barrier);
        }

        void transition_image_to_color_attachment(vk::CommandBuffer cmd, vk::Image image)
        {
            transition_image(cmd,
                             image,
                             vk::ImageLayout::eUndefined,
                             vk::ImageLayout::eColorAttachmentOptimal,
                             {},
                             vk::AccessFlagBits::eColorAttachmentWrite,
                             vk::PipelineStageFlagBits::eTopOfPipe,
                             vk::PipelineStageFlagBits::eColorAttachmentOutput);
        }

        void transition_image_to_present_src(vk::CommandBuffer cmd, vk::Image image)
        {
            transition_image(cmd,
                             image,
                             vk::ImageLayout::eColorAttachmentOptimal,
                             vk::ImageLayout::ePresentSrcKHR,
                             vk::AccessFlagBits::eColorAttachmentWrite,
                             {},
                             vk::PipelineStageFlagBits::eColorAttachmentOutput,
                             vk::PipelineStageFlagBits::eBottomOfPipe);
        }
    }

    Device::Device() : m_pimpl(new DevicePimpl) {}

    Device::~Device()
    {
        shutdown();
    }

    Device::operator bool() const
    {
        return m_pimpl->device;
    }

    void Device::init(void* native_window_handle)
    {
        // Create instance
        {
            vk::ApplicationInfo appInfo{};
            appInfo.setApiVersion(VK_API_VERSION_1_3);

            std::vector<const char*> layers{
                "VK_LAYER_KHRONOS_validation",
            };

            std::vector<const char*> extensions{
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            };

            vk::InstanceCreateInfo instance_info{};
            instance_info.setPApplicationInfo(&appInfo);
            instance_info.setPEnabledLayerNames(layers);
            instance_info.setPEnabledExtensionNames(extensions);
            m_pimpl->instance = vk::createInstance(instance_info);
        }

        // Pick physical device
        {
            auto physicalDevices = m_pimpl->instance.enumeratePhysicalDevices();
            ASSERT(physicalDevices.empty() == false);

            // #TODO: Pick best physical device

            m_pimpl->physicalDevice = physicalDevices[0];
        }

        // Create device
        {
            std::vector<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

            static f32 queue_priority = 1.0f;
            std::vector<vk::DeviceQueueCreateInfo> queue_infos(1);
            queue_infos[0].setQueuePriorities(queue_priority);
            queue_infos[0].setQueueCount(1);
            queue_infos[0].setQueueFamilyIndex(m_pimpl->graphicsQueueFamily);

            vk::PhysicalDeviceFeatures enabled_features{};

            vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{};
            dynamic_rendering_features.setDynamicRendering(true);

            vk::DeviceCreateInfo create_info{};
            create_info.setPEnabledExtensionNames(extensions);
            create_info.setQueueCreateInfos(queue_infos);
            create_info.setPEnabledFeatures(&enabled_features);
            create_info.setPNext(&dynamic_rendering_features);

            m_pimpl->device = m_pimpl->physicalDevice.createDevice(create_info);

            m_pimpl->graphicsQueue = m_pimpl->device.getQueue(m_pimpl->graphicsQueueFamily, 0);
        }

        // Create allocator
        {
            VmaAllocatorCreateInfo alloc_info{};
            alloc_info.instance = m_pimpl->instance;
            alloc_info.physicalDevice = m_pimpl->physicalDevice;
            alloc_info.device = m_pimpl->device;
            alloc_info.vulkanApiVersion = VK_API_VERSION_1_3;
            vmaCreateAllocator(&alloc_info, &m_pimpl->allocator);
        }

        // Create descriptor pool
        {
            std::vector<vk::DescriptorPoolSize> pool_sizes{
                { vk::DescriptorType::eUniformBuffer, 1000 },
                { vk::DescriptorType::eCombinedImageSampler, 1000 },
            };

            vk::DescriptorPoolCreateInfo pool_info{};
            pool_info.setMaxSets(1000);
            pool_info.setPoolSizes(pool_sizes);
            m_pimpl->descriptorPool = m_pimpl->device.createDescriptorPool(pool_info);
        }

        // Create command pool
        {
            vk::CommandPoolCreateInfo pool_info{};
            pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
            pool_info.setQueueFamilyIndex(m_pimpl->graphicsQueueFamily);
            m_pimpl->cmdPool = m_pimpl->device.createCommandPool(pool_info);
        }

        // Setup frames
        {
            for (auto& frame : m_pimpl->frames)
            {
                vk::CommandBufferAllocateInfo alloc_info{};
                alloc_info.setCommandBufferCount(1);
                alloc_info.setCommandPool(m_pimpl->cmdPool);
                alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
                frame.cmd = m_pimpl->device.allocateCommandBuffers(alloc_info)[0];

                frame.imageReadySemaphore = m_pimpl->device.createSemaphore({});
                frame.renderDoneSemaphore = m_pimpl->device.createSemaphore({});
                frame.cmdFence = m_pimpl->device.createFence({ vk::FenceCreateFlagBits::eSignaled });
            }
        }

        // Create surface
        {
#if defined(WIN32)
            vk::Win32SurfaceCreateInfoKHR surfaceInfo{};
            surfaceInfo.hinstance = GetModuleHandle(nullptr);
            surfaceInfo.hwnd = static_cast<HWND>(native_window_handle);
            m_pimpl->surface = m_pimpl->instance.createWin32SurfaceKHR(surfaceInfo);
#endif
        }

        // Create swapchain
        {
            create_swapchain(*m_pimpl, 1600, 900);
        }
    }

    void Device::shutdown()
    {
        if (!m_pimpl->device)
        {
            return;
        }

        m_pimpl->device.waitIdle();

        clean_swapchain(*m_pimpl);

        for (auto& frame : m_pimpl->frames)
        {
            m_pimpl->device.destroy(frame.imageReadySemaphore);
            m_pimpl->device.destroy(frame.renderDoneSemaphore);
            m_pimpl->device.destroy(frame.cmdFence);
        }

        m_pimpl->device.destroy(m_pimpl->cmdPool);
        m_pimpl->device.destroy(m_pimpl->descriptorPool);

        vmaDestroyAllocator(m_pimpl->allocator);
        m_pimpl->allocator = nullptr;

        m_pimpl->device.destroy();
        m_pimpl->device = nullptr;

        m_pimpl->instance.destroy(m_pimpl->surface);
        m_pimpl->instance.destroy();
        m_pimpl->instance = nullptr;
    }

    auto Device::get_instance() const -> vk::Instance
    {
        return m_pimpl->instance;
    }

    auto Device::get_physical_device() const -> vk::PhysicalDevice
    {
        return m_pimpl->physicalDevice;
    }

    auto Device::get_device() const -> vk::Device
    {
        return m_pimpl->device;
    }

    auto Device::get_graphics_family() -> u32
    {
        return m_pimpl->graphicsQueueFamily;
    }

    auto Device::get_graphics_queue() -> vk::Queue
    {
        return m_pimpl->graphicsQueue;
    }

    auto Device::get_allocator() const -> VmaAllocator
    {
        return m_pimpl->allocator;
    }

    auto Device::get_descriptor_pool() -> vk::DescriptorPool
    {
        return m_pimpl->descriptorPool;
    }

    auto Device::get_swapchain_format() -> vk::Format
    {
        return m_pimpl->surfaceFormat;
    }

    auto Device::get_swapchain_image_count() -> u32
    {
        return static_cast<u32>(m_pimpl->backBuffers.size());
    }

    auto Device::get_current_cmd() const -> vk::CommandBuffer
    {
        return m_pimpl->frames[m_pimpl->frameIndex].cmd;
    }

    void Device::wait_idle()
    {
        ASSERT(m_pimpl->device);
        m_pimpl->device.waitIdle();
    }

    auto Device::begin_single_use_cmd() -> vk::CommandBuffer
    {
        vk::CommandBufferAllocateInfo alloc_info{};
        alloc_info.setCommandBufferCount(1);
        alloc_info.setCommandPool(m_pimpl->cmdPool);
        alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
        auto cmd = m_pimpl->device.allocateCommandBuffers(alloc_info)[0];

        vk::CommandBufferBeginInfo begin_info{};
        cmd.begin(begin_info);

        return cmd;
    }

    void Device::end_single_use_cmd(vk::CommandBuffer cmd)
    {
        cmd.end();

        auto fence = m_pimpl->device.createFence({});

        vk::SubmitInfo submit_info{};
        submit_info.setCommandBuffers(cmd);
        m_pimpl->graphicsQueue.submit(submit_info, fence);

        UNUSED(m_pimpl->device.waitForFences(fence, true, u64_max));

        m_pimpl->device.destroy(fence);
        m_pimpl->device.freeCommandBuffers(m_pimpl->cmdPool, cmd);
    }

    void Device::new_frame()
    {
        if (m_pimpl->recreateSwapchain)
        {
            // Recreate swapchain
            create_swapchain(*m_pimpl, 1600, 900);
        }

        auto& frame = m_pimpl->get_frame();

        auto result = m_pimpl->device.acquireNextImageKHR(m_pimpl->swapchain, u64_max, frame.imageReadySemaphore);
        if (result.result == vk::Result::eErrorOutOfDateKHR || result.result == vk::Result::eSuboptimalKHR)
        {
            m_pimpl->recreateSwapchain = true;
            return;
        }
        m_pimpl->imageIndex = result.value;

        UNUSED(m_pimpl->device.waitForFences(frame.cmdFence, true, u64_max));
        m_pimpl->device.resetFences(frame.cmdFence);

        frame.cmd.reset();

        vk::CommandBufferBeginInfo begin_info{};
        frame.cmd.begin(begin_info);
    }

    void Device::flush_frame()
    {
        if (m_pimpl->recreateSwapchain)
        {
            return;
        }

        auto& frame = m_pimpl->get_frame();

        frame.cmd.end();

        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submit_info{};
        submit_info.setWaitSemaphores(frame.imageReadySemaphore);
        submit_info.setWaitDstStageMask(wait_stage);
        submit_info.setCommandBuffers(frame.cmd);
        submit_info.setSignalSemaphores(frame.renderDoneSemaphore);
        m_pimpl->graphicsQueue.submit(submit_info, frame.cmdFence);

        vk::PresentInfoKHR present_info{};
        present_info.setWaitSemaphores(frame.renderDoneSemaphore);
        present_info.setSwapchains(m_pimpl->swapchain);
        present_info.setImageIndices(m_pimpl->imageIndex);
        auto result = m_pimpl->graphicsQueue.presentKHR(present_info);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
        {
            m_pimpl->recreateSwapchain = true;
            return;
        }

        m_pimpl->frameIndex = (m_pimpl->frameIndex + 1) % FramesInFlight;
    }

    void Device::begin_backbuffer_pass(const glm::vec4& clear_color)
    {
        auto cmd = get_current_cmd();
        const auto& backbuffer = m_pimpl->backBuffers[m_pimpl->imageIndex];

        transition_image_to_color_attachment(cmd, backbuffer.image);

        std::array<float, 4> color_array{ clear_color.x, clear_color.y, clear_color.z, clear_color.w };
        vk::ClearColorValue clearValue(color_array);

        vk::RenderingAttachmentInfo attachment_info{};
        attachment_info.setImageView(backbuffer.view);
        attachment_info.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
        attachment_info.setClearValue(clearValue);
        attachment_info.setLoadOp(vk::AttachmentLoadOp::eClear);
        attachment_info.setStoreOp(vk::AttachmentStoreOp::eStore);

        vk::RenderingInfo rendering_info{};
        rendering_info.setColorAttachments(attachment_info);
        rendering_info.setLayerCount(1);
        rendering_info.setRenderArea(vk::Rect2D({ 0, 0 }, m_pimpl->extent));
        cmd.beginRendering(rendering_info);
    }

    void Device::end_backbuffer_pass()
    {
        auto cmd = get_current_cmd();
        auto& backbuffer = m_pimpl->get_backbuffer();

        cmd.endRendering();

        transition_image_to_present_src(cmd, backbuffer.image);
    }

}
