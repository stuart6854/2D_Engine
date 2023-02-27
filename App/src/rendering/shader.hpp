#pragma once

#include "core/core.hpp"

#include <vulkan/vulkan.hpp>

#include <string>

namespace app::gfx
{
    class Device;

    class Shader
    {
    public:
        explicit Shader(Device* device);
        ~Shader();

        /* Initialisation/Destruction */

        void init(const std::string& vertex_file, const std::string& fragment_file);
        void destroy();

        /* Getters */

        bool is_valid() const;

        auto get_layout() const -> vk::PipelineLayout;
        auto get_pipeline() const -> vk::Pipeline;

        /* Commands */

    private:
        struct ShaderPimpl;
        Owned<ShaderPimpl> m_pimpl = nullptr;
    };
}