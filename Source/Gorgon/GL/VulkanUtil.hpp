#pragma once

#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>

#include <optional>
#include <fstream>
#include <array>
#include <memory>
#include <set>
#include <unordered_map>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

// VULKAN_UTIL_CORE_ONLY keeps only the VulkanF and SingleTimeCommands parts. There is no window
// library, model or image loader in this mode. Engines that have their own window and asset
// systems (like Gorgon) can use it to drive Vulkan without the 3D engine at the end of this file.
#ifndef VULKAN_UTIL_CORE_ONLY
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#endif

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>



#ifdef VULKAN_UTIL_IMPLEMENTATION
    #define VMA_IMPLEMENTATION
    #ifndef VULKAN_UTIL_CORE_ONLY
        #define STB_IMAGE_IMPLEMENTATION
        #define TINYOBJLOADER_IMPLEMENTATION
        #define CGLTF_IMPLEMENTATION
    #endif
#endif

#include <vk_mem_alloc.h>
#ifndef VULKAN_UTIL_CORE_ONLY
#include <stb/stb_image.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <tiny_obj_loader.h>
#include <cgltf/cgltf.h>
#endif

namespace VulkanAbstraction {

#ifndef VULKAN_UTIL_CORE_ONLY
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        
        static VkVertexInputBindingDescription GetBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }
        
        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
            return attributeDescriptions;
        }
    };

    void LoadModelOBJ(const std::string& MODEL_PATH, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void LoadModelGLB(const char* filePath, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices, stbi_uc** outPixels, uint32_t* outWidth, uint32_t* outHeight);
#endif

    std::vector<char> readFile(const std::string& filename);

    namespace SingleTimeCommands {
        VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
        void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);
        void copyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        // These only record, so multiple operations can share one submit and frames can use them
        void recordTransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
        void recordCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkOffset2D offset, VkExtent2D extent);
    }

    namespace VulkanF {
        static const std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        static const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        static const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        struct FrameSyncObjects {
            VkSemaphore imageAvailableSemaphore;
            VkSemaphore renderFinishedSemaphore;
            VkFence inFlightFence;
        };

        struct BaseMemoryAllocatable { VmaAllocation deviceMemory; };
        struct Image : public BaseMemoryAllocatable { VkImage image; };
        struct Buffer : public BaseMemoryAllocatable { VkBuffer buffer; };

        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentationFamily;
            bool isComplete() { return graphicsFamily.has_value() && presentationFamily.has_value(); }
        };

        struct VulkanDevice {
            VkPhysicalDevice physicalDevice;
            VkDevice logicalDevice;
            QueueFamilyIndices familyIndices;
            VkQueue graphicsQueue;
            VkQueue presentQueue;
        };

        struct VulkanPresenter {
            VkSurfaceKHR surface;
            VkSwapchainKHR swapChain;
            struct SwapchainProperties {
                VkSurfaceFormatKHR surfaceFormat;
                VkSurfaceCapabilitiesKHR surfaceCapabilities;
                VkPresentModeKHR presentMode;
                VkExtent2D imageExtent;
            } swapchainProperties;
            std::vector<VkImage> swapChainImages;
            std::vector<VkImageView> swapChainImageViews;
        };

        VkDescriptorSetLayoutBinding CreateDescriptorSetBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags shaderStage);
        VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanDevice device, const std::vector<VkDescriptorSetLayoutBinding>& setBindings);

        struct VulkanTexture {
            Image textureImage;
            VkImageView textureImageView;
            VkSampler textureSampler;
        };

        struct VulkanRenderPass {
            VkRenderPass renderPass;
            std::vector<VkFramebuffer> frameBuffers;
            Image depthImage;
            VkImageView depthImageView;
            Image msaaColorImage;
            VkImageView msaaColorImageView;
        };

        // A color image that can be rendered to and then sampled like a regular texture. It is in
        // SHADER_READ_ONLY_OPTIMAL layout whenever it is not inside a render pass.
        struct VulkanOffscreenTarget {
            Image image;
            VkImageView imageView;
            VkFramebuffer frameBuffer;
            VkExtent2D extent;
        };

        struct TextureInfo {
            unsigned char* pixels;
            size_t size;
            int width, height, channels;
        };

        struct VulkanShader {
            VkPipelineShaderStageCreateInfo shaderStage;
            VkShaderModule __shaderModule;
        };

        struct VulkanDescriptorSetLayout {
            VkDescriptorSetLayout layout;
            std::vector<VkDescriptorSetLayoutBinding> bindingList;
            void AddBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags shaderStage) {
                bindingList.push_back(CreateDescriptorSetBinding(binding, descriptorType, descriptorCount, shaderStage));
            }
            void Build(VulkanDevice device) { layout = CreateDescriptorSetLayout(device, bindingList); }
        };
        struct VulkanPushConstant{
            VkPushConstantRange pushConstantRange;
            uint32_t offset,size;
            VkShaderStageFlags flags;
        };
        struct VulkanGraphicsPipeline {
            VkPipeline pipeline;
            std::vector<VkVertexInputBindingDescription> bindingDescriptions;
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
            std::vector<VulkanShader> shaders;
            std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
            std::vector<VkDescriptorSetLayout> setLayouts;
            std::vector<VulkanDescriptorSetLayout> _setLayouts;
            std::optional<VulkanPushConstant> pushConstant;
            VkPipelineLayout sharedLayout{VK_NULL_HANDLE};

            struct {
                VkRect2D scissor;
                VkViewport viewport;
                VkPipelineColorBlendAttachmentState colorBlendAttachement;
                uint32_t                                   stageCount;
                VkPipelineShaderStageCreateInfo*           pStages;
                VkPipelineVertexInputStateCreateInfo       VertexInputState;
                VkPipelineInputAssemblyStateCreateInfo     InputAssemblyState;
                VkPipelineTessellationStateCreateInfo      TessellationState;
                VkPipelineViewportStateCreateInfo          ViewportState;
                VkPipelineRasterizationStateCreateInfo     RasterizationState;
                VkPipelineMultisampleStateCreateInfo       MultisampleState;
                VkPipelineDepthStencilStateCreateInfo      DepthStencilState;
                VkPipelineColorBlendStateCreateInfo        ColorBlendState;
                VkPipelineDynamicStateCreateInfo           DynamicState;
                VkPipelineLayout                           layout;
                VkRenderPass                               renderPass;
                uint32_t                                   subpass;
            } __States;

            void InitDefaultPipeline(VulkanPresenter presenter, VulkanRenderPass vulkanRenderPass, uint32_t subpassid = 0);
            void BuildPipeline(VulkanDevice device,VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_4_BIT);
            void AddVertexInputState(const std::vector<VkVertexInputBindingDescription>& bindingDescriptions, const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions) {
                this->attributeDescriptions = attributeDescriptions;
                this->bindingDescriptions = bindingDescriptions;
            }
            void AddShaderStage(VulkanDevice device, const std::string& filename, VkShaderStageFlagBits shaderStage);
            void AddShaderStageFromSPIRV(VulkanDevice device, const std::vector<char>& code, VkShaderStageFlagBits shaderStage);
            void AddDescriptorSetLayouts(const std::vector<VulkanDescriptorSetLayout>& setLayouts) { this->_setLayouts = setLayouts; }
            void AddPushConstants(VkShaderStageFlags shaderStage,uint32_t size,uint32_t offset);
            // Pipelines that share a layout can be switched without rebinding descriptor sets.
            // BuildPipeline will not create a layout and the given layout stays owned by the caller.
            void UseSharedPipelineLayout(VkPipelineLayout layout) { sharedLayout = layout; }
        };

        namespace Memory {
            Image CreateImage(VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, VmaAllocationCreateFlags flags,VkSampleCountFlagBits sampleFlags=VK_SAMPLE_COUNT_1_BIT);
            Buffer CreateBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memUsage, VmaAllocationCreateFlags flags);
            void MapMemory(VmaAllocator allocator, BaseMemoryAllocatable* allocatableType, void** Data);
            void UnMapMemory(VmaAllocator allocator, BaseMemoryAllocatable* allocatableType);
        }

        namespace SwapchainHelpers {
            bool SupportsDeviceExtensions(VkPhysicalDevice device,const std::vector<const char*>& required); 
            bool IsSwapchainAdequate(VkPhysicalDevice device, VkSurfaceKHR surface);
            VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
            VkPresentModeKHR ChoosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
            VkExtent2D GetImageExtent(void* window, VkSurfaceCapabilitiesKHR capabilities);
            VkExtent2D GetImageExtent(VkExtent2D windowExtent, VkSurfaceCapabilitiesKHR capabilities);
            VkSurfaceFormatKHR ChooseSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<VkFormat>& preferredFormats = { VK_FORMAT_B8G8R8A8_SRGB });
            std::vector<VkImage> GetSwapchainImages(VkDevice logicalDevice, VkSwapchainKHR swapChain);
            VkImageView CreateImageView(VkDevice logicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkComponentMapping components = {});
            std::vector<VkImageView> CreateSwapChainImageViews(VkDevice logicalDevice, const std::vector<VkImage>& swapChainImages, VkSurfaceFormatKHR surfaceFormat);
        }

        VkShaderModule CreateShaderModule(VulkanDevice device, const std::vector<char>& code);
        VkPipelineShaderStageCreateInfo CreateShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage);
        VulkanShader SetupShader(VulkanDevice device, const std::string& filename, VkShaderStageFlagBits shaderStage);

        namespace DepthHelpers {
            VkFormat findSupportedFormat(VkPhysicalDevice phy, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
            VkFormat findDepthFormat(VkPhysicalDevice phy);
            bool hasStencilComponent(VkFormat format);
        }

#ifndef VULKAN_UTIL_CORE_ONLY
        VkInstance CreateInstance(uint32_t api_version = VK_API_VERSION_1_0, const std::string& appName = "app", const std::string& engineName = "No Engine");
#endif
        VkInstance CreateInstance(const std::vector<const char*>& extensions, const std::vector<const char*>& layers, uint32_t api_version = VK_API_VERSION_1_0, const std::string& appName = "app", const std::string& engineName = "No Engine");
        VkPhysicalDevice PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
        QueueFamilyIndices PickQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        VkDevice CreateLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices queuefamilyindices, VkQueue* graphicsQueue, VkQueue* presentQueue);
        VkSwapchainKHR CreateSwapchain(VkDevice logicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR capabilities, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkExtent2D imageExtent, QueueFamilyIndices familyIndices, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
        VulkanDevice SetupDevice(VkInstance instance, VkSurfaceKHR surface);
        VmaAllocator CreateAllocator(VkInstance instance, VulkanDevice& vulkanDevice);
#ifndef VULKAN_UTIL_CORE_ONLY
        VulkanPresenter SetupPresenter(VulkanDevice& device, void* window, VkSurfaceKHR surface);
#endif
        VulkanPresenter SetupPresenter(VulkanDevice& device, VkExtent2D windowExtent, VkSurfaceKHR surface, const std::vector<VkFormat>& preferredFormats, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
        VkRenderPass CreateRenderpass(VkPhysicalDevice phy, VkDevice log, VkFormat format,VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT);
        // Color only render pass that keeps the contents of its attachment, which stays in the given layout
        // before and after the pass. Render passes that differ only in layout are compatible with each other.
        VkRenderPass CreateColorRenderpass(VkDevice device, VkFormat format, VkImageLayout layout);
        VkFramebuffer CreateColorFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, VkExtent2D extent);
        std::vector<VkFramebuffer> CreateFramebuffers(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& imageViews, VkImageView depthImageView, size_t count, uint32_t width, uint32_t height);
        Image CreateDepthImage(VulkanDevice& device, VulkanPresenter presenter, VmaAllocator allocator);
        VkImageView CreateDepthImageView(VulkanDevice& device, Image depthImage);
        VulkanRenderPass SetupRenderPass(VulkanDevice& vulkanDevice, VulkanPresenter& presenter, VmaAllocator& allocator,VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_4_BIT);

        VkVertexInputBindingDescription CreateBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);
        VkVertexInputAttributeDescription CreateAttributeDescription(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset);

#ifndef VULKAN_UTIL_CORE_ONLY
        TextureInfo LoadImagePixels(const std::string& filename);
        TextureInfo LoadImagePixelsFromMemory(stbi_uc* pixels, uint32_t width, uint32_t height);
        void UnloadImagePixels(TextureInfo info);
        Image CreateTextureImage(const std::string& filename, VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool);
        Image CreateTextureImageFromMemory(stbi_uc* pixels, uint32_t width, uint32_t height, VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool);
        VulkanTexture SetupVulkanTexture(const std::string& filename, VulkanDevice device, VmaAllocator allocator, VkCommandPool commandPool);
        VulkanTexture SetupVulkanTextureFromMemory(stbi_uc* pixels, uint32_t width, uint32_t height, VulkanDevice device, VmaAllocator allocator, VkCommandPool commandPool);
#endif
        VkSampler CreateTextureSampler(VulkanDevice& vulkanDevice, VkFilter filter = VK_FILTER_LINEAR);
        // Creates a texture cleared to zero, ready to be sampled and updated
        Image CreateEmptyTextureImage(VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool, VkFormat format, VkExtent2D extent, VkImageUsageFlags extraUsage = 0);
        // Replaces the given part of a texture that is in SHADER_READ_ONLY_OPTIMAL layout. Commands that are already
        // submitted will see the old contents. Waits for the upload to finish.
        void UpdateTextureImage(VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool, VkImage image, const void* pixels, VkDeviceSize size, VkOffset2D offset, VkExtent2D extent);
        VulkanOffscreenTarget CreateOffscreenTarget(VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool, VkRenderPass renderPass, VkFormat format, VkExtent2D extent);
        void DestroyOffscreenTarget(VulkanDevice vulkanDevice, VmaAllocator allocator, VulkanOffscreenTarget& target);

        VkDescriptorPool CreateDescriptorPool(VulkanDevice& device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
        VkDescriptorPoolSize CreatePoolSize(VkDescriptorType type, uint32_t count);
        std::vector<VkDescriptorSet> CreateDescriptorSet(uint32_t setAmount, VulkanDevice device, VkDescriptorPool descriptorPool, std::vector<VkDescriptorSetLayout> descriptorLayouts);
        VkDescriptorBufferInfo CreateBufferInfo(Buffer& buffer, VkDeviceSize offset, VkDeviceSize range);
        VkDescriptorImageInfo CreateImageInfo(VulkanTexture& texture, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet CreateWriteDescriptorSet(VkDescriptorSet& descSet, VkDescriptorType descriptorType, uint32_t binding, uint32_t arrayElement = 0, uint32_t descriptorCount = 1, VkDescriptorBufferInfo* bufferInfo = nullptr, VkDescriptorImageInfo* imageInfo = nullptr, VkBufferView* bufferView = nullptr);
        void UpdateDescriptorSets(VulkanDevice& device, const std::vector<VkWriteDescriptorSet>& WriteDescriptorSets);

        VkCommandPool CreateCommandPool(VulkanDevice vulkanDevice, VkCommandPoolCreateFlags flags);
        std::vector<VkCommandBuffer> CreateCommandBuffers(uint32_t bufferCount, VulkanDevice device, VkCommandPool commandPool);

        namespace RecordCommand {
            void BeginCommandBuffer(VkCommandBuffer commandBuffer);
            void BeginRenderPass(std::vector<float> _clearColor, VulkanRenderPass renderPass, VulkanPresenter presenter, VkCommandBuffer commandBuffer, uint32_t imageIndex);
            void BeginColorRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D extent);
            void EndRenderPass(VkCommandBuffer commandBuffer);
            VkResult EndCommandBuffer(VkCommandBuffer commandBuffer);
            void BindPipeline(VulkanGraphicsPipeline& pipeline, VkCommandBuffer& commandBuffer, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
            void SetScissor(VkCommandBuffer& commandBuffer, VulkanPresenter& presenter);
            void SetScissor(VkCommandBuffer& commandBuffer, const VkRect2D& scissor);
#ifndef VULKAN_UTIL_CORE_ONLY
            void SetViewport(VulkanPresenter& presenter, VkCommandBuffer& commandBuffer, const glm::vec2& offset = { 0.0f, 0.0f });
#endif
            void SetViewport(VkCommandBuffer& commandBuffer, const VkViewport& viewport);
            void BindVertexBuffer(VkCommandBuffer commandBuffer, const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets = { 0 }, uint32_t firstBinding = 0, uint32_t bindingCount = 1);
            void BindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
            void BindDescriptorSets(VkCommandBuffer& commandBuffer, VulkanGraphicsPipeline& pipeline, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t firstSet = 0);
            inline void PushConstants(VkCommandBuffer& commandBuffer,VkPipelineLayout& pipelineLayout,VkShaderStageFlags shaderStageFlags,uint32_t offset,uint32_t size,const void* Data){vkCmdPushConstants(commandBuffer,pipelineLayout,shaderStageFlags,offset,size,Data);}
            void DrawIndexed(VkCommandBuffer& commandBuffer, uint32_t indexSize, uint32_t instanceCount = 1);
            void Draw(VkCommandBuffer& commandBuffer, uint32_t vertexCount, uint32_t instanceCount = 1);
            // Clears the given part of the first color attachment of the active render pass
            void ClearColorAttachment(VkCommandBuffer& commandBuffer, const VkClearColorValue& color, const VkRect2D& rect);
        }

        FrameSyncObjects CreateSyncObjects(VulkanDevice& device, bool createSignaled = true);
        VkSemaphore CreateBinarySemaphore(VulkanDevice& device);
        uint32_t AcquireNextImage(VulkanDevice device, VulkanPresenter presenter, VkSemaphore& signalSemaphore);
        // Unlike AcquireNextImage, this reports the result so that the swapchain can be recreated when necessary
        VkResult TryAcquireNextImage(VulkanDevice& device, VulkanPresenter& presenter, VkSemaphore& signalSemaphore, uint32_t* imageIndex);
        void GraphicsQueueSubmit(VulkanDevice& device, FrameSyncObjects& syncObjects, const std::vector<VkCommandBuffer>& commandBuffers, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores);
        VkResult PresentQueueSubmit(VulkanDevice& device, VulkanPresenter& presenter, uint32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores);
    }

#ifndef VULKAN_UTIL_CORE_ONLY
    // Maksimum obje sınırımız
    constexpr uint32_t MAX_OBJECTS = 100000;

    struct GlobalStorageData {
        glm::mat4 models[MAX_OBJECTS];
    };

    struct RenderBatch {
        std::vector<glm::mat4> transforms;
    };

    struct Mesh {
        VulkanF::Buffer vertexBuffer;
        VulkanF::Buffer indexBuffer;
        uint32_t indexCount{0};
        VulkanF::VulkanTexture texture;
        std::vector<VkDescriptorSet> descriptorSets;
    };

    struct Transform{
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
    };
    struct RenderObject {
        private:
        glm::mat4 transformMatrix;
        public:
        Transform transform;
        std::shared_ptr<Mesh> mesh;
        uint32_t objectId{0};
        void ChangeTransform(float deltaTime,const glm::vec3& delta_pos=glm::vec3(0.0f),const glm::vec3& delta_rot=glm::vec3(0.0f),const glm::vec3& delta_scale=glm::vec3(0.0f)){
            transform.position+=delta_pos*deltaTime;
            transform.rotation+=delta_rot*deltaTime;
            transform.scale+=delta_scale*deltaTime;
        }
        glm::mat4 GetTransformMatrix() const{
            auto rotation=glm::rotate(glm::mat4(1.0f),glm::radians(transform.rotation.y),glm::vec3(0.0f,1.0f,0.0f))*
                            glm::rotate(glm::mat4(1.0f),glm::radians(transform.rotation.x),glm::vec3(1.0f,0.0f,0.0f))*
                            glm::rotate(glm::mat4(1.0f),glm::radians(transform.rotation.z),glm::vec3(0.0f,0.0f,1.0f));
            return  glm::translate(glm::mat4(1.0f),transform.position)*
                    rotation*
                    glm::scale(glm::mat4(1.0f),transform.scale);
        }
        RenderObject():transform({glm::vec3(0.0f,0.0f,0.0f),glm::vec3(0.0f,0.0f,0.0f),glm::vec3(1.0f,1.0f,1.0f)}){}

    };

    class VulkanEngine {
    public:
        GLFWwindow* window{nullptr};
        VkInstance instance{VK_NULL_HANDLE};
        VkSurfaceKHR surface{VK_NULL_HANDLE};
        
        VmaAllocator allocator{VK_NULL_HANDLE};
        VulkanF::VulkanDevice vulkanDevice;
        VulkanF::VulkanPresenter presenter;

        VulkanF::VulkanRenderPass vulkanRenderPass;
        VulkanF::VulkanGraphicsPipeline graphicsPipeline;

        VkCommandPool commandPool{VK_NULL_HANDLE};
        VkDescriptorPool descriptorPool{VK_NULL_HANDLE};

        uint32_t MAX_FRAMES_IN_FLIGHT{2};
        uint32_t currentFrame{0};
        
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VulkanF::FrameSyncObjects> syncObjects;

        std::vector<VulkanF::Buffer> storageBuffer;
        std::vector<void*> storageBufferMapped;

        std::vector<std::unique_ptr<RenderObject>> renderables;
        std::unordered_map<std::string, std::shared_ptr<Mesh>> meshCache;
        std::unordered_map<Mesh*, RenderBatch> batchMap;

        void Init(const char* title, const int width, const int height);
        void Cleanup();
        
        void DrawFrame(const void* pushConstantData = nullptr);
        
        std::shared_ptr<Mesh> GetOrLoadMesh(const std::string& modelPath);
        RenderObject* CreateRenderObject(const std::string& modelPath, const Transform& initialTransform ={{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}});
        
        float yaw = -90.0f; 
        float pitch = 0.0f;
    private:
        float lastX = 400.0f; 
        float lastY = 300.0f;
        bool firstMouse = true;
        float mouseSensitivity = 0.05f;

        static void cursorPositionCallback(GLFWwindow* window, double xposIn, double yposIn);
        
        void processMouseMovement(float xpos, float ypos);
        void InitWindow(const char* title, const int width, const int height);
        void InitVulkanCore(size_t descriptorCount = MAX_OBJECTS);
        void InitFramesInFlight();
        void InitPipeline();
        void BuildBatches();
    };
#endif
}
#ifdef VULKAN_UTIL_IMPLEMENTATION
namespace VulkanAbstraction {

    std::vector<char> readFile(const std::string& filename) {
        std::ifstream files(filename.c_str(), std::ios::ate | std::ios::binary);
        assert(files.is_open());
        size_t size = (size_t)files.tellg();
        files.seekg(0);
        std::vector<char> file_content;
        file_content.resize(size);
        files.read(file_content.data(), size);
        return file_content;
    }

#ifndef VULKAN_UTIL_CORE_ONLY
    void LoadModelOBJ(const std::string& MODEL_PATH, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(err);
        }
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1 - attrib.texcoords[2 * index.texcoord_index + 1]
                };
                vertex.color = { 1.0f, 1.0f, 1.0f };
                vertices.push_back(vertex);
                indices.push_back(indices.size());
            }
        }
    }

    void LoadModelGLB(const char* filePath, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices, stbi_uc** outPixels, uint32_t* outWidth, uint32_t* outHeight) {
        cgltf_options options = {};
        cgltf_data* data = nullptr;

        cgltf_result result = cgltf_parse_file(&options, filePath, &data);
        if (result != cgltf_result_success) {
            throw std::runtime_error("Cannot read glb file");
        }
        result = cgltf_load_buffers(&options, data, filePath);
        if (result != cgltf_result_success) {
            cgltf_free(data);
            throw std::runtime_error("Cannot load glb buffers");
        }

        for (size_t i = 0; i < data->meshes_count; ++i) {
            const cgltf_mesh& mesh = data->meshes[i];
            for (size_t j = 0; j < mesh.primitives_count; ++j) {
                const cgltf_primitive& primitive = mesh.primitives[j];
                size_t vertexOffset = outVertices.size();
                cgltf_accessor* posAccessor = nullptr;
                cgltf_accessor* uvAccessor = nullptr;

                for (size_t k = 0; k < primitive.attributes_count; ++k) {
                    const cgltf_attribute& attribute = primitive.attributes[k];
                    if (attribute.type == cgltf_attribute_type_position) {
                        posAccessor = attribute.data;
                    }
                    else if (attribute.type == cgltf_attribute_type_texcoord) {
                        uvAccessor = attribute.data;
                    }
                }

                if (posAccessor != nullptr) {
                    for (size_t v = 0; v < posAccessor->count; ++v) {
                        Vertex vertex{};
                        cgltf_accessor_read_float(posAccessor, v, &vertex.pos.x, 3);
                        if (uvAccessor != nullptr) {
                            cgltf_accessor_read_float(uvAccessor, v, &vertex.texCoord.x, 2);
                        }
                        else {
                            vertex.texCoord.x = 0.0f; vertex.texCoord.y = 0.0f;
                        }
                        outVertices.push_back(vertex);
                    }
                }

                if (primitive.indices != nullptr) {
                    for (size_t idx = 0; idx < primitive.indices->count; ++idx) {
                        uint32_t index = cgltf_accessor_read_index(primitive.indices, idx);
                        outIndices.push_back(index + vertexOffset);
                    }
                }
            }
        }
        if (data->images_count > 0) {
            const cgltf_image& image = data->images[0];
            if (image.buffer_view != nullptr) {
                uint8_t* rawImageData = (uint8_t*)image.buffer_view->buffer->data + image.buffer_view->offset;
                size_t rawImageSize = image.buffer_view->size;
                int texWidth, texHeight, texChannels;
                stbi_uc* pixels = stbi_load_from_memory(rawImageData, static_cast<int>(rawImageSize), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
                if (!pixels) {
                    throw std::runtime_error("Cannot resolve texture from memory!");
                }
                *outPixels = pixels;
                *outWidth = texWidth;
                *outHeight = texHeight;
            }
        }
        cgltf_free(data);
    }
#endif

    namespace SingleTimeCommands {
        VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;
            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(commandBuffer, &beginInfo);
            return commandBuffer;
        }

        void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
            vkEndCommandBuffer(commandBuffer);
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);
            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }

        void copyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
            VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
            endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
        }

        void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
            VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
            recordTransitionImageLayout(commandBuffer, image, oldLayout, newLayout);
            endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
        }

        void recordTransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                barrier.dstAccessMask = 0;
                sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }
            else {
                throw std::invalid_argument("unsupported layout transition!");
            }
            vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
            VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
            recordCopyBufferToImage(commandBuffer, buffer, image, { 0, 0 }, { width, height });
            endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
        }

        void recordCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkOffset2D offset, VkExtent2D extent) {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { offset.x, offset.y, 0 };
            region.imageExtent = { extent.width, extent.height, 1 };
            vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }
    }

    namespace VulkanF {
        VkDescriptorSetLayoutBinding CreateDescriptorSetBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags shaderStage) {
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = binding;
            layoutBinding.descriptorType = descriptorType;
            layoutBinding.descriptorCount = descriptorCount;
            layoutBinding.stageFlags = shaderStage;
            return layoutBinding;
        }

        VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanDevice device, const std::vector<VkDescriptorSetLayoutBinding>& setBindings) {
            VkDescriptorSetLayout layout;
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
            layoutInfo.pBindings = setBindings.data();
            if(vkCreateDescriptorSetLayout(device.logicalDevice, &layoutInfo, nullptr, &layout) != VK_SUCCESS){
                throw std::runtime_error("Cannot create DescriptorSet Layout");
            }

            return layout;
        }

        void VulkanGraphicsPipeline::InitDefaultPipeline(VulkanPresenter presenter, VulkanRenderPass vulkanRenderPass, uint32_t subpassid) {
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;

            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)presenter.swapchainProperties.imageExtent.width;
            viewport.height = (float)presenter.swapchainProperties.imageExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = presenter.swapchainProperties.imageExtent;
            __States.scissor = scissor;
            __States.viewport = viewport;

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &__States.viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &__States.scissor;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
            __States.colorBlendAttachement = colorBlendAttachment;

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &__States.colorBlendAttachement;

            __States.InputAssemblyState = inputAssembly;
            __States.ViewportState = viewportState;
            __States.RasterizationState = rasterizer;
            __States.MultisampleState = multisampling;
            __States.DepthStencilState = depthStencil;
            __States.ColorBlendState = colorBlending;
            __States.DynamicState = dynamicState;
            __States.renderPass = vulkanRenderPass.renderPass;
            __States.subpass = subpassid;
        }

        void VulkanGraphicsPipeline::BuildPipeline(VulkanDevice device,VkSampleCountFlagBits samples) {

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE; 
            multisampling.rasterizationSamples = samples; 
            __States.MultisampleState=multisampling;


            std::vector<VkPipelineShaderStageCreateInfo> shaderInfos;
            for (auto shader : shaders) {
                shaderInfos.push_back(shader.shaderStage);
            }
            __States.stageCount = static_cast<uint32_t>(shaderInfos.size());
            __States.pStages = shaderInfos.data();

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
            vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
            vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
            __States.VertexInputState = vertexInputInfo;

            for (auto& el : _setLayouts) {
                setLayouts.push_back(el.layout);
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = setLayouts.size();
            pipelineLayoutInfo.pSetLayouts = setLayouts.data();

            if(pushConstant.has_value()){

                pipelineLayoutInfo.pushConstantRangeCount = 1;
                pipelineLayoutInfo.pPushConstantRanges = &pushConstant.value().pushConstantRange;
            }

            if (sharedLayout != VK_NULL_HANDLE) {
                __States.layout = sharedLayout;
            }
            else if (vkCreatePipelineLayout(device.logicalDevice, &pipelineLayoutInfo, nullptr, &__States.layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = __States.stageCount;
            pipelineInfo.pStages = __States.pStages;
            pipelineInfo.pVertexInputState = &__States.VertexInputState;
            pipelineInfo.pInputAssemblyState = &__States.InputAssemblyState;
            pipelineInfo.pTessellationState = &__States.TessellationState;
            pipelineInfo.pViewportState = &__States.ViewportState;
            pipelineInfo.pRasterizationState = &__States.RasterizationState;
            pipelineInfo.pMultisampleState = &__States.MultisampleState;
            pipelineInfo.pDepthStencilState = &__States.DepthStencilState;
            pipelineInfo.pColorBlendState = &__States.ColorBlendState;
            pipelineInfo.pDynamicState = &__States.DynamicState;
            pipelineInfo.layout = __States.layout;
            pipelineInfo.renderPass = __States.renderPass;
            pipelineInfo.subpass = __States.subpass;

            if (vkCreateGraphicsPipelines(device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }
#ifndef VULKAN_UTIL_CORE_ONLY
            std::cout << "\nGraphics Pipeline Created Successfully!";
#endif
            for (auto& shader : shaders) {
                vkDestroyShaderModule(device.logicalDevice, shader.__shaderModule, nullptr);
            }
        }

        void VulkanGraphicsPipeline::AddShaderStage(VulkanDevice device, const std::string& filename, VkShaderStageFlagBits shaderStage) {
            shaders.push_back(SetupShader(device, filename, shaderStage));
        }
        void VulkanGraphicsPipeline::AddShaderStageFromSPIRV(VulkanDevice device, const std::vector<char>& code, VkShaderStageFlagBits shaderStage) {
            VulkanShader shader{};
            shader.__shaderModule = CreateShaderModule(device, code);
            shader.shaderStage = CreateShaderStage(shader.__shaderModule, shaderStage);
            shaders.push_back(shader);
        }
        void VulkanGraphicsPipeline::AddPushConstants(VkShaderStageFlags shaderStage,uint32_t size,uint32_t offset){
            VkPushConstantRange pushconstRange{};
            pushconstRange.offset=offset;
            pushconstRange.size=size;
            pushconstRange.stageFlags=shaderStage;
            VulkanPushConstant pushConstant{};
            pushConstant.pushConstantRange = pushconstRange;
            pushConstant.offset = offset;
            pushConstant.size = size;
            pushConstant.flags = shaderStage;
            this->pushConstant = std::make_optional(pushConstant);
        }
        namespace Memory {
            Image CreateImage(VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, VmaAllocationCreateFlags flags,VkSampleCountFlagBits sampleFlags) {
                Image image;
                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = width;
                imageInfo.extent.height = height;
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.format = format;
                imageInfo.tiling = tiling;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = usage;
                imageInfo.samples = sampleFlags;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                VmaAllocationCreateInfo allocinfo{};
                allocinfo.usage = memUsage;
                allocinfo.flags = flags;

                if (vmaCreateImage(allocator, &imageInfo, &allocinfo, &image.image, &image.deviceMemory, nullptr) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create image!");
                }
                return image;
            }

            Buffer CreateBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memUsage, VmaAllocationCreateFlags flags) {
                Buffer buffer{};
                VkBufferCreateInfo bufferInfo{};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = size;
                bufferInfo.usage = bufferUsage;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                VmaAllocationCreateInfo allocInfo{};
                allocInfo.usage = memUsage;
                allocInfo.flags = flags;
                if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.deviceMemory, nullptr) != VK_SUCCESS) {
                    throw std::runtime_error("Buffer olusturulamadi!");
                }
                
                return buffer;
            }

            void MapMemory(VmaAllocator allocator, BaseMemoryAllocatable* allocatableType, void** Data) {
                vmaMapMemory(allocator, allocatableType->deviceMemory, Data);
            }
            void UnMapMemory(VmaAllocator allocator, BaseMemoryAllocatable* allocatableType) {
                vmaUnmapMemory(allocator, allocatableType->deviceMemory);
            }
        }

        namespace SwapchainHelpers {
                bool SupportsDeviceExtensions(VkPhysicalDevice device,const std::vector<const char*>& required) {
                    uint32_t count = 0;
                    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS)
                        return false;

                    std::vector<VkExtensionProperties> available(count);
                    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data()) != VK_SUCCESS)
                        return false;

                    for (const char* name : required) {
                        bool found = std::any_of(available.begin(), available.end(),
                            [name](const VkExtensionProperties& ext) {
                                return std::strcmp(ext.extensionName, name) == 0;
                            });
                        if (!found) return false;
                    }
                    return true;
                }
                bool IsSwapchainAdequate(VkPhysicalDevice device, VkSurfaceKHR surface) {
                    uint32_t formatCount = 0;
                    if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr) != VK_SUCCESS)
                        return false;

                    uint32_t presentModeCount = 0;
                    if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr) != VK_SUCCESS)
                        return false;

                    return formatCount > 0 && presentModeCount > 0;
                }
            VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
                VkSurfaceCapabilitiesKHR caps;
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);
                return caps;
            }
            VkPresentModeKHR ChoosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
                std::vector<VkPresentModeKHR> presentModes(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
                for (const auto& availablePresentMode : presentModes) {
                    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                        return availablePresentMode;
                    }
                }
                return VK_PRESENT_MODE_FIFO_KHR;
            }
            VkExtent2D GetImageExtent(void* window, VkSurfaceCapabilitiesKHR capabilities) {
                if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                    return capabilities.currentExtent;
                }
                else {
                    int width, height;
#ifdef GLFW_INCLUDE_VULKAN
                    glfwGetFramebufferSize((GLFWwindow*)window, &width, &height);
#else
                    throw std::runtime_error("Not Supported Window!");
#endif
                    VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
                    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                    return actualExtent;
                }
            }
            VkExtent2D GetImageExtent(VkExtent2D windowExtent, VkSurfaceCapabilitiesKHR capabilities) {
                if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                    return capabilities.currentExtent;
                }
                windowExtent.width = std::clamp(windowExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                windowExtent.height = std::clamp(windowExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                return windowExtent;
            }
            VkSurfaceFormatKHR ChooseSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<VkFormat>& preferredFormats) {
                uint32_t formatCount;
                std::vector<VkSurfaceFormatKHR> formats;
                vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
                formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
                for (VkFormat preferredFormat : preferredFormats) {
                    for (const auto& availableFormat : formats) {
                        if (availableFormat.format == preferredFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                            return availableFormat;
                        }
                    }
                }
                return formats[0];
            }
            std::vector<VkImage> GetSwapchainImages(VkDevice logicalDevice, VkSwapchainKHR swapChain) {
                uint32_t imageCount;
                vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
                std::vector<VkImage> swapChainImages(imageCount);
                vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
                return swapChainImages;
            }
            VkImageView CreateImageView(VkDevice logicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkComponentMapping components) {
                VkImageView iv;
                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = image;
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = format;
                createInfo.components = components;
                createInfo.subresourceRange.aspectMask = aspectFlags;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;
                if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &iv) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create image views!");
                }
                return iv;
            }
            std::vector<VkImageView> CreateSwapChainImageViews(VkDevice logicalDevice, const std::vector<VkImage>& swapChainImages, VkSurfaceFormatKHR surfaceFormat) {
                std::vector<VkImageView> swapChainImageViews;
                swapChainImageViews.resize(swapChainImages.size());
                for (size_t i = 0; i < swapChainImageViews.size(); i++) {
                    swapChainImageViews[i] = VulkanF::SwapchainHelpers::CreateImageView(logicalDevice, swapChainImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
                }
                return swapChainImageViews;
            }
        }

        VkShaderModule CreateShaderModule(VulkanDevice device, const std::vector<char>& code) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
            VkShaderModule shadermodule;
            if (vkCreateShaderModule(device.logicalDevice, &createInfo, nullptr, &shadermodule) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module!");
            }
            return shadermodule;
        }

        VkPipelineShaderStageCreateInfo CreateShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage) {
            VkPipelineShaderStageCreateInfo ShaderStageInfo{};
            ShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            ShaderStageInfo.stage = stage;
            ShaderStageInfo.module = shaderModule;
            ShaderStageInfo.pName = "main";
            return ShaderStageInfo;
        }

        VulkanShader SetupShader(VulkanDevice device, const std::string& filename, VkShaderStageFlagBits shaderStage) {
            auto binaryCode = readFile(filename);
            VulkanShader shader{};
            shader.__shaderModule = CreateShaderModule(device, binaryCode);
            shader.shaderStage = CreateShaderStage(shader.__shaderModule, shaderStage);
            return shader;
        }

        namespace DepthHelpers {
            VkFormat findSupportedFormat(VkPhysicalDevice phy, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
                for (VkFormat format : candidates) {
                    VkFormatProperties props;
                    vkGetPhysicalDeviceFormatProperties(phy, format, &props);
                    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                        return format;
                    }
                    else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                        return format;
                    }
                }
                throw std::runtime_error("failed to find supported format!");
            }
            VkFormat findDepthFormat(VkPhysicalDevice phy) {
                return findSupportedFormat(phy,
                    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                );
            }
            bool hasStencilComponent(VkFormat format) {
                return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
            }
        }

#ifndef VULKAN_UTIL_CORE_ONLY
        VkInstance CreateInstance(uint32_t api_version, const std::string& appName, const std::string& engineName) {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            VkInstance instance = CreateInstance(std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount), validationLayers, api_version, appName, engineName);
            std::cout << "Instance Created!";
            return instance;
        }
#endif 


        VkInstance CreateInstance(const std::vector<const char*>& extensions, const std::vector<const char*>& layers, uint32_t api_version, const std::string& appName, const std::string& engineName) {
            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = appName.c_str();
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = engineName.c_str();
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = api_version;
            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;
            VkInstance instance;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
            createInfo.ppEnabledLayerNames = layers.data();
            if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
                throw std::runtime_error("failed to create instance!");
            }
            return instance;
        }

        bool SupportsDeviceExtensions(VkPhysicalDevice device,const std::vector<const char*>& required) {
            uint32_t count = 0;
            if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS)
                return false;

            std::vector<VkExtensionProperties> available(count);
            if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data()) != VK_SUCCESS)
                return false;

            for (const char* name : required) {
                bool found = std::any_of(available.begin(), available.end(),
                    [name](const VkExtensionProperties& ext) {
                        return std::strcmp(ext.extensionName, name) == 0;
                    });
                if (!found) return false;
            }
            return true;
        }
        bool IsSwapchainAdequate(VkPhysicalDevice device, VkSurfaceKHR surface) {
            uint32_t formatCount = 0;
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr) != VK_SUCCESS)
                return false;

            uint32_t presentModeCount = 0;
            if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr) != VK_SUCCESS)
                return false;

            return formatCount > 0 && presentModeCount > 0;
        }
        VkPhysicalDevice PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
            if (deviceCount <= 0) { throw std::runtime_error("There is no vulkan supported physical device"); }
            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
            VkPhysicalDevice nonDiscrete=VK_NULL_HANDLE;
            for (auto& device : devices) {
                VkPhysicalDeviceFeatures deviceFeatures;
                vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);
                QueueFamilyIndices indices=PickQueueFamilyIndices(device,surface);
                if(!indices.isComplete()) continue;
                VkBool32 supportsPresentation;
                vkGetPhysicalDeviceSurfaceSupportKHR(device,indices.presentationFamily.value(),surface,&supportsPresentation);
                
                if (    deviceProperties.apiVersion>=VK_API_VERSION_1_1 &&
                        supportsPresentation &&
                        deviceFeatures.geometryShader &&
                        SupportsDeviceExtensions(device, { VK_KHR_SWAPCHAIN_EXTENSION_NAME })&&
                        IsSwapchainAdequate(device, surface) &&
                        indices.isComplete()
                    ) {
                    if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){   
                        return device;
                    }
                    nonDiscrete=device;
                }

            }
            if(nonDiscrete!=VK_NULL_HANDLE){
                return nonDiscrete;
            }else{
                throw std::runtime_error("There is no graphics device with presentation compatibility !");
            }
        }

        QueueFamilyIndices PickQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
            QueueFamilyIndices queuefamilyindices{};
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
            uint32_t i = 0;
            for (auto& fam : queueFamilies) {
                if (fam.queueFlags & VK_QUEUE_GRAPHICS_BIT) { queuefamilyindices.graphicsFamily = i; }
                VkBool32 presentSupport;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if (presentSupport) { queuefamilyindices.presentationFamily = i; }
                i++;
            }
            return queuefamilyindices;
        }

        VkDevice CreateLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices queuefamilyindices, VkQueue* graphicsQueue, VkQueue* presentQueue) {
            std::set<uint32_t> uniqueQueueFamilies = { queuefamilyindices.graphicsFamily.value(), queuefamilyindices.presentationFamily.value() };
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }
            VkPhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.samplerAnisotropy = VK_FALSE;
            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pEnabledFeatures = &deviceFeatures;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(VulkanF::deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = VulkanF::deviceExtensions.data();
            createInfo.enabledLayerCount = 0;
            VkDevice logicalDevice;
            if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            }
            vkGetDeviceQueue(logicalDevice, queuefamilyindices.graphicsFamily.value(), 0, graphicsQueue);
            vkGetDeviceQueue(logicalDevice, queuefamilyindices.presentationFamily.value(), 0, presentQueue);
            return logicalDevice;
        }

        VkSwapchainKHR CreateSwapchain(VkDevice logicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR capabilities, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkExtent2D imageExtent, QueueFamilyIndices familyIndices, VkSwapchainKHR oldSwapchain) {
            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;
            createInfo.minImageCount = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount > 0 && createInfo.minImageCount > capabilities.maxImageCount) {
                createInfo.minImageCount = capabilities.maxImageCount;
            }
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = imageExtent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            uint32_t queueFamilyIndices[] = { familyIndices.graphicsFamily.value(), familyIndices.presentationFamily.value() };
            if (familyIndices.graphicsFamily != familyIndices.presentationFamily) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            createInfo.preTransform = capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = oldSwapchain;
            VkSwapchainKHR swapChain;
            if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
                throw std::runtime_error("failed to create swap chain!");
            }
            return swapChain;
        }

        VulkanDevice SetupDevice(VkInstance instance, VkSurfaceKHR surface) {
            VulkanDevice device{};
            device.physicalDevice = PickPhysicalDevice(instance,surface);
            device.familyIndices = PickQueueFamilyIndices(device.physicalDevice, surface);
            device.logicalDevice = CreateLogicalDevice(device.physicalDevice, device.familyIndices, &device.graphicsQueue, &device.presentQueue);
            return device;
        }

        VmaAllocator CreateAllocator(VkInstance instance, VulkanDevice& vulkanDevice) {
            VmaAllocatorCreateInfo createinfo{};
            createinfo.instance = instance;
            createinfo.physicalDevice = vulkanDevice.physicalDevice;
            createinfo.device = vulkanDevice.logicalDevice;
            createinfo.vulkanApiVersion = VK_API_VERSION_1_0;
            VmaAllocator allocator{};
            if(vmaCreateAllocator(&createinfo, &allocator) != VK_SUCCESS){
                throw std::runtime_error("Cannot create allocator!");
            }
            return allocator;
        }

        VulkanPresenter SetupPresenter(VulkanDevice& device, VkExtent2D windowExtent, VkSurfaceKHR surface, const std::vector<VkFormat>& preferredFormats, VkSwapchainKHR oldSwapchain) {
            VulkanPresenter presenter{};
            presenter.surface = surface;
            presenter.swapchainProperties.surfaceCapabilities = SwapchainHelpers::GetSurfaceCapabilities(device.physicalDevice, surface);
            presenter.swapchainProperties.presentMode = SwapchainHelpers::ChoosePresentMode(device.physicalDevice, surface);
            presenter.swapchainProperties.surfaceFormat = SwapchainHelpers::ChooseSurfaceFormats(device.physicalDevice, surface, preferredFormats);
            presenter.swapchainProperties.imageExtent = SwapchainHelpers::GetImageExtent(windowExtent, presenter.swapchainProperties.surfaceCapabilities);
            presenter.swapChain = CreateSwapchain(device.logicalDevice, surface, presenter.swapchainProperties.surfaceCapabilities, presenter.swapchainProperties.surfaceFormat, presenter.swapchainProperties.presentMode, presenter.swapchainProperties.imageExtent, device.familyIndices, oldSwapchain);
            presenter.swapChainImages = SwapchainHelpers::GetSwapchainImages(device.logicalDevice, presenter.swapChain);
            presenter.swapChainImageViews = SwapchainHelpers::CreateSwapChainImageViews(device.logicalDevice, presenter.swapChainImages, presenter.swapchainProperties.surfaceFormat);
            return presenter;
        }

#ifndef VULKAN_UTIL_CORE_ONLY
        VulkanPresenter SetupPresenter(VulkanDevice& device, void* window, VkSurfaceKHR surface) {
            VulkanPresenter presenter{};
            presenter.swapchainProperties.surfaceCapabilities = SwapchainHelpers::GetSurfaceCapabilities(device.physicalDevice, surface);
            presenter.swapchainProperties.presentMode = SwapchainHelpers::ChoosePresentMode(device.physicalDevice, surface);
            presenter.swapchainProperties.surfaceFormat = SwapchainHelpers::ChooseSurfaceFormats(device.physicalDevice, surface);
#ifdef GLFW_INCLUDE_VULKAN
            presenter.swapchainProperties.imageExtent = SwapchainHelpers::GetImageExtent((GLFWwindow*)window, presenter.swapchainProperties.surfaceCapabilities);
#endif
            presenter.swapChain = CreateSwapchain(device.logicalDevice, surface, presenter.swapchainProperties.surfaceCapabilities, presenter.swapchainProperties.surfaceFormat, presenter.swapchainProperties.presentMode, presenter.swapchainProperties.imageExtent, device.familyIndices);
            presenter.swapChainImages = SwapchainHelpers::GetSwapchainImages(device.logicalDevice, presenter.swapChain);
            presenter.swapChainImageViews = SwapchainHelpers::CreateSwapChainImageViews(device.logicalDevice, presenter.swapChainImages, presenter.swapchainProperties.surfaceFormat);
            return presenter;
        }
#endif

        VkRenderPass CreateColorRenderpass(VkDevice device, VkFormat format, VkImageLayout layout) {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = format;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = layout;
            colorAttachment.finalLayout = layout;

            VkAttachmentReference colorAttachmentRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            // The attachment could have been written or sampled by earlier passes, and it could be sampled by later ones
            std::array<VkSubpassDependency, 2> dependencies{};
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies = dependencies.data();

            VkRenderPass renderPass;
            if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
            return renderPass;
        }

        VkFramebuffer CreateColorFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, VkExtent2D extent) {
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &imageView;
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;
            VkFramebuffer frameBuffer;
            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
            return frameBuffer;
        }

        VkRenderPass CreateRenderpass(VkPhysicalDevice phy, VkDevice log, VkFormat format,VkSampleCountFlagBits samples) {

            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = format;
            colorAttachment.samples = samples;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; 
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = DepthHelpers::findDepthFormat(phy);
            depthAttachment.samples = samples;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription colorAttachmentResolve{};
            colorAttachmentResolve.format = format;
            colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
            colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            VkAttachmentReference depthAttachmentRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkAttachmentReference colorAttachmentResolveRef{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
            subpass.pResolveAttachments = &colorAttachmentResolveRef; 

            VkSubpassDependency dependency{};
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            VkRenderPass renderPass;
            vkCreateRenderPass(log, &renderPassInfo, nullptr, &renderPass);
            return renderPass;
        }

        std::vector<VkFramebuffer> CreateFramebuffers(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& imageViews, VkImageView depthImageView, size_t count, uint32_t width, uint32_t height) {
            std::vector<VkFramebuffer> frameBuffers;
            frameBuffers.resize(count);
            for (size_t i = 0; i < count; i++) {
                VkImageView attachments[] = { imageViews[i], depthImageView };
                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass;
                framebufferInfo.attachmentCount = 2;
                framebufferInfo.pAttachments = attachments;
                framebufferInfo.width = width;
                framebufferInfo.height = height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
            return frameBuffers;
        }

        Image CreateDepthImage(VulkanDevice& device, VulkanPresenter presenter, VmaAllocator allocator) {
            VkFormat depthFormat = DepthHelpers::findDepthFormat(device.physicalDevice);
            return VulkanF::Memory::CreateImage(allocator, presenter.swapchainProperties.imageExtent.width, presenter.swapchainProperties.imageExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, 0);
        }

        VkImageView CreateDepthImageView(VulkanDevice& device, Image depthImage) {
            return SwapchainHelpers::CreateImageView(device.logicalDevice, depthImage.image, DepthHelpers::findDepthFormat(device.physicalDevice), VK_IMAGE_ASPECT_DEPTH_BIT);
        }

        VulkanRenderPass SetupRenderPass(VulkanDevice& vulkanDevice, VulkanPresenter& presenter, VmaAllocator& allocator,VkSampleCountFlagBits samples) {
            VulkanRenderPass renderpasss = {};
            renderpasss.renderPass = CreateRenderpass(vulkanDevice.physicalDevice, vulkanDevice.logicalDevice, presenter.swapchainProperties.surfaceFormat.format,samples);
            
            renderpasss.msaaColorImage = VulkanF::Memory::CreateImage(
                allocator, presenter.swapchainProperties.imageExtent.width, presenter.swapchainProperties.imageExtent.height,
                presenter.swapchainProperties.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, 0, samples
            );
            renderpasss.msaaColorImageView = SwapchainHelpers::CreateImageView(vulkanDevice.logicalDevice, renderpasss.msaaColorImage.image, presenter.swapchainProperties.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

            VkFormat depthFormat = DepthHelpers::findDepthFormat(vulkanDevice.physicalDevice);
            renderpasss.depthImage = VulkanF::Memory::CreateImage(
                allocator, presenter.swapchainProperties.imageExtent.width, presenter.swapchainProperties.imageExtent.height,
                depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, 0, samples
            );
            renderpasss.depthImageView = SwapchainHelpers::CreateImageView(vulkanDevice.logicalDevice, renderpasss.depthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

            renderpasss.frameBuffers.resize(presenter.swapChainImages.size());
            for (size_t i = 0; i < presenter.swapChainImages.size(); i++) {
                VkImageView attachments[] = { renderpasss.msaaColorImageView, renderpasss.depthImageView, presenter.swapChainImageViews[i] };
                
                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderpasss.renderPass;
                framebufferInfo.attachmentCount = 3;
                framebufferInfo.pAttachments = attachments;
                framebufferInfo.width = presenter.swapchainProperties.imageExtent.width;
                framebufferInfo.height = presenter.swapchainProperties.imageExtent.height;
                framebufferInfo.layers = 1;
                vkCreateFramebuffer(vulkanDevice.logicalDevice, &framebufferInfo, nullptr, &renderpasss.frameBuffers[i]);
            }
            return renderpasss;
        }

        VkVertexInputBindingDescription CreateBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = binding;
            bindingDescription.stride = stride;
            bindingDescription.inputRate = inputRate;
            return bindingDescription;
        }

        VkVertexInputAttributeDescription CreateAttributeDescription(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset) {
            VkVertexInputAttributeDescription attributeDescriptions{};
            attributeDescriptions.binding = binding;
            attributeDescriptions.location = location;
            attributeDescriptions.format = format;
            attributeDescriptions.offset = offset;
            return attributeDescriptions;
        }

#ifndef VULKAN_UTIL_CORE_ONLY
        TextureInfo LoadImagePixels(const std::string& filename) {
            TextureInfo info;
            info.pixels = stbi_load(filename.c_str(), &info.width, &info.height, &info.channels, STBI_rgb_alpha);
            info.size = info.width * info.height * 4;
            if (!info.pixels) { throw std::runtime_error("failed to load texture image!"); }
            return info;
        }

        TextureInfo LoadImagePixelsFromMemory(stbi_uc* pixels, uint32_t width, uint32_t height) {
            TextureInfo info;
            info.pixels = pixels;
            info.width = width;
            info.height = height;
            info.channels = 3;
            info.size = width * height * 4;
            if (!info.pixels) { throw std::runtime_error("failed to load texture image!"); }
            return info;
        }

        void UnloadImagePixels(TextureInfo info) { stbi_image_free(info.pixels); }

        Image CreateTextureImage(const std::string& filename, VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool) {
            VulkanF::TextureInfo info = VulkanF::LoadImagePixels(filename.c_str());
            auto StagingBuffer = VulkanF::Memory::CreateBuffer(allocator, info.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
            auto TextureImage = VulkanF::Memory::CreateImage(allocator, info.width, info.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, 0);
            void* texdata;
            VulkanF::Memory::MapMemory(allocator, &StagingBuffer, &texdata);
            memcpy(texdata, info.pixels, info.size);
            VulkanF::Memory::UnMapMemory(allocator, &StagingBuffer);
            VulkanF::UnloadImagePixels(info);
            SingleTimeCommands::transitionImageLayout(vulkanDevice.logicalDevice, vulkanDevice.graphicsQueue, commandPool, TextureImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            SingleTimeCommands::copyBufferToImage(vulkanDevice.logicalDevice, vulkanDevice.graphicsQueue, commandPool, StagingBuffer.buffer, TextureImage.image, static_cast<uint32_t>(info.width), static_cast<uint32_t>(info.height));
            SingleTimeCommands::transitionImageLayout(vulkanDevice.logicalDevice, vulkanDevice.graphicsQueue, commandPool, TextureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            return TextureImage;
        }

        Image CreateTextureImageFromMemory(stbi_uc* pixels, uint32_t width, uint32_t height, VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool) {
            VulkanF::TextureInfo info = VulkanF::LoadImagePixelsFromMemory(pixels, width, height);
            auto StagingBuffer = VulkanF::Memory::CreateBuffer(allocator, info.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
            auto TextureImage = VulkanF::Memory::CreateImage(allocator, info.width, info.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, 0);
            void* texdata;
            VulkanF::Memory::MapMemory(allocator, &StagingBuffer, &texdata);
            memcpy(texdata, info.pixels, info.size);
            VulkanF::Memory::UnMapMemory(allocator, &StagingBuffer);
            VulkanF::UnloadImagePixels(info);
            SingleTimeCommands::transitionImageLayout(vulkanDevice.logicalDevice, vulkanDevice.graphicsQueue, commandPool, TextureImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            SingleTimeCommands::copyBufferToImage(vulkanDevice.logicalDevice, vulkanDevice.graphicsQueue, commandPool, StagingBuffer.buffer, TextureImage.image, static_cast<uint32_t>(info.width), static_cast<uint32_t>(info.height));
            SingleTimeCommands::transitionImageLayout(vulkanDevice.logicalDevice, vulkanDevice.graphicsQueue, commandPool, TextureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            return TextureImage;
        }
#endif

        Image CreateEmptyTextureImage(VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool, VkFormat format, VkExtent2D extent, VkImageUsageFlags extraUsage) {
            auto TextureImage = VulkanF::Memory::CreateImage(allocator, extent.width, extent.height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsage, VMA_MEMORY_USAGE_GPU_ONLY, 0);

            VkClearColorValue clearColor{};
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.levelCount = 1;
            range.layerCount = 1;

            VkCommandBuffer commandBuffer = SingleTimeCommands::beginSingleTimeCommands(vulkanDevice.logicalDevice, commandPool);
            SingleTimeCommands::recordTransitionImageLayout(commandBuffer, TextureImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            vkCmdClearColorImage(commandBuffer, TextureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
            SingleTimeCommands::recordTransitionImageLayout(commandBuffer, TextureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            SingleTimeCommands::endSingleTimeCommands(vulkanDevice.logicalDevice, vulkanDevice.graphicsQueue, commandPool, commandBuffer);
            return TextureImage;
        }

        void UpdateTextureImage(VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool, VkImage image, const void* pixels, VkDeviceSize size, VkOffset2D offset, VkExtent2D extent) {
            auto StagingBuffer = VulkanF::Memory::CreateBuffer(allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
            void* texdata;
            VulkanF::Memory::MapMemory(allocator, &StagingBuffer, &texdata);
            memcpy(texdata, pixels, size);
            vmaFlushAllocation(allocator, StagingBuffer.deviceMemory, 0, VK_WHOLE_SIZE);
            VulkanF::Memory::UnMapMemory(allocator, &StagingBuffer);

            VkCommandBuffer commandBuffer = SingleTimeCommands::beginSingleTimeCommands(vulkanDevice.logicalDevice, commandPool);
            SingleTimeCommands::recordTransitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            SingleTimeCommands::recordCopyBufferToImage(commandBuffer, StagingBuffer.buffer, image, offset, extent);
            SingleTimeCommands::recordTransitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            SingleTimeCommands::endSingleTimeCommands(vulkanDevice.logicalDevice, vulkanDevice.graphicsQueue, commandPool, commandBuffer);

            vmaDestroyBuffer(allocator, StagingBuffer.buffer, StagingBuffer.deviceMemory);
        }

        VulkanOffscreenTarget CreateOffscreenTarget(VulkanDevice vulkanDevice, VmaAllocator allocator, VkCommandPool commandPool, VkRenderPass renderPass, VkFormat format, VkExtent2D extent) {
            VulkanOffscreenTarget target{};
            target.extent = extent;
            target.image = CreateEmptyTextureImage(vulkanDevice, allocator, commandPool, format, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            target.imageView = SwapchainHelpers::CreateImageView(vulkanDevice.logicalDevice, target.image.image, format, VK_IMAGE_ASPECT_COLOR_BIT);
            target.frameBuffer = CreateColorFramebuffer(vulkanDevice.logicalDevice, renderPass, target.imageView, extent);
            return target;
        }

        void DestroyOffscreenTarget(VulkanDevice vulkanDevice, VmaAllocator allocator, VulkanOffscreenTarget& target) {
            vkDestroyFramebuffer(vulkanDevice.logicalDevice, target.frameBuffer, nullptr);
            vkDestroyImageView(vulkanDevice.logicalDevice, target.imageView, nullptr);
            vmaDestroyImage(allocator, target.image.image, target.image.deviceMemory);
            target = {};
        }

        VkSampler CreateTextureSampler(VulkanDevice& vulkanDevice, VkFilter filter) {
            VkSampler textureSampler{};
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = filter;
            samplerInfo.minFilter = filter;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            if (vkCreateSampler(vulkanDevice.logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture sampler!");
            }
            return textureSampler;
        }

#ifndef VULKAN_UTIL_CORE_ONLY
        VulkanTexture SetupVulkanTexture(const std::string& filename, VulkanDevice device, VmaAllocator allocator, VkCommandPool commandPool) {
            VulkanTexture vulkanTexture{};
            auto textureInfo = LoadImagePixels(filename.c_str());
            vulkanTexture.textureImage = CreateTextureImageFromMemory(textureInfo.pixels, textureInfo.width, textureInfo.height, device, allocator, commandPool);
            vulkanTexture.textureImageView = SwapchainHelpers::CreateImageView(device.logicalDevice, vulkanTexture.textureImage.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
            vulkanTexture.textureSampler = CreateTextureSampler(device);
            return vulkanTexture;
        }

        VulkanTexture SetupVulkanTextureFromMemory(stbi_uc* pixels, uint32_t width, uint32_t height, VulkanDevice device, VmaAllocator allocator, VkCommandPool commandPool) {
            VulkanTexture vulkanTexture{};
            vulkanTexture.textureImage = CreateTextureImageFromMemory(pixels, width, height, device, allocator, commandPool);
            vulkanTexture.textureImageView = SwapchainHelpers::CreateImageView(device.logicalDevice, vulkanTexture.textureImage.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
            vulkanTexture.textureSampler = CreateTextureSampler(device);
            return vulkanTexture;
        }
#endif

        VkDescriptorPool CreateDescriptorPool(VulkanDevice& device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets) {
            VkDescriptorPool descriptorPool{};
            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = maxSets;
            if (vkCreateDescriptorPool(device.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor pool!");
            }
            return descriptorPool;
        }

        VkDescriptorPoolSize CreatePoolSize(VkDescriptorType type, uint32_t count) {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = type;
            poolSize.descriptorCount = count;
            return poolSize;
        }

        std::vector<VkDescriptorSet> CreateDescriptorSet(uint32_t setAmount, VulkanDevice device, VkDescriptorPool descriptorPool, std::vector<VkDescriptorSetLayout> descriptorLayouts) {
            std::vector<VkDescriptorSet> descriptorsets;
            descriptorLayouts.resize(setAmount);
            descriptorsets.resize(setAmount);
            auto setLayout=descriptorLayouts[0];
            for (size_t i = 0; i < setAmount; i++)
            {
                descriptorLayouts[i]=setLayout;
            }
            
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPool;
            allocInfo.descriptorSetCount = descriptorsets.size();
            allocInfo.pSetLayouts = descriptorLayouts.data();
            if (vkAllocateDescriptorSets(device.logicalDevice, &allocInfo, descriptorsets.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }
            return descriptorsets;
        }

        VkDescriptorBufferInfo CreateBufferInfo(Buffer& buffer, VkDeviceSize offset, VkDeviceSize range) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buffer.buffer;
            bufferInfo.offset = offset;
            bufferInfo.range = range;
            return bufferInfo;
        }

        VkDescriptorImageInfo CreateImageInfo(VulkanTexture& texture, VkImageLayout imageLayout) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = imageLayout;
            imageInfo.imageView = texture.textureImageView;
            imageInfo.sampler = texture.textureSampler;
            return imageInfo;
        }

        VkWriteDescriptorSet CreateWriteDescriptorSet(VkDescriptorSet& descSet, VkDescriptorType descriptorType, uint32_t binding, uint32_t arrayElement, uint32_t descriptorCount, VkDescriptorBufferInfo* bufferInfo, VkDescriptorImageInfo* imageInfo, VkBufferView* bufferView) {
            VkWriteDescriptorSet descriptorWrites{};
            descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites.dstSet = descSet;
            descriptorWrites.dstBinding = binding;
            descriptorWrites.dstArrayElement = arrayElement;
            descriptorWrites.descriptorType = descriptorType;
            descriptorWrites.descriptorCount = descriptorCount;
            descriptorWrites.pBufferInfo = bufferInfo;
            descriptorWrites.pImageInfo = imageInfo;
            descriptorWrites.pTexelBufferView = bufferView;
            return descriptorWrites;
        }

        void UpdateDescriptorSets(VulkanDevice& device, const std::vector<VkWriteDescriptorSet>& WriteDescriptorSets) {
            vkUpdateDescriptorSets(device.logicalDevice, WriteDescriptorSets.size(), WriteDescriptorSets.data(), 0, nullptr);
        }

        VkCommandPool CreateCommandPool(VulkanDevice vulkanDevice, VkCommandPoolCreateFlags flags) {
            VkCommandPool commandPool;
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = flags;
            poolInfo.queueFamilyIndex = vulkanDevice.familyIndices.graphicsFamily.value();
            if (vkCreateCommandPool(vulkanDevice.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create command pool!");
            }
            return commandPool;
        }

        std::vector<VkCommandBuffer> CreateCommandBuffers(uint32_t bufferCount, VulkanDevice device, VkCommandPool commandPool) {
            std::vector<VkCommandBuffer> commandBuffers;
            commandBuffers.resize(bufferCount);
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = bufferCount;
            if (vkAllocateCommandBuffers(device.logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate command buffers!");
            }
            return commandBuffers;
        }

        namespace RecordCommand {
            void BeginCommandBuffer(VkCommandBuffer commandBuffer) {
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
                    throw std::runtime_error("failed to begin recording command buffer!");
                }
            }
            void BeginRenderPass(std::vector<float> _clearColor, VulkanRenderPass renderPass, VulkanPresenter presenter, VkCommandBuffer commandBuffer, uint32_t imageIndex) {
                assert(_clearColor.size() >= 4);
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = renderPass.renderPass;
                renderPassInfo.framebuffer = renderPass.frameBuffers[imageIndex];
                renderPassInfo.renderArea.offset = { 0, 0 };
                renderPassInfo.renderArea.extent = presenter.swapchainProperties.imageExtent;

                VkClearValue clearColor[2];
                clearColor[0].color = { {_clearColor[0],_clearColor[1],_clearColor[2],_clearColor[3]} };
                clearColor[1].depthStencil = { 1.0f,0 };

                renderPassInfo.clearValueCount = 2;
                renderPassInfo.pClearValues = clearColor;
                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            }
            void BeginColorRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D extent) {
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = renderPass;
                renderPassInfo.framebuffer = frameBuffer;
                renderPassInfo.renderArea.offset = { 0, 0 };
                renderPassInfo.renderArea.extent = extent;
                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            }
            void EndRenderPass(VkCommandBuffer commandBuffer) {
                vkCmdEndRenderPass(commandBuffer);
            }
            VkResult EndCommandBuffer(VkCommandBuffer commandBuffer) {
                return vkEndCommandBuffer(commandBuffer);
            }
            void BindPipeline(VulkanGraphicsPipeline& pipeline, VkCommandBuffer& commandBuffer, VkPipelineBindPoint bindPoint) {
                vkCmdBindPipeline(commandBuffer, bindPoint, pipeline.pipeline);
            }
            void SetScissor(VkCommandBuffer& commandBuffer, VulkanPresenter& presenter) {
                VkRect2D scissor{};
                scissor.offset = {0, 0};
                scissor.extent = {presenter.swapchainProperties.imageExtent.width, presenter.swapchainProperties.imageExtent.height};
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            }
            void SetScissor(VkCommandBuffer& commandBuffer, const VkRect2D& scissor) {
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            }
            void SetViewport(VkCommandBuffer& commandBuffer, const VkViewport& viewport) {
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            }
#ifndef VULKAN_UTIL_CORE_ONLY
            void SetViewport(VulkanPresenter& presenter, VkCommandBuffer& commandBuffer, const glm::vec2& offset) {
                VkViewport viewport{};
                viewport.x = offset.x;
                viewport.y = offset.y;
                viewport.width = static_cast<float>(presenter.swapchainProperties.imageExtent.width);
                viewport.height = static_cast<float>(presenter.swapchainProperties.imageExtent.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            }
#endif
            void BindVertexBuffer(VkCommandBuffer commandBuffer, const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets, uint32_t firstBinding, uint32_t bindingCount) {
                vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, buffers.data(), offsets.data());
            }
            void BindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
                vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
            }
            void BindDescriptorSets(VkCommandBuffer& commandBuffer, VulkanGraphicsPipeline& pipeline, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t firstSet) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.__States.layout, firstSet, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, NULL);
            }
            void DrawIndexed(VkCommandBuffer& commandBuffer, uint32_t indexSize, uint32_t instanceCount) {
                vkCmdDrawIndexed(commandBuffer, indexSize, instanceCount, 0, 0, 0);
            }
            void Draw(VkCommandBuffer& commandBuffer, uint32_t vertexCount, uint32_t instanceCount) {
                vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
            }
            void ClearColorAttachment(VkCommandBuffer& commandBuffer, const VkClearColorValue& color, const VkRect2D& rect) {
                VkClearAttachment attachment{};
                attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                attachment.colorAttachment = 0;
                attachment.clearValue.color = color;
                VkClearRect clearRect{};
                clearRect.rect = rect;
                clearRect.baseArrayLayer = 0;
                clearRect.layerCount = 1;
                vkCmdClearAttachments(commandBuffer, 1, &attachment, 1, &clearRect);
            }
        }

        FrameSyncObjects CreateSyncObjects(VulkanDevice& device, bool createSignaled) {
            FrameSyncObjects sync{};
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            if (createSignaled) {
                fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            }
            if (vkCreateSemaphore(device.logicalDevice, &semaphoreInfo, nullptr, &sync.imageAvailableSemaphore) != VK_SUCCESS ||
                vkCreateSemaphore(device.logicalDevice, &semaphoreInfo, nullptr, &sync.renderFinishedSemaphore) != VK_SUCCESS ||
                vkCreateFence(device.logicalDevice, &fenceInfo, nullptr, &sync.inFlightFence) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
            return sync;
        }
        VkSemaphore CreateBinarySemaphore(VulkanDevice& device) {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkSemaphore semaphore;
            if (vkCreateSemaphore(device.logicalDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphore!");
            }
            return semaphore;
        }
        uint32_t AcquireNextImage(VulkanDevice device, VulkanPresenter presenter, VkSemaphore& signalSemaphore) {
            uint32_t imageIndex;
            vkAcquireNextImageKHR(device.logicalDevice, presenter.swapChain, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, &imageIndex);
            return imageIndex;
        }
        VkResult TryAcquireNextImage(VulkanDevice& device, VulkanPresenter& presenter, VkSemaphore& signalSemaphore, uint32_t* imageIndex) {
            return vkAcquireNextImageKHR(device.logicalDevice, presenter.swapChain, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, imageIndex);
        }
        void GraphicsQueueSubmit(VulkanDevice& device, FrameSyncObjects& syncObjects, const std::vector<VkCommandBuffer>& commandBuffers, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores) {
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
            submitInfo.pWaitSemaphores = waitSemaphores.data();
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
            submitInfo.pCommandBuffers = commandBuffers.data();
            submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
            submitInfo.pSignalSemaphores = signalSemaphores.data();
            if (vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, syncObjects.inFlightFence) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }
        }
        VkResult PresentQueueSubmit(VulkanDevice& device, VulkanPresenter& presenter, uint32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores) {
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
            presentInfo.pWaitSemaphores = waitSemaphores.data();
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &presenter.swapChain;
            presentInfo.pImageIndices = &imageIndex;
            return vkQueuePresentKHR(device.presentQueue, &presentInfo);
        }
    }

#ifndef VULKAN_UTIL_CORE_ONLY
    void VulkanEngine::Init(const char* title, const int width, const int height) {
        InitWindow(title, width, height);
        InitVulkanCore();
        InitFramesInFlight();
        InitPipeline();

    }

    void VulkanEngine::InitWindow(const char* title, const int width, const int height) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);    
        glfwSetCursorPosCallback(window, cursorPositionCallback);        
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    }

    void VulkanEngine::InitVulkanCore(size_t descriptorCount) {
        instance = VulkanF::CreateInstance();
        glfwCreateWindowSurface(instance, window, NULL, &surface);
        vulkanDevice = VulkanF::SetupDevice(instance, surface);
        allocator = VulkanF::CreateAllocator(instance, vulkanDevice);
        presenter = VulkanF::SetupPresenter(vulkanDevice, window, surface);
        MAX_FRAMES_IN_FLIGHT=2;
        vulkanRenderPass = VulkanF::SetupRenderPass(vulkanDevice, presenter, allocator,VK_SAMPLE_COUNT_8_BIT);

        commandPool = VulkanF::CreateCommandPool(vulkanDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        descriptorPool = VulkanF::CreateDescriptorPool(vulkanDevice, {
            VulkanF::CreatePoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorCount),
            VulkanF::CreatePoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount)
        }, descriptorCount);
    }

    void VulkanEngine::InitFramesInFlight() {
        commandBuffers = VulkanF::CreateCommandBuffers(MAX_FRAMES_IN_FLIGHT, vulkanDevice, commandPool);
        syncObjects.resize(MAX_FRAMES_IN_FLIGHT);
        storageBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        storageBufferMapped.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            syncObjects[i] = VulkanF::CreateSyncObjects(vulkanDevice);
            
            
            // Sadece model matrislerini barındıran SSBO'muz
            storageBuffer[i] = VulkanF::Memory::CreateBuffer(
                allocator, 
                sizeof(GlobalStorageData),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                VMA_MEMORY_USAGE_AUTO, 
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            );
            VulkanF::Memory::MapMemory(allocator, &storageBuffer[i], &storageBufferMapped[i]);
        }

        
    }

    void VulkanEngine::InitPipeline() {
        graphicsPipeline.InitDefaultPipeline(presenter, vulkanRenderPass);
        
        graphicsPipeline.AddVertexInputState(
            { VulkanF::CreateBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX) },
            { 
                VulkanF::CreateAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
                VulkanF::CreateAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)),
                VulkanF::CreateAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)) 
            }
        );
        
        graphicsPipeline.AddShaderStage(vulkanDevice, "./Shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        graphicsPipeline.AddShaderStage(vulkanDevice, "./Shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        VulkanF::VulkanDescriptorSetLayout setLayouts{};
        setLayouts.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
        setLayouts.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        setLayouts.Build(vulkanDevice);

        graphicsPipeline.AddDescriptorSetLayouts({ setLayouts });
        
        // Push Constant sadece viewProj (mat4 = 64 bytes) içeriyor
        graphicsPipeline.AddPushConstants(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0);
        graphicsPipeline.BuildPipeline(vulkanDevice,VK_SAMPLE_COUNT_8_BIT);
    }

    std::shared_ptr<Mesh> VulkanEngine::GetOrLoadMesh(const std::string& modelPath) {
        if (meshCache.find(modelPath) != meshCache.end()) {
            return meshCache[modelPath];
        }
        
        auto newMesh = std::make_shared<Mesh>();
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        stbi_uc* pixels;
        uint32_t w, h;
        
        LoadModelGLB(modelPath.c_str(), vertices, indices, &pixels, &w, &h);
        newMesh->indexCount = static_cast<uint32_t>(indices.size());
        
        newMesh->texture = VulkanF::SetupVulkanTextureFromMemory(pixels, w, h, vulkanDevice, allocator, commandPool);
        
        newMesh->vertexBuffer = VulkanF::Memory::CreateBuffer(
            allocator, sizeof(Vertex) * vertices.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, 
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );
        void* vData;
        VulkanF::Memory::MapMemory(allocator, &newMesh->vertexBuffer, &vData);
        memcpy(vData, vertices.data(), vertices.size() * sizeof(Vertex));
        
        vmaFlushAllocation(allocator, newMesh->vertexBuffer.deviceMemory, 0, VK_WHOLE_SIZE);
        VulkanF::Memory::UnMapMemory(allocator, &newMesh->vertexBuffer);

        newMesh->indexBuffer = VulkanF::Memory::CreateBuffer(
            allocator, sizeof(uint32_t) * indices.size(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, 
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );
        void* iData;
        VulkanF::Memory::MapMemory(allocator, &newMesh->indexBuffer, &iData);
        memcpy(iData, indices.data(), indices.size() * sizeof(uint32_t));
        
        vmaFlushAllocation(allocator, newMesh->indexBuffer.deviceMemory, 0, VK_WHOLE_SIZE);
        VulkanF::Memory::UnMapMemory(allocator, &newMesh->indexBuffer);

        newMesh->descriptorSets = VulkanF::CreateDescriptorSet(1, vulkanDevice, descriptorPool, graphicsPipeline.setLayouts);

        VkDescriptorBufferInfo bufferInfo = VulkanF::CreateBufferInfo(storageBuffer[currentFrame], 0, sizeof(GlobalStorageData));
        VkDescriptorImageInfo imageInfo = VulkanF::CreateImageInfo(newMesh->texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        std::vector<VkWriteDescriptorSet> descriptorWrites{
            VulkanF::CreateWriteDescriptorSet(newMesh->descriptorSets[0], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 0, 1, &bufferInfo),
            VulkanF::CreateWriteDescriptorSet(newMesh->descriptorSets[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 0, 1, nullptr, &imageInfo)
        };
        VulkanF::UpdateDescriptorSets(vulkanDevice, descriptorWrites);
        
        meshCache[modelPath] = newMesh;
        return newMesh;
    }

    RenderObject* VulkanEngine::CreateRenderObject(const std::string& modelPath, const VulkanAbstraction::Transform& initialTransform) {
        auto newObj = std::make_unique<RenderObject>();
        newObj->mesh = GetOrLoadMesh(modelPath);
        newObj->objectId = static_cast<uint32_t>(renderables.size());
        newObj->transform=initialTransform;
        renderables.push_back(std::move(newObj));
        return renderables.back().get();
    }

    void VulkanEngine::BuildBatches() {
        batchMap.clear();
        for (const auto& obj : renderables) {
            batchMap[obj->mesh.get()].transforms.push_back(obj->GetTransformMatrix());
        }
    }

    void VulkanEngine::DrawFrame(const void* pushConstantData) {
        vkWaitForFences(vulkanDevice.logicalDevice, 1, &syncObjects[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vulkanDevice.logicalDevice, 1, &syncObjects[currentFrame].inFlightFence);

        uint32_t imageIndex = VulkanF::AcquireNextImage(vulkanDevice, presenter, syncObjects[currentFrame].imageAvailableSemaphore);

        BuildBatches();

        GlobalStorageData* storageData = static_cast<GlobalStorageData*>(storageBufferMapped[currentFrame]);
        uint32_t bufferOffset = 0;
        
        for (const auto& [mesh, batch] : batchMap) {
            for (const auto& transform : batch.transforms) {
                memcpy(&storageData->models[bufferOffset++], &transform, sizeof(glm::mat4));
            }
        }
        
        vmaFlushAllocation(allocator, storageBuffer[currentFrame].deviceMemory, 0, VK_WHOLE_SIZE);

        VkCommandBuffer cmd = commandBuffers[currentFrame];
        vkResetCommandBuffer(cmd, 0);

        VulkanF::RecordCommand::BeginCommandBuffer(cmd);
        VulkanF::RecordCommand::BeginRenderPass({ 0.04f, 0.04f, 0.04f, 1.0f }, vulkanRenderPass, presenter, cmd, imageIndex);
        VulkanF::RecordCommand::BindPipeline(graphicsPipeline, cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);
        VulkanF::RecordCommand::SetViewport(presenter, cmd);
        VulkanF::RecordCommand::SetScissor(cmd, presenter);


        if (graphicsPipeline.pushConstant.has_value() && pushConstantData != nullptr) {
            VulkanF::RecordCommand::PushConstants(
                cmd, 
                graphicsPipeline.__States.layout,
                graphicsPipeline.pushConstant->flags,
                graphicsPipeline.pushConstant->offset,
                graphicsPipeline.pushConstant->size,
                pushConstantData
            );
        }
            
        uint32_t currentInstanceOffset = 0;
        int drawCall=0;
        for (const auto& [mesh, batch] : batchMap) {
            uint32_t instanceCount = static_cast<uint32_t>(batch.transforms.size());

            VulkanF::RecordCommand::BindVertexBuffer(cmd, { mesh->vertexBuffer.buffer });
            VulkanF::RecordCommand::BindIndexBuffer(cmd, mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            VulkanF::RecordCommand::BindDescriptorSets(cmd, graphicsPipeline, { mesh->descriptorSets[0] });
            
            vkCmdDrawIndexed(cmd, mesh->indexCount, instanceCount, 0, 0, currentInstanceOffset);
            drawCall++;
            currentInstanceOffset += instanceCount;
        }

        VulkanF::RecordCommand::EndRenderPass(cmd);
        if (VulkanF::RecordCommand::EndCommandBuffer(cmd) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }

        VulkanF::GraphicsQueueSubmit(vulkanDevice, syncObjects[currentFrame],
            { cmd }, { syncObjects[currentFrame].imageAvailableSemaphore }, { syncObjects[currentFrame].renderFinishedSemaphore });
            
        VulkanF::PresentQueueSubmit(vulkanDevice, presenter, imageIndex, { syncObjects[currentFrame].renderFinishedSemaphore });

        currentFrame=(currentFrame+1)%MAX_FRAMES_IN_FLIGHT;
    }
    void VulkanEngine::cursorPositionCallback(GLFWwindow* window, double xposIn, double yposIn) {
        auto* engine = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
        engine->processMouseMovement(static_cast<float>(xposIn), static_cast<float>(yposIn));
    }
    void VulkanEngine::processMouseMovement(float xpos, float ypos) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; 

        lastX = xpos;
        lastY = ypos;

        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }
    void VulkanEngine::Cleanup() {
        vkDeviceWaitIdle(vulkanDevice.logicalDevice);


            vkDestroyBuffer(vulkanDevice.logicalDevice, storageBuffer[currentFrame].buffer, nullptr);
            vkDestroySemaphore(vulkanDevice.logicalDevice, syncObjects[currentFrame].renderFinishedSemaphore, nullptr);
            vkDestroySemaphore(vulkanDevice.logicalDevice, syncObjects[currentFrame].imageAvailableSemaphore, nullptr);
            vkDestroyFence(vulkanDevice.logicalDevice, syncObjects[currentFrame].inFlightFence, nullptr);
        

        for (auto& [path, sharedMesh] : meshCache) {
            vkDestroyBuffer(vulkanDevice.logicalDevice, sharedMesh->vertexBuffer.buffer, nullptr);
            vkDestroyBuffer(vulkanDevice.logicalDevice, sharedMesh->indexBuffer.buffer, nullptr);
        }

        vkFreeCommandBuffers(vulkanDevice.logicalDevice, commandPool, 1, &commandBuffers[currentFrame]);
        vkDestroyCommandPool(vulkanDevice.logicalDevice, commandPool, nullptr);
        vkDestroyDescriptorPool(vulkanDevice.logicalDevice, descriptorPool, nullptr);

        for (auto framebuffer : vulkanRenderPass.frameBuffers) {
            vkDestroyFramebuffer(vulkanDevice.logicalDevice, framebuffer, nullptr);
        }
        vkDestroyPipeline(vulkanDevice.logicalDevice, graphicsPipeline.pipeline, nullptr);
        vkDestroyPipelineLayout(vulkanDevice.logicalDevice, graphicsPipeline.__States.layout, nullptr);
        vkDestroyRenderPass(vulkanDevice.logicalDevice, vulkanRenderPass.renderPass, nullptr);
        
        for (auto imageView : presenter.swapChainImageViews) {
            vkDestroyImageView(vulkanDevice.logicalDevice, imageView, nullptr);
        }
        vkDestroySwapchainKHR(vulkanDevice.logicalDevice, presenter.swapChain, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }
#endif
}
#endif
