#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    bool IsComplete() {
        return graphics_family.has_value();
    }
};

class HelloTriangleApplication {
public:
    virtual void OnInit();
    virtual void OnRelease();
    virtual void OnRender();
    virtual void OnUpdate();
protected:
    void PrintFPS();

    virtual void CreateInstance();
    virtual void DestroyInstance();
    bool CheckValidationLayerSupport(const std::vector<const char*> validation_layers);
    virtual void SetupDebugMessenger();
    virtual void DestroyDebugMessenger();
    virtual void PickPhysicalDevice();
    virtual bool IsDeviceSuitable(VkPhysicalDevice device);
    virtual QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

#ifdef NDEBUG
    const bool kEnableValidationLayers = false;
#else
    const bool kEnableValidationLayers = true;
#endif


    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
};