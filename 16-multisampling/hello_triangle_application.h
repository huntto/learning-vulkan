#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <array>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    bool IsComplete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = 0;
        binding_description.stride = sizeof(Vertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static std::array<VkVertexInputAttributeDescription, 2>
        GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};
        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(Vertex, pos);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(Vertex, color);

        return attribute_descriptions;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class HelloTriangleApplication {
public:
    virtual void OnRender();
    virtual void OnUpdate();

    void OnWindowCreated(HINSTANCE hinstance, HWND hwnd) {
        hinstance_ = hinstance;
        hwnd_ = hwnd;
        Init();
    }

    void OnWindowDestroyed() {
        Release();
    }

    void OnWindowSizeChanged() {
        framebuffer_resized_ = true;
    }
protected:
    void PrintFPS();

    virtual void Init();
    virtual void Release();

    virtual void CreateInstance();
    virtual void DestroyInstance();
    bool CheckValidationLayerSupport(const std::vector<const char*> validation_layers);
    virtual void SetupDebugMessenger();
    virtual void DestroyDebugMessenger();
    virtual void PickPhysicalDevice();
    virtual bool IsDeviceSuitable(VkPhysicalDevice device);
    virtual QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    virtual void CreateLogicalDevice();
    virtual void DestroyLogicalDevice();
    virtual void CreateSurface();
    virtual void DestroySurface();
    virtual bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

    virtual const std::vector<const char*> GetDeviceExtensions() {
        return std::vector<const char*>{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
    }

    virtual const std::vector<const char*> GetValidationLayers() {
        return std::vector<const char*> {
            "VK_LAYER_KHRONOS_validation",
        };
    }

    virtual SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
    virtual VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
    virtual VkPresentModeKHR ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
    virtual VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    virtual void CreateSwapchain();
    virtual void DestroySwapchain();
    virtual void CreateSwapchainImageViews();
    virtual void DestroySwapchainImageViews();
    virtual VkShaderModule CreateShaderModule(const std::vector<char>& code);
    virtual void CreatePipelineLayout();
    virtual void DestroyPipelineLayout();
    virtual void CreateRenderPass();
    virtual void DestroyRenderPass();
    virtual void CreateGraphicsPipeline();
    virtual void DestroyGraphicsPipeline();
    virtual void CreateFramebuffers();
    virtual void DestroyFramebuffers();
    virtual void CreateCommandPool();
    virtual void DestroyCommandPool();
    virtual void CreateCommandBuffers();
    virtual void DestroyCommandBuffers();
    virtual void RecordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);
    virtual void CreateSyncObjects();
    virtual void DestroySyncObjects();
    virtual void RecreateSwapchain();
    virtual void CleanupSwapchain();
    virtual void CreateVertexBuffer();
    virtual void DestroyVertexBuffer();
    virtual uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    virtual void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
    virtual void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
    virtual void CreateIndexBuffer();
    virtual void DestroyIndexBuffer();
    virtual void CreateDescriptorSetLayout();
    virtual void DestroyDescriptorSetLayout();
    virtual void CreateUniformBuffers();
    virtual void DestroyUniformBuffers();
    virtual void UpdateUniformBuffer(uint32_t current_image);
    virtual void CreateDescriptorPool();
    virtual void DestroyDescriptorPool();
    virtual void CreateDescriptorSets();
    virtual VkSampleCountFlagBits GetMaxUsableSampleCount();
    virtual void CreateImage(uint32_t width,
        uint32_t height,
        uint32_t mip_levels,
        VkSampleCountFlagBits num_samples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& image_memory);
    virtual void CreateColorResources();
    virtual VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);
    virtual void DestroyColorResources();

#ifdef NDEBUG
    const bool kEnableValidationLayers = false;
#else
    const bool kEnableValidationLayers = true;
#endif

    const int kMaxFramesInFlight = 2;

    HINSTANCE hinstance_;
    HWND hwnd_;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;

    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkSampleCountFlagBits msaa_samples_ = VK_SAMPLE_COUNT_1_BIT;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images_{};
    VkFormat swapchain_image_format_;
    VkExtent2D swapchain_extent_;
    std::vector<VkImageView> swapchain_image_views_{};
    std::vector<VkFramebuffer> swapchain_framebuffers_{};

    VkImage color_image_ = VK_NULL_HANDLE;
    VkDeviceMemory color_image_memory_ = VK_NULL_HANDLE;
    VkImageView color_image_view_ = VK_NULL_HANDLE;

    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets_{};
    VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;

    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers_{};

    std::vector < VkSemaphore> image_available_semaphores_{};
    std::vector < VkSemaphore> render_finished_semaphores_{};
    std::vector < VkFence> in_flight_fences_{};
    uint32_t current_frame_ = 0;

    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
    VkBuffer index_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory index_buffer_memory_ = VK_NULL_HANDLE;
    std::vector<VkBuffer> uniform_buffers_{};
    std::vector<VkDeviceMemory> uniform_buffers_memory_{};
    std::vector<void*> uniform_buffers_mapped_{};

    bool framebuffer_resized_ = false;

    const std::vector<Vertex> kVertices = {
        {{ -0.5f, -0.5f }, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f }, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, 0.5f }, {0.0f, 0.0f, 1.0f}},
        {{ -0.5f, 0.5f }, {1.0f, 1.0f, 1.0f}}
    };

    const std::vector<uint16_t> kIndices = {
        0, 1, 2, 2, 3, 0
    };

};