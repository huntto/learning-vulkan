#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vector>

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

#ifdef NDEBUG
    const bool kEnableValidationLayers = false;
#else
    const bool kEnableValidationLayers = true;
#endif


    VkInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
};