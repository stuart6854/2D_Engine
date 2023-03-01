#pragma once

#include "core/core.hpp"

#include <vulkan/vulkan.hpp>

#include <string>

namespace app::gfx
{
    class Device;

    class Texture
    {
    public:
        Texture(Device* device);
        ~Texture();

        void init(const std::string& filename);
        void destroy();

        /* Getters */

        bool is_valid() const;

        auto get_width() const -> u32;
        auto get_height() const -> u32;

        auto get_set() const -> vk::DescriptorSet;

    private:
        struct TexturePimpl;
        Owned<TexturePimpl> m_pimpl = nullptr;
    };
}