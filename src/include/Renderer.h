#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <VkBootstrap.h>

#include "Engine.h"

struct GLFWwindow;

class Renderer {
public:
    explicit Renderer(GLFWwindow* window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void draw(const Engine& engine);

private:
    struct Vertex {
        float pos[2];
        float color[3];
    };

    struct TextVertex {
        float pos[2];
        float uv[2];
    };

    std::vector<Vertex> buildVertices(const Engine& engine) const;
    std::vector<TextVertex> buildTextVertices(const Engine& engine) const;
    void addText(std::vector<TextVertex>& verts, const std::string& text, float x, float y, float pixelScale) const;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    vkb::Instance instance;
    VkSurfaceKHR surface;
    vkb::Device device;
    vkb::Swapchain swapchain;

    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkQueue graphicsQueue;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    void* mappedVertexData;

    // --- text rendering ---
    VkImage fontAtlasImage;
    VkDeviceMemory fontAtlasImageMemory;
    VkImageView fontAtlasImageView;
    VkSampler fontAtlasSampler;
    int fontAtlasWidth;
    int fontAtlasHeight;

    VkDescriptorSetLayout textDescriptorSetLayout;
    VkDescriptorPool textDescriptorPool;
    VkDescriptorSet textDescriptorSet;

    VkPipelineLayout textPipelineLayout;
    VkPipeline textPipeline;

    VkBuffer textVertexBuffer;
    VkDeviceMemory textVertexBufferMemory;
    void* mappedTextVertexData;
};
