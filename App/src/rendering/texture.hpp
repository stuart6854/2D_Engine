#pragma once

#include "core/core.hpp"

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

    private:
        struct TexturePimpl;
        Owned<TexturePimpl> m_pimpl = nullptr;
    };
}