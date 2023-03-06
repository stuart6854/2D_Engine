#pragma once

#include "core/core.hpp"

#include <string>

namespace app
{
    namespace gfx
    {
        class Renderer;
        class Texture;
    }

    namespace game
    {
        struct Sprite
        {
            std::string Name{};
            u32 x = 0;
            u32 y = 0;
            u32 width = 0;
            u32 height = 0;

            glm::vec2 MinUV{};
            glm::vec2 MaxUV{};
        };

        class TextureAtlas
        {
        public:
            TextureAtlas() = default;
            ~TextureAtlas() = default;

            void init(gfx::Renderer* renderer, const std::string& atlas_file);

            auto get_texture() const -> gfx::Texture*;
            auto get_sprite(const std::string& name) const -> const Sprite&;

        private:
            Shared<gfx::Texture> m_texture = nullptr;

            std::vector<Sprite> m_sprites{};
            std::unordered_map<std::string, u32> m_spriteMap{};
        };
    }
}