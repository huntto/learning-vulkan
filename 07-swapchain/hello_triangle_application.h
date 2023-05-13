#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    bool IsComplete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

class HelloTriangleApplication {
public:
    virtual void OnInit();
    virtual void OnRelease();
    virtual void OnRender();
    virtual void OnUpdate();

    void SetHINSTANCE(HINSTANCE hinstance) {
        hinstance_ = hinstance;
    }

    void SetHWND(HWND hwnd) {
        hwnd_ = hwnd;
    }
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
    virtual void CreateLogicalDevice();
    virtual void DestroyLogicalDevice();
    virtual void CreateSurface();
    virtual void DestroySurface();

#ifdef NDEBUG
    const bool kEnableValidationLayers = false;
#else
    const bool kEnableValidationLayers = true;
#endif

    HINSTANCE hinstance_;
    HWND hwnd_;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
};