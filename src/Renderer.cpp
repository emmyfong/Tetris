#include "Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Font.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace {

std::vector<char> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    return buffer;
}

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
}

// Worst case: every board cell filled plus the falling piece, plus grid
// line quads, plus a generous allowance for UI panel elements (borders +
// piece previews), 6 vertices per quad (2 triangles).
constexpr size_t kMaxVertices =
    (Settings::kBoardWidth * Settings::kBoardHeight + 16 +
     (Settings::kBoardWidth + 1) + (Settings::kBoardHeight + 1) + 100) * 6;

// Generous allowance for a few short lines of UI text, 6 vertices per
// character quad.
constexpr size_t kMaxTextVertices = 600 * 6;

} // namespace

uint32_t Renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device.physical_device.physical_device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find a suitable Vulkan memory type");
}

VkCommandBuffer Renderer::beginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device.device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    return cmd;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer cmd) const {
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    // Synchronous: fine for one-time startup work, not something we'd do
    // every frame.
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device.device, commandPool, 1, &cmd);
}

Renderer::Renderer(GLFWwindow* window) {
    vkb::InstanceBuilder instanceBuilder;
    auto instanceResult = instanceBuilder
        .set_app_name("Tetris")
        .request_validation_layers()
        .use_default_debug_messenger()
        .require_api_version(1, 2, 0)
        .build();
    if (!instanceResult) {
        throw std::runtime_error("Failed to create Vulkan instance: " + instanceResult.error().message());
    }
    instance = instanceResult.value();

    if (glfwCreateWindowSurface(instance.instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan surface");
    }

    vkb::PhysicalDeviceSelector physicalDeviceSelector{ instance };
    auto physicalDeviceResult = physicalDeviceSelector.set_surface(surface).select();
    if (!physicalDeviceResult) {
        throw std::runtime_error("Failed to select Vulkan physical device: " + physicalDeviceResult.error().message());
    }

    vkb::DeviceBuilder deviceBuilder{ physicalDeviceResult.value() };
    auto deviceResult = deviceBuilder.build();
    if (!deviceResult) {
        throw std::runtime_error("Failed to create Vulkan logical device: " + deviceResult.error().message());
    }
    device = deviceResult.value();

    vkb::SwapchainBuilder swapchainBuilder{ device };
    auto swapchainResult = swapchainBuilder.build();
    if (!swapchainResult) {
        throw std::runtime_error("Failed to create Vulkan swapchain: " + swapchainResult.error().message());
    }
    swapchain = swapchainResult.value();

    auto imageViewsResult = swapchain.get_image_views();
    if (!imageViewsResult) {
        throw std::runtime_error("Failed to get swapchain image views");
    }
    swapchainImageViews = imageViewsResult.value();

    // --- render pass ---
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchain.image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(device.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }

    // --- graphics pipeline (colored quads: board, pieces, grid lines, UI borders) ---
    VkShaderModule vertShaderModule = createShaderModule(device.device, readFile("shaders/tetris.vert.spv"));
    VkShaderModule fragShaderModule = createShaderModule(device.device, readFile("shaders/tetris.frag.spv"));

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2]{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device.device, fragShaderModule, nullptr);

    // --- framebuffers ---
    framebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapchainImageViews[i];
        framebufferInfo.width = swapchain.extent.width;
        framebufferInfo.height = swapchain.extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device.device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }

    // --- command pool/buffer ---
    auto graphicsQueueResult = device.get_queue(vkb::QueueType::graphics);
    if (!graphicsQueueResult) {
        throw std::runtime_error("Failed to get graphics queue");
    }
    graphicsQueue = graphicsQueueResult.value();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value();

    if (vkCreateCommandPool(device.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device.device, &cmdAllocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }

    // --- sync objects ---
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device.device, &fenceInfo, nullptr, &inFlightFence);

    // --- vertex buffer: host-visible, persistently mapped, overwritten every frame ---
    VkDeviceSize bufferSize = kMaxVertices * sizeof(Vertex);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device.device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.device, vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memRequirements.size;
    memAllocInfo.memoryTypeIndex = findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device.device, &memAllocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory");
    }

    vkBindBufferMemory(device.device, vertexBuffer, vertexBufferMemory, 0);
    vkMapMemory(device.device, vertexBufferMemory, 0, bufferSize, 0, &mappedVertexData);

    // ============================================================
    // Everything below this point is new: a small texture-based font
    // renderer (font atlas texture, descriptor set, a second pipeline,
    // a second vertex buffer) used for real UI text (SCORE/NEXT/HOLD
    // labels and the score number) instead of the earlier 7-segment
    // digit shortcut.
    // ============================================================

    // --- font atlas texture ---
    {
        constexpr int cellW = Font::kGlyphWidth + 1; // +1px padding to avoid sampling bleed between glyphs
        constexpr int cellH = Font::kGlyphHeight + 1;
        fontAtlasWidth = cellW * Font::kGlyphCount;
        fontAtlasHeight = cellH;

        std::vector<uint8_t> atlasPixels(static_cast<size_t>(fontAtlasWidth) * fontAtlasHeight, 0);
        for (int idx = 0; idx < Font::kGlyphCount; ++idx) {
            const Font::Glyph& glyph = Font::getGlyph(Font::kGlyphOrder[idx]);
            for (int row = 0; row < Font::kGlyphHeight; ++row) {
                uint8_t rowBits = glyph.rows[row];
                for (int col = 0; col < Font::kGlyphWidth; ++col) {
                    bool on = (rowBits >> (Font::kGlyphWidth - 1 - col)) & 1u;
                    if (on) {
                        int px = idx * cellW + col;
                        atlasPixels[static_cast<size_t>(row) * fontAtlasWidth + px] = 255;
                    }
                }
            }
        }

        VkDeviceSize imageSize = atlasPixels.size();

        // Staging buffer: CPU-writable, temporary - only exists to get the
        // pixel data into GPU-only memory, then gets destroyed.
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = imageSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device.device, &stagingBufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create font atlas staging buffer");
        }

        VkMemoryRequirements stagingMemReq;
        vkGetBufferMemoryRequirements(device.device, stagingBuffer, &stagingMemReq);

        VkMemoryAllocateInfo stagingAllocInfo{};
        stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        stagingAllocInfo.allocationSize = stagingMemReq.size;
        stagingAllocInfo.memoryTypeIndex = findMemoryType(
            stagingMemReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device.device, &stagingAllocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate font atlas staging buffer memory");
        }
        vkBindBufferMemory(device.device, stagingBuffer, stagingBufferMemory, 0);

        void* stagingData;
        vkMapMemory(device.device, stagingBufferMemory, 0, imageSize, 0, &stagingData);
        std::memcpy(stagingData, atlasPixels.data(), static_cast<size_t>(imageSize));
        vkUnmapMemory(device.device, stagingBufferMemory);

        // The actual texture image - device-local (fast for the GPU to
        // sample), not directly writable from the CPU, which is exactly
        // why the staging buffer above exists.
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(fontAtlasWidth);
        imageInfo.extent.height = static_cast<uint32_t>(fontAtlasHeight);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8_UNORM; // single channel: 0=off, 255=on
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(device.device, &imageInfo, nullptr, &fontAtlasImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create font atlas image");
        }

        VkMemoryRequirements imageMemReq;
        vkGetImageMemoryRequirements(device.device, fontAtlasImage, &imageMemReq);

        VkMemoryAllocateInfo imageAllocInfo{};
        imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        imageAllocInfo.allocationSize = imageMemReq.size;
        imageAllocInfo.memoryTypeIndex = findMemoryType(imageMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device.device, &imageAllocInfo, nullptr, &fontAtlasImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate font atlas image memory");
        }
        vkBindImageMemory(device.device, fontAtlasImage, fontAtlasImageMemory, 0);

        // Copying into the image requires it to be in TRANSFER_DST layout
        // first, then SHADER_READ_ONLY once we're done writing - these
        // layout changes aren't automatic, we declare them explicitly via
        // pipeline barriers.
        VkCommandBuffer cmd = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = fontAtlasImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                              0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { static_cast<uint32_t>(fontAtlasWidth), static_cast<uint32_t>(fontAtlasHeight), 1 };

        vkCmdCopyBufferToImage(cmd, stagingBuffer, fontAtlasImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                              0, 0, nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(cmd);

        vkDestroyBuffer(device.device, stagingBuffer, nullptr);
        vkFreeMemory(device.device, stagingBufferMemory, nullptr);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = fontAtlasImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.device, &viewInfo, nullptr, &fontAtlasImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create font atlas image view");
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST; // crisp pixel-font look, no blurring
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        if (vkCreateSampler(device.device, &samplerInfo, nullptr, &fontAtlasSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create font atlas sampler");
        }
    }

    // --- descriptor set: connects the font atlas texture+sampler to the text shader ---
    {
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &samplerBinding;

        if (vkCreateDescriptorSetLayout(device.device, &layoutInfo, nullptr, &textDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create text descriptor set layout");
        }

        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolCreateInfo{};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.poolSizeCount = 1;
        poolCreateInfo.pPoolSizes = &poolSize;
        poolCreateInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device.device, &poolCreateInfo, nullptr, &textDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create text descriptor pool");
        }

        VkDescriptorSetAllocateInfo dsAllocInfo{};
        dsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAllocInfo.descriptorPool = textDescriptorPool;
        dsAllocInfo.descriptorSetCount = 1;
        dsAllocInfo.pSetLayouts = &textDescriptorSetLayout;

        if (vkAllocateDescriptorSets(device.device, &dsAllocInfo, &textDescriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate text descriptor set");
        }

        VkDescriptorImageInfo imageInfoDesc{};
        imageInfoDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfoDesc.imageView = fontAtlasImageView;
        imageInfoDesc.sampler = fontAtlasSampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = textDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfoDesc;

        vkUpdateDescriptorSets(device.device, 1, &descriptorWrite, 0, nullptr);
    }

    // --- text pipeline: a second, separate pipeline that samples the font atlas ---
    {
        VkShaderModule textVertModule = createShaderModule(device.device, readFile("shaders/text.vert.spv"));
        VkShaderModule textFragModule = createShaderModule(device.device, readFile("shaders/text.frag.spv"));

        VkPipelineShaderStageCreateInfo textVertStage{};
        textVertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        textVertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        textVertStage.module = textVertModule;
        textVertStage.pName = "main";

        VkPipelineShaderStageCreateInfo textFragStage{};
        textFragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        textFragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        textFragStage.module = textFragModule;
        textFragStage.pName = "main";

        VkPipelineShaderStageCreateInfo textStages[] = { textVertStage, textFragStage };

        VkVertexInputBindingDescription textBinding{};
        textBinding.binding = 0;
        textBinding.stride = sizeof(TextVertex);
        textBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription textAttrs[2]{};
        textAttrs[0].binding = 0;
        textAttrs[0].location = 0;
        textAttrs[0].format = VK_FORMAT_R32G32_SFLOAT;
        textAttrs[0].offset = offsetof(TextVertex, pos);

        textAttrs[1].binding = 0;
        textAttrs[1].location = 1;
        textAttrs[1].format = VK_FORMAT_R32G32_SFLOAT;
        textAttrs[1].offset = offsetof(TextVertex, uv);

        VkPipelineVertexInputStateCreateInfo textVertexInput{};
        textVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        textVertexInput.vertexBindingDescriptionCount = 1;
        textVertexInput.pVertexBindingDescriptions = &textBinding;
        textVertexInput.vertexAttributeDescriptionCount = 2;
        textVertexInput.pVertexAttributeDescriptions = textAttrs;

        VkPipelineInputAssemblyStateCreateInfo textInputAssembly{};
        textInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        textInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo textViewportState{};
        textViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        textViewportState.viewportCount = 1;
        textViewportState.scissorCount = 1;

        VkDynamicState textDynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo textDynamicState{};
        textDynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        textDynamicState.dynamicStateCount = 2;
        textDynamicState.pDynamicStates = textDynamicStates;

        VkPipelineRasterizationStateCreateInfo textRasterizer{};
        textRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        textRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        textRasterizer.lineWidth = 1.0f;
        textRasterizer.cullMode = VK_CULL_MODE_NONE;
        textRasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo textMultisampling{};
        textMultisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        textMultisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState textColorBlendAttachment{};
        textColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        textColorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo textColorBlending{};
        textColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        textColorBlending.attachmentCount = 1;
        textColorBlending.pAttachments = &textColorBlendAttachment;

        VkPipelineLayoutCreateInfo textPipelineLayoutInfo{};
        textPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        textPipelineLayoutInfo.setLayoutCount = 1;
        textPipelineLayoutInfo.pSetLayouts = &textDescriptorSetLayout;

        if (vkCreatePipelineLayout(device.device, &textPipelineLayoutInfo, nullptr, &textPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create text pipeline layout");
        }

        VkGraphicsPipelineCreateInfo textPipelineInfo{};
        textPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        textPipelineInfo.stageCount = 2;
        textPipelineInfo.pStages = textStages;
        textPipelineInfo.pVertexInputState = &textVertexInput;
        textPipelineInfo.pInputAssemblyState = &textInputAssembly;
        textPipelineInfo.pViewportState = &textViewportState;
        textPipelineInfo.pRasterizationState = &textRasterizer;
        textPipelineInfo.pMultisampleState = &textMultisampling;
        textPipelineInfo.pColorBlendState = &textColorBlending;
        textPipelineInfo.pDynamicState = &textDynamicState;
        textPipelineInfo.layout = textPipelineLayout;
        textPipelineInfo.renderPass = renderPass;
        textPipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device.device, VK_NULL_HANDLE, 1, &textPipelineInfo, nullptr, &textPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create text pipeline");
        }

        vkDestroyShaderModule(device.device, textVertModule, nullptr);
        vkDestroyShaderModule(device.device, textFragModule, nullptr);
    }

    // --- text vertex buffer: host-visible, persistently mapped, overwritten every frame ---
    {
        VkDeviceSize textBufferSize = kMaxTextVertices * sizeof(TextVertex);

        VkBufferCreateInfo textBufferInfo{};
        textBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        textBufferInfo.size = textBufferSize;
        textBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        textBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device.device, &textBufferInfo, nullptr, &textVertexBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create text vertex buffer");
        }

        VkMemoryRequirements textMemReq;
        vkGetBufferMemoryRequirements(device.device, textVertexBuffer, &textMemReq);

        VkMemoryAllocateInfo textMemAllocInfo{};
        textMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        textMemAllocInfo.allocationSize = textMemReq.size;
        textMemAllocInfo.memoryTypeIndex = findMemoryType(
            textMemReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device.device, &textMemAllocInfo, nullptr, &textVertexBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate text vertex buffer memory");
        }

        vkBindBufferMemory(device.device, textVertexBuffer, textVertexBufferMemory, 0);
        vkMapMemory(device.device, textVertexBufferMemory, 0, textBufferSize, 0, &mappedTextVertexData);
    }

    std::printf("Renderer initialized successfully.\n");
    std::fflush(stdout);
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(device.device);

    vkUnmapMemory(device.device, textVertexBufferMemory);
    vkDestroyBuffer(device.device, textVertexBuffer, nullptr);
    vkFreeMemory(device.device, textVertexBufferMemory, nullptr);

    vkDestroyPipeline(device.device, textPipeline, nullptr);
    vkDestroyPipelineLayout(device.device, textPipelineLayout, nullptr);

    vkDestroyDescriptorPool(device.device, textDescriptorPool, nullptr); // also frees textDescriptorSet
    vkDestroyDescriptorSetLayout(device.device, textDescriptorSetLayout, nullptr);

    vkDestroySampler(device.device, fontAtlasSampler, nullptr);
    vkDestroyImageView(device.device, fontAtlasImageView, nullptr);
    vkDestroyImage(device.device, fontAtlasImage, nullptr);
    vkFreeMemory(device.device, fontAtlasImageMemory, nullptr);

    vkUnmapMemory(device.device, vertexBufferMemory);
    vkDestroyBuffer(device.device, vertexBuffer, nullptr);
    vkFreeMemory(device.device, vertexBufferMemory, nullptr);

    vkDestroyFence(device.device, inFlightFence, nullptr);
    vkDestroySemaphore(device.device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device.device, imageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(device.device, commandPool, nullptr);

    for (VkFramebuffer framebuffer : framebuffers) {
        vkDestroyFramebuffer(device.device, framebuffer, nullptr);
    }

    vkDestroyPipeline(device.device, pipeline, nullptr);
    vkDestroyPipelineLayout(device.device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device.device, renderPass, nullptr);

    swapchain.destroy_image_views(swapchainImageViews);
    vkb::destroy_swapchain(swapchain);
    vkb::destroy_device(device);
    vkb::destroy_surface(instance, surface);
    vkb::destroy_instance(instance);
}

std::vector<Renderer::Vertex> Renderer::buildVertices(const Engine& engine) const {
    std::vector<Vertex> vertices;

    // Board occupies NDC x in [-1, boardRightNDC] (left portion of the
    // window, in pixels Settings::kBoardPixelWidth wide); everything right
    // of boardRightNDC is the UI panel.
    const float boardFractionX = static_cast<float>(Settings::kBoardPixelWidth) / Settings::kWindowWidth;
    const float boardRightNDC = -1.0f + 2.0f * boardFractionX;

    const float cellWidth = (boardRightNDC - (-1.0f)) / Settings::kBoardWidth;
    const float cellHeight = 2.0f / Settings::kBoardHeight;

    auto addQuad = [&](float x0, float y0, float x1, float y1, float r, float g, float b) {
        vertices.push_back({ {x0, y0}, {r, g, b} });
        vertices.push_back({ {x1, y0}, {r, g, b} });
        vertices.push_back({ {x1, y1}, {r, g, b} });

        vertices.push_back({ {x0, y0}, {r, g, b} });
        vertices.push_back({ {x1, y1}, {r, g, b} });
        vertices.push_back({ {x0, y1}, {r, g, b} });
    };

    auto addCell = [&](int row, int col, Settings::RGB rgb) {
        float x0 = -1.0f + col * cellWidth;
        float x1 = x0 + cellWidth;
        float y0 = -1.0f + row * cellHeight;
        float y1 = y0 + cellHeight;
        addQuad(x0, y0, x1, y1, rgb.r / 255.0f, rgb.g / 255.0f, rgb.b / 255.0f);
    };

    const float lineHalfThickness = 0.004f;
    const float gr = 45.0f / 255.0f;
    const float gg = 45.0f / 255.0f;
    const float gb = 45.0f / 255.0f;

    // Draws a thin rectangular outline (4 edges) in color (r,g,b) - used for
    // the board's grid lines, UI panel box borders (always gray, gr/gg/gb),
    // the preview-piece grid lines (gray), and the ghost piece outline
    // (the falling piece's own color).
    auto addBorderBox = [&](float x0, float y0, float x1, float y1, float r, float g, float b) {
        addQuad(x0, y0, x1, y0 + lineHalfThickness * 2.0f, r, g, b);              // top
        addQuad(x0, y1 - lineHalfThickness * 2.0f, x1, y1, r, g, b);              // bottom
        addQuad(x0, y0, x0 + lineHalfThickness * 2.0f, y1, r, g, b);              // left
        addQuad(x1 - lineHalfThickness * 2.0f, y0, x1, y1, r, g, b);              // right
    };

    // Draws `block`'s current shape centered inside box [x0,y0]-[x1,y1],
    // using the board's own cellWidth/cellHeight (not a box-derived size) -
    // so next/hold previews render at the same visual scale as the board
    // itself, just centered in their own box. Used for next-piece and
    // hold-piece previews.
    auto addPiecePreview = [&](const Block& block, float x0, float y0, float x1, float y1) {
        auto [minRow, maxRow, minCol, maxCol] = block.getShapeBounds();
        int pieceWidthCells = maxCol - minCol + 1;
        int pieceHeightCells = maxRow - minRow + 1;

        float boxWidth = x1 - x0;
        float boxHeight = y1 - y0;
        float pieceWidthNDC = pieceWidthCells * cellWidth;
        float pieceHeightNDC = pieceHeightCells * cellHeight;

        float startX = x0 + (boxWidth - pieceWidthNDC) / 2.0f;
        float startY = y0 + (boxHeight - pieceHeightNDC) / 2.0f;

        const Shape& previewShape = block.getCurrentShape();
        Settings::RGB previewColor = Settings::getColorRGB(Blocks::getColor(block.type));
        float pr = previewColor.r / 255.0f;
        float pg = previewColor.g / 255.0f;
        float pb = previewColor.b / 255.0f;

        for (int i = minRow; i <= maxRow; ++i) {
            for (int j = minCol; j <= maxCol; ++j) {
                if (previewShape[i][j] != 0) {
                    float cellX0 = startX + (j - minCol) * cellWidth;
                    float cellY0 = startY + (i - minRow) * cellHeight;
                    addQuad(cellX0, cellY0, cellX0 + cellWidth, cellY0 + cellHeight, pr, pg, pb);
                    addBorderBox(cellX0, cellY0, cellX0 + cellWidth, cellY0 + cellHeight, gr, gg, gb);
                }
            }
        }
    };

    const Grid& grid = engine.getGrid();
    for (int row = 0; row < Settings::kBoardHeight; ++row) {
        for (int col = 0; col < Settings::kBoardWidth; ++col) {
            uint8_t cell = grid.getCell(row, col);
            if (cell != 0) {
                BlockType type = static_cast<BlockType>(cell - 1);
                addCell(row, col, Settings::getColorRGB(Blocks::getColor(type)));
            }
        }
    }

    const Block& activeBlock = engine.getCurrentBlock();
    const Shape& shape = activeBlock.getCurrentShape();
    Settings::RGB activeColor = Settings::getColorRGB(Blocks::getColor(activeBlock.type));
    float activeR = activeColor.r / 255.0f;
    float activeG = activeColor.g / 255.0f;
    float activeB = activeColor.b / 255.0f;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (shape[i][j] != 0) {
                int globalRow = activeBlock.row + i;
                int globalCol = activeBlock.col + j;
                if (grid.isInBounds(globalRow, globalCol)) {
                    addCell(globalRow, globalCol, activeColor);
                }
            }
        }
    }

    // Board grid outline (internal lines, not just the outer border)
    for (int col = 0; col <= Settings::kBoardWidth; ++col) {
        float x = -1.0f + col * cellWidth;
        addQuad(x - lineHalfThickness, -1.0f, x + lineHalfThickness, 1.0f, gr, gg, gb);
    }
    for (int row = 0; row <= Settings::kBoardHeight; ++row) {
        float y = -1.0f + row * cellHeight;
        addQuad(-1.0f, y - lineHalfThickness, boardRightNDC, y + lineHalfThickness, gr, gg, gb);
    }

    // Ghost piece: outline only (no fill, since the pipeline has no alpha
    // blending set up) at the row the piece would land on hard drop.
    // Drawn AFTER the grid lines (above) so it paints on top of them -
    // there's no depth test, so draw order is paint order, and this was
    // previously drawn before the grid lines, making the lines cut across
    // the outline instead of sitting behind it.
    int ghostRow = engine.getGhostRow();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (shape[i][j] != 0) {
                int globalRow = ghostRow + i;
                int globalCol = activeBlock.col + j;
                if (grid.isInBounds(globalRow, globalCol)) {
                    float x0 = -1.0f + globalCol * cellWidth;
                    float y0 = -1.0f + globalRow * cellHeight;
                    addBorderBox(x0, y0, x0 + cellWidth, y0 + cellHeight, activeR, activeG, activeB);
                }
            }
        }
    }

    // --- UI panel boxes. Each region is a label band (text drawn
    // separately in buildTextVertices) above a bordered content box. ---
    const float panelX0 = boardRightNDC + 0.05f;
    const float panelX1 = 1.0f - 0.05f;

    // Score box
    addBorderBox(panelX0, -0.85f, panelX1, -0.55f, gr, gg, gb);

    // Next-piece box
    addBorderBox(panelX0, -0.40f, panelX1, -0.05f, gr, gg, gb);
    addPiecePreview(engine.getNextBlock(), panelX0, -0.40f, panelX1, -0.05f);

    // Hold-piece box
    addBorderBox(panelX0, 0.15f, panelX1, 0.50f, gr, gg, gb);
    const std::optional<Block>& holdBlock = engine.getHoldBlock();
    if (holdBlock.has_value()) {
        addPiecePreview(*holdBlock, panelX0, 0.15f, panelX1, 0.50f);
    }

    return vertices;
}

void Renderer::addText(std::vector<TextVertex>& verts, const std::string& text, float x, float y, float pixelScale) const {
    // Each font pixel becomes `pixelScale` screen pixels; converting
    // separately via window width vs height (rather than one shared NDC
    // delta) is what keeps glyphs square on screen regardless of the
    // window's aspect ratio - the exact bug class that hit both the piece
    // previews and the old digit display earlier.
    float glyphWidthNDC = (Font::kGlyphWidth * pixelScale * 2.0f) / Settings::kWindowWidth;
    float glyphHeightNDC = (Font::kGlyphHeight * pixelScale * 2.0f) / Settings::kWindowHeight;
    float spacingNDC = (pixelScale * 2.0f) / Settings::kWindowWidth;

    constexpr int cellW = Font::kGlyphWidth + 1;

    float cursorX = x;
    for (char c : text) {
        if (c != ' ') {
            int idx = Font::getGlyphIndex(c);
            float u0 = static_cast<float>(idx * cellW) / fontAtlasWidth;
            float u1 = static_cast<float>(idx * cellW + Font::kGlyphWidth) / fontAtlasWidth;
            float v0 = 0.0f;
            float v1 = static_cast<float>(Font::kGlyphHeight) / fontAtlasHeight;

            verts.push_back({ {cursorX, y}, {u0, v0} });
            verts.push_back({ {cursorX + glyphWidthNDC, y}, {u1, v0} });
            verts.push_back({ {cursorX + glyphWidthNDC, y + glyphHeightNDC}, {u1, v1} });

            verts.push_back({ {cursorX, y}, {u0, v0} });
            verts.push_back({ {cursorX + glyphWidthNDC, y + glyphHeightNDC}, {u1, v1} });
            verts.push_back({ {cursorX, y + glyphHeightNDC}, {u0, v1} });
        }
        cursorX += glyphWidthNDC + spacingNDC;
    }
}

std::vector<Renderer::TextVertex> Renderer::buildTextVertices(const Engine& engine) const {
    std::vector<TextVertex> verts;

    const float boardFractionX = static_cast<float>(Settings::kBoardPixelWidth) / Settings::kWindowWidth;
    const float boardRightNDC = -1.0f + 2.0f * boardFractionX;
    const float panelX0 = boardRightNDC + 0.05f;
    const float panelX1 = 1.0f - 0.05f;

    auto centeredX = [&](const std::string& text, float pixelScale) {
        float glyphWidthNDC = (Font::kGlyphWidth * pixelScale * 2.0f) / Settings::kWindowWidth;
        float spacingNDC = (pixelScale * 2.0f) / Settings::kWindowWidth;
        float textWidth = text.empty() ? 0.0f : text.length() * (glyphWidthNDC + spacingNDC) - spacingNDC;
        return panelX0 + ((panelX1 - panelX0) - textWidth) / 2.0f;
    };

    constexpr float kLabelScale = 2.0f;
    addText(verts, "SCORE", centeredX("SCORE", kLabelScale), -0.95f, kLabelScale);
    addText(verts, "NEXT", centeredX("NEXT", kLabelScale), -0.50f, kLabelScale);
    addText(verts, "HOLD", centeredX("HOLD", kLabelScale), 0.05f, kLabelScale);

    char scoreBuf[16];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%06d", engine.getScore());
    std::string scoreText(scoreBuf);
    constexpr float kScoreScale = 4.0f;
    addText(verts, scoreText, centeredX(scoreText, kScoreScale), -0.73f, kScoreScale);

    return verts;
}

void Renderer::draw(const Engine& engine) {
    vkWaitForFences(device.device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device.device, 1, &inFlightFence);

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(
        device.device, swapchain.swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        std::printf("Warning: vkAcquireNextImageKHR returned %d (window resize isn't handled yet)\n",
                    static_cast<int>(acquireResult));
        return;
    }

    std::vector<Vertex> vertices = buildVertices(engine);
    std::memcpy(mappedVertexData, vertices.data(), vertices.size() * sizeof(Vertex));

    std::vector<TextVertex> textVertices = buildTextVertices(engine);
    std::memcpy(mappedTextVertexData, textVertices.data(), textVertices.size() * sizeof(TextVertex));

    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkClearValue clearColor{};
    clearColor.color.float32[0] = 0.0f;
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 0.0f;
    clearColor.color.float32[3] = 1.0f;

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = swapchain.extent;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.extent.width);
    viewport.height = static_cast<float>(swapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain.extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Pass 1: colored quads (board, pieces, grid lines, UI borders)
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

    // Pass 2: text (SCORE/NEXT/HOLD labels + score number), drawn on top,
    // same render pass
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textPipelineLayout, 0, 1, &textDescriptorSet, 0, nullptr);

    VkBuffer textVertexBuffers[] = { textVertexBuffer };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, textVertexBuffers, offsets);
    vkCmdDraw(commandBuffer, static_cast<uint32_t>(textVertices.size()), 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapchains[] = { swapchain.swapchain };
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR) {
        std::printf("Warning: vkQueuePresentKHR returned %d (window resize isn't handled yet)\n",
                    static_cast<int>(presentResult));
    }
}
