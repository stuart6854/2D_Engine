#include "shader.hpp"

#include "device.hpp"

#include <vulkan/vulkan.hpp>

#include <glm/ext/matrix_float4x4.hpp>

#include <string>
#include <fstream>

namespace app::gfx
{
    namespace
    {
        auto read_spirv_file(const std::string& filename) -> std::vector<u32>
        {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open())
            {
                LOG_ERROR("Shader - Failed to open SPIRV file <{}>!", filename);
                return {};
            }

            size_t fileSize = file.tellg();
            std::vector<u32> buffer(fileSize / sizeof(u32));

            file.seekg(0);
            file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

            file.close();

            return buffer;
        }
    }

    struct Shader::ShaderPimpl
    {
        Device* device = nullptr;

        std::string vertex_file;
        std::string fragment_file;

        vk::PipelineLayout layout{};
        vk::Pipeline pipeline{};
    };

    Shader::Shader(Device* device) : m_pimpl(new ShaderPimpl)
    {
        m_pimpl->device = device;
    }

    Shader::~Shader()
    {
        destroy();
    }

    void Shader::init(const std::string& vertex_file, const std::string& fragment_file)
    {
        destroy();

        auto device = m_pimpl->device->get_device();

        {
            vk::PushConstantRange const_range{};
            const_range.setOffset(0);
            const_range.setSize(sizeof(glm::mat4) * 2);
            const_range.setStageFlags(vk::ShaderStageFlagBits::eVertex);

            vk::DescriptorSetLayoutBinding binding{};
            binding.setBinding(0);
            binding.setDescriptorCount(32);
            binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

            vk::DescriptorSetLayoutCreateInfo set_info{};
            set_info.setBindings(binding);

            vk::PipelineLayoutCreateInfo layout_info{};
            layout_info.setPushConstantRanges(const_range);
            m_pimpl->layout = device.createPipelineLayout(layout_info);
        }

        auto vert_spv_code = read_spirv_file(vertex_file);
        if (vert_spv_code.empty())
        {
            return;
        }
        vk::ShaderModuleCreateInfo moduleInfo{};
        moduleInfo.setCode(vert_spv_code);
        vk::ShaderModule vertShaderModule = device.createShaderModule(moduleInfo);

        auto frag_spv_code = read_spirv_file(fragment_file);
        if (frag_spv_code.empty())
        {
            return;
        }
        moduleInfo.setCode(frag_spv_code);
        vk::ShaderModule fragShaderModule = device.createShaderModule(moduleInfo);

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

        std::vector<vk::VertexInputAttributeDescription> attributes(4);
        attributes[0].setBinding(0);
        attributes[0].setLocation(0);
        attributes[0].setFormat(vk::Format::eR32G32B32Sfloat);
        attributes[0].setOffset(0);
        attributes[1].setBinding(0);
        attributes[1].setLocation(1);
        attributes[1].setFormat(vk::Format::eR32G32B32A32Sfloat);
        attributes[1].setOffset(12);
        attributes[2].setBinding(0);
        attributes[2].setLocation(2);
        attributes[2].setFormat(vk::Format::eR32G32Sfloat);
        attributes[2].setOffset(28);
        attributes[3].setBinding(0);
        attributes[3].setLocation(3);
        attributes[3].setFormat(vk::Format::eR32Sfloat);
        attributes[3].setOffset(36);

        vk::VertexInputBindingDescription binding{};
        binding.setBinding(0);
        binding.setInputRate(vk::VertexInputRate::eVertex);
        binding.setStride(40);

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        if (!attributes.empty())
        {
            vertexInputInfo.setVertexBindingDescriptions(binding);
            vertexInputInfo.setVertexAttributeDescriptions(attributes);
        }

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        vk::Viewport viewport{};
        vk::Rect2D scissor{};

        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.setViewports(viewport);
        viewportState.setScissors(scissor);

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eClockwise;
        rasterizer.depthBiasEnable = VK_FALSE;

        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

        // Depth/Stencil State

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.setBlendEnable(true);
        colorBlendAttachment.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
        colorBlendAttachment.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
        colorBlendAttachment.setColorBlendOp(vk::BlendOp::eAdd);
        colorBlendAttachment.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
        colorBlendAttachment.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
        colorBlendAttachment.setAlphaBlendOp(vk::BlendOp::eAdd);
        colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

        vk::PipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::vector dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.setDynamicStates(dynamicStates);

        vk::Format colorFormat = m_pimpl->device->get_swapchain_format();
        vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
        pipelineRenderingInfo.setColorAttachmentFormats(colorFormat);

        vk::GraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.stageCount = static_cast<u32>(shaderStages.size());
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pNext = &pipelineRenderingInfo;
        pipeline_info.pVertexInputState = &vertexInputInfo;
        pipeline_info.pInputAssemblyState = &inputAssembly;
        pipeline_info.pViewportState = &viewportState;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = nullptr;
        pipeline_info.pColorBlendState = &colorBlending;
        pipeline_info.pDynamicState = &dynamicState;
        pipeline_info.layout = m_pimpl->layout;
        pipeline_info.subpass = 0;

        m_pimpl->pipeline = device.createGraphicsPipeline({}, pipeline_info).value;

        device.destroy(vertShaderModule);
        device.destroy(fragShaderModule);
    }

    void Shader::destroy()
    {
        if (!m_pimpl->pipeline)
        {
            return;
        }

        auto device = m_pimpl->device->get_device();

        device.destroy(m_pimpl->pipeline);
        device.destroy(m_pimpl->layout);
    }

    bool Shader::is_valid() const
    {
        return m_pimpl->pipeline;
    }

    auto Shader::get_layout() const -> vk::PipelineLayout
    {
        return m_pimpl->layout;
    }

    auto Shader::get_pipeline() const -> vk::Pipeline
    {
        return m_pimpl->pipeline;
    }

}
