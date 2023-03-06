#include "texture_atlas.hpp"

#include "rendering/renderer.hpp"
#include "rendering/texture.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <fstream>
#include <string>

namespace app::game
{
    void TextureAtlas::init(gfx::Renderer* renderer, const std::string& atlas_file)
    {
        m_texture = nullptr;
        m_sprites = {};
        m_spriteMap = {};

        std::ifstream file(atlas_file);
        auto data = json::parse(file);

        auto texture_file = data["texture"].get<std::string>();
        std::filesystem::path texture_path = texture_file;
        if (texture_path.is_relative())
        {
            texture_path = atlas_file;
            texture_path = texture_path.parent_path();
            texture_path /= texture_file;
        }

        // Load atlas texture
        m_texture = renderer->create_texture();
        m_texture->init(texture_path.string());
        const glm::vec2 textureSize = { m_texture->get_width(), m_texture->get_height() };

        auto sprites = data["sprites"];
        for (auto& sprite_data : sprites)
        {
            auto& sprite = m_sprites.emplace_back();
            sprite.Name = sprite_data["name"].get<std::string>();
            sprite.x = sprite_data["x"].get<u32>();
            sprite.y = sprite_data["y"].get<u32>();
            sprite.width = sprite_data["w"].get<u32>();
            sprite.height = sprite_data["h"].get<u32>();
            sprite.MinUV = glm::vec2{ sprite.x, sprite.y } / textureSize;
            sprite.MaxUV = sprite.MinUV + glm::vec2{ sprite.width, sprite.height } / textureSize;

            m_spriteMap[sprite.Name] = static_cast<u32>(m_sprites.size() - 1);
        }
    }

    auto TextureAtlas::get_texture() const -> gfx::Texture*
    {
        return m_texture.get();
    }

    auto TextureAtlas::get_sprite(const std::string& name) const -> const Sprite&
    {
        const auto it = m_spriteMap.find(name);
        ASSERT(it != m_spriteMap.end());

        const auto index = it->second;
        auto& sprite = m_sprites[static_cast<sizet>(index)];
        return sprite;
    }

}
