#include "hello_triangle_application.h"

#include <chrono>
#include <stdexcept>
#include <array>
#include <map>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>

#include "log.h"
#include "filesystem.h"

#pragma warning(disable : 26812)

void HelloTriangleApplication::Init() {
    LOGD("Init");
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
    CreateSwapchainImageViews();
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreatePipelineLayout();
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateColorResources();
    CreateFramebuffers();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void HelloTriangleApplication::Release() {
    LOGD("Release");
    vkDeviceWaitIdle(device_);
    DestroySyncObjects();
    DestroyCommandBuffers();
    DestroyDescriptorPool();
    DestroyUniformBuffers();
    DestroyIndexBuffer();
    DestroyVertexBuffer();
    DestroyFramebuffers();
    DestroyColorResources();
    DestroyCommandPool();
    DestroyGraphicsPipeline();
    DestroyPipelineLayout();
    DestroyDescriptorSetLayout();
    DestroyRenderPass();
    DestroySwapchainImageViews();
    DestroySwapchain();
    DestroyLogicalDevice();
    DestroySurface();
    DestroyDebugMessenger();
    DestroyInstance();
}

void HelloTriangleApplication::OnUpdate() {
    PrintFPS();

}

void HelloTriangleApplication::OnRender() {
    if (!device_) {
        return;
    }

    vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    UpdateUniformBuffer(current_frame_);
    vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);
    vkResetCommandBuffer(command_buffers_[current_frame_], 0);
    RecordCommandBuffer(command_buffers_[current_frame_], image_index);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = { image_available_semaphores_[current_frame_] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers_[current_frame_];

    VkSemaphore signal_semaphores[] = { render_finished_semaphores_[current_frame_] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    if (vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fences_[current_frame_]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = { swapchain_ };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr; // Optional
    result = vkQueuePresentKHR(present_queue_, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized_) {
        framebuffer_resized_ = false;
        RecreateSwapchain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
}

void HelloTriangleApplication::PrintFPS() {
    static uint64_t frame_counter = 0;
    static double elapsed_seconds = 0.0;
    static std::chrono::high_resolution_clock clock{};
    static auto t0 = clock.now();

    frame_counter++;
    auto t1 = clock.now();
    auto delta_time = t1 - t0;
    t0 = t1;
    elapsed_seconds += delta_time.count() * 1e-9;
    if (elapsed_seconds > 1.0) {
        auto fps = frame_counter / elapsed_seconds;
        LOGD("FPS: %f", fps);
        frame_counter = 0;
        elapsed_seconds = 0.0;
    }
}


void HelloTriangleApplication::CreateInstance() {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    std::vector<const char*> instance_extensions{
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
    };
    if (kEnableValidationLayers) {
        instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
    create_info.ppEnabledExtensionNames = instance_extensions.data();

    std::vector<const char*> validation_layers = GetValidationLayers();
    if (kEnableValidationLayers) {
        if (!CheckValidationLayerSupport(validation_layers)) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else {
        create_info.enabledLayerCount = 0;
    }
    if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void HelloTriangleApplication::DestroyInstance() {
    if (instance_) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

bool HelloTriangleApplication::CheckValidationLayerSupport(const std::vector<const char*> validation_layers) {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (auto layer_name : validation_layers) {
        bool layer_found = false;
        for (auto& layer_properties : available_layers) {
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if (!layer_found) {
            return false;
        }
    }

    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    LOGD("validation layer: %s", callback_data->pMessage);
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debug_messenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, create_info, allocator, debug_messenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks* allocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debug_messenger, allocator);
    }
}

void HelloTriangleApplication::SetupDebugMessenger() {
    if (!kEnableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = DebugCallback;
    create_info.pUserData = nullptr; // Optional
    if (CreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr, &debug_messenger_) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void HelloTriangleApplication::DestroyDebugMessenger() {
    if (kEnableValidationLayers && debug_messenger_) {
        DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
        debug_messenger_ = VK_NULL_HANDLE;
    }
}

void HelloTriangleApplication::PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    if (device_count == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            physical_device_ = device;
            msaa_samples_ = GetMaxUsableSampleCount();
            break;
        }
    }

    if (physical_device_ == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

bool HelloTriangleApplication::IsDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = FindQueueFamilies(device);

    bool extensions_supported = CheckDeviceExtensionSupport(device);
    bool swapchain_adequate = false;
    if (extensions_supported) {
        SwapchainSupportDetails deatils =
            QuerySwapchainSupport(device);
        swapchain_adequate = !deatils.formats.empty() && !deatils.present_modes.empty();
    }

    return indices.IsComplete() && extensions_supported && swapchain_adequate;
}

QueueFamilyIndices HelloTriangleApplication::FindQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queue_family : queue_families) {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
        if (present_support) {
            indices.present_family = i;
        }

        if (indices.IsComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

void HelloTriangleApplication::CreateLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(physical_device_);

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkPhysicalDeviceFeatures device_features{};
    device_features.sampleRateShading = VK_TRUE;

    create_info.pEnabledFeatures = &device_features;
    auto device_extensions = GetDeviceExtensions();
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
    std::set<uint32_t> unique_queue_families = {
        indices.graphics_family.value(),
        indices.present_family.value(),
    };
    float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();

    std::vector<const char*> validation_layers = GetValidationLayers();
    if (kEnableValidationLayers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device_, indices.present_family.value(), 0, &present_queue_);
    vkGetDeviceQueue(device_, indices.graphics_family.value(), 0, &graphics_queue_);
    if (present_queue_ == VK_NULL_HANDLE || graphics_queue_ == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to get present or graphics queue!");
    }
}

void HelloTriangleApplication::DestroyLogicalDevice() {
    if (device_) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
}

void HelloTriangleApplication::CreateSurface() {
    VkWin32SurfaceCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.hwnd = hwnd_;
    create_info.hinstance = hinstance_;
    if (vkCreateWin32SurfaceKHR(instance_, &create_info, nullptr, &surface_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void HelloTriangleApplication::DestroySurface() {
    if (instance_ && surface_) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
}

bool HelloTriangleApplication::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    auto device_extensions = GetDeviceExtensions();
    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

SwapchainSupportDetails HelloTriangleApplication::QuerySwapchainSupport(VkPhysicalDevice device) {
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);

    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_,
            &present_mode_count, details.present_modes.data());
    }

    return details;
}

VkSurfaceFormatKHR HelloTriangleApplication::ChooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }
    return available_formats[0];
}

VkPresentModeKHR  HelloTriangleApplication::ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT_MAX) {
        return capabilities.currentExtent;
    }
    else {
        RECT client_rect = {};
        ::GetClientRect(hwnd_, &client_rect);
        int width = client_rect.right - client_rect.left;
        int height = client_rect.bottom - client_rect.top;

        VkExtent2D actual_extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actual_extent.width = std::clamp(actual_extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actual_extent;
    }
}

void HelloTriangleApplication::CreateSwapchain() {
    SwapchainSupportDetails swapchain_support = QuerySwapchainSupport(physical_device_);

    VkSurfaceFormatKHR surface_format = ChooseSwapchainSurfaceFormat(swapchain_support.formats);
    VkPresentModeKHR present_mode = ChooseSwapchainPresentMode(swapchain_support.present_modes);
    VkExtent2D extent = ChooseSwapchainExtent(swapchain_support.capabilities);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0
        && image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(physical_device_);
    if (indices.graphics_family != indices.present_family) {
        uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0; // Optional
        create_info.pQueueFamilyIndices = nullptr; // Optional
    }
    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());

    swapchain_image_format_ = surface_format.format;
    swapchain_extent_ = extent;
}

void HelloTriangleApplication::DestroySwapchain() {
    if (device_ && swapchain_) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

void HelloTriangleApplication::CreateSwapchainImageViews() {
    swapchain_image_views_.resize(swapchain_images_.size());
    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchain_images_[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain_image_format_;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device_, &create_info, nullptr, &swapchain_image_views_[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void HelloTriangleApplication::DestroySwapchainImageViews() {
    if (device_ && !swapchain_image_views_.empty()) {
        for (auto image_view : swapchain_image_views_) {
            vkDestroyImageView(device_, image_view, nullptr);
        }
        swapchain_image_views_.clear();
    }
}

void HelloTriangleApplication::CreateGraphicsPipeline() {
    auto vert_shader_code = ReadFile("vert.spv");
    auto frag_shader_code = ReadFile("frag.spv");

    VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
    VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT; vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vert_shader_stage_info,
        frag_shader_stage_info
    };

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    auto binding_description = Vertex::GetBindingDescription();
    auto attribute_descriptions = Vertex::GetAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain_extent_.width;
    viewport.height = (float)swapchain_extent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain_extent_;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.minSampleShading = .2f;
    multisampling.rasterizationSamples = msaa_samples_;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f; // Optional
    color_blending.blendConstants[1] = 0.0f; // Optional
    color_blending.blendConstants[2] = 0.0f; // Optional
    color_blending.blendConstants[3] = 0.0f; // Optional

    VkGraphicsPipelineCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.stageCount = 2;
    create_info.pStages = shader_stages;
    create_info.pVertexInputState = &vertex_input_info;
    create_info.pInputAssemblyState = &input_assembly;
    create_info.pViewportState = &viewport_state;
    create_info.pRasterizationState = &rasterizer;
    create_info.pMultisampleState = &multisampling;
    create_info.pDepthStencilState = nullptr; // Optional
    create_info.pColorBlendState = &color_blending;
    create_info.pDynamicState = &dynamic_state;
    create_info.layout = pipeline_layout_;
    create_info.renderPass = render_pass_;
    create_info.subpass = 0;
    create_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
    create_info.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &create_info, nullptr, &graphics_pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device_, vert_shader_module, nullptr);
    vkDestroyShaderModule(device_, frag_shader_module, nullptr);
}

void HelloTriangleApplication::DestroyGraphicsPipeline() {
    if (device_ && graphics_pipeline_) {
        vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
        graphics_pipeline_ = VK_NULL_HANDLE;
    }
}


VkShaderModule HelloTriangleApplication::CreateShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shader_module;
}

void  HelloTriangleApplication::CreatePipelineLayout() {
    VkPipelineLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.setLayoutCount = 1;
    create_info.pSetLayouts = &descriptor_set_layout_;
    create_info.pushConstantRangeCount = 0; // Optional
    create_info.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(device_, &create_info, nullptr,
        &pipeline_layout_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void  HelloTriangleApplication::DestroyPipelineLayout() {
    if (device_ && pipeline_layout_) {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
}

void  HelloTriangleApplication::CreateRenderPass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain_image_format_;
    color_attachment.samples = msaa_samples_;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription color_attachment_resolve{};
    color_attachment_resolve.format = swapchain_image_format_;
    color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_resolve_ref{};
    color_attachment_resolve_ref.attachment = 1;
    color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pResolveAttachments = &color_attachment_resolve_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { color_attachment, color_attachment_resolve };
    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    create_info.pAttachments = attachments.data();
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;
    create_info.dependencyCount = 1;
    create_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device_, &create_info, nullptr, &render_pass_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void  HelloTriangleApplication::DestroyRenderPass() {
    if (device_ && render_pass_) {
        vkDestroyRenderPass(device_, render_pass_, nullptr);
        render_pass_ = VK_NULL_HANDLE;
    }
}

void HelloTriangleApplication::CreateFramebuffers() {
    swapchain_framebuffers_.resize(swapchain_image_views_.size());
    for (size_t i = 0; i < swapchain_image_views_.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            color_image_view_,
            swapchain_image_views_[i]
        };

        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass_;
        create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        create_info.pAttachments = attachments.data();
        create_info.width = swapchain_extent_.width;
        create_info.height = swapchain_extent_.height;
        create_info.layers = 1;
        if (vkCreateFramebuffer(device_, &create_info, nullptr,
            &swapchain_framebuffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void HelloTriangleApplication::DestroyFramebuffers() {
    if (device_ && !swapchain_framebuffers_.empty()) {
        for (auto framebuffer : swapchain_framebuffers_) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
        swapchain_framebuffers_.clear();
    }
}


void HelloTriangleApplication::CreateCommandPool() {
    QueueFamilyIndices queue_family_indices = FindQueueFamilies(physical_device_);

    VkCommandPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

    if (vkCreateCommandPool(device_, &create_info, nullptr, &command_pool_) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void HelloTriangleApplication::DestroyCommandPool() {
    if (device_ && command_pool_) {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
    }
}

void HelloTriangleApplication::CreateCommandBuffers() {
    command_buffers_.resize(kMaxFramesInFlight);
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

    if (vkAllocateCommandBuffers(device_, &alloc_info, command_buffers_.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void HelloTriangleApplication::DestroyCommandBuffers() {
    if (device_ && !command_buffers_.empty()) {
        vkFreeCommandBuffers(device_, command_pool_, 1, command_buffers_.data());
        command_buffers_.clear();
    }
}

void HelloTriangleApplication::RecordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0; // Optional
    begin_info.pInheritanceInfo = nullptr; // Optional
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass_;
    render_pass_info.framebuffer = swapchain_framebuffers_[image_index];
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = swapchain_extent_;
    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clearColor;
    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain_extent_.width);
    viewport.height = static_cast<float>(swapchain_extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain_extent_;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer vertex_buffers[] = { vertex_buffer_ };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, index_buffer_, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, &descriptor_sets_[current_frame_], 0, nullptr);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(kIndices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}


void HelloTriangleApplication::CreateSyncObjects() {
    image_available_semaphores_.resize(kMaxFramesInFlight);
    render_finished_semaphores_.resize(kMaxFramesInFlight);
    in_flight_fences_.resize(kMaxFramesInFlight);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < kMaxFramesInFlight; i++) {
        if (vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphores_[i]) != VK_SUCCESS
            || vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphores_[i]) != VK_SUCCESS
            || vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }
}

void HelloTriangleApplication::DestroySyncObjects() {
    if (device_
        && !image_available_semaphores_.empty()
        && !render_finished_semaphores_.empty()
        && !in_flight_fences_.empty()) {
        for (size_t i = 0; i < kMaxFramesInFlight; i++) {

            vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
            vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
            vkDestroyFence(device_, in_flight_fences_[i], nullptr);
        }
        image_available_semaphores_.clear();
        render_finished_semaphores_.clear();
        in_flight_fences_.clear();
    }
}

void HelloTriangleApplication::CleanupSwapchain() {
    DestroyFramebuffers();
    DestroyColorResources();
    DestroySwapchainImageViews();
    DestroySwapchain();
}

void HelloTriangleApplication::RecreateSwapchain() {
    vkDeviceWaitIdle(device_);

    CleanupSwapchain();

    CreateSwapchain();
    CreateSwapchainImageViews();
    CreateColorResources();
    CreateFramebuffers();
}


void HelloTriangleApplication::CreateVertexBuffer() {
    VkDeviceSize buffer_size = sizeof(kVertices[0]) * kVertices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
    void* data;
    vkMapMemory(device_, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, kVertices.data(), buffer_size);
    vkUnmapMemory(device_, staging_buffer_memory);

    CreateBuffer(buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer_, vertex_buffer_memory_);

    CopyBuffer(staging_buffer, vertex_buffer_, buffer_size);

    vkDestroyBuffer(device_, staging_buffer, nullptr);
    vkFreeMemory(device_, staging_buffer_memory, nullptr);
}

void HelloTriangleApplication::DestroyVertexBuffer() {
    if (device_ && vertex_buffer_ && vertex_buffer_memory_) {
        vkDestroyBuffer(device_, vertex_buffer_, nullptr);
        vkFreeMemory(device_, vertex_buffer_memory_, nullptr);
        vertex_buffer_ = VK_NULL_HANDLE;
        vertex_buffer_memory_ = VK_NULL_HANDLE;
    }
}

uint32_t HelloTriangleApplication::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties)) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void HelloTriangleApplication::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) {
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &create_info, nullptr, &buffer)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device_, buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device_, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device_, buffer, buffer_memory, 0);
}

void HelloTriangleApplication::CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool_;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0; // Optional
    copy_region.dstOffset = 0; // Optional
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue_);
    vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);
}

void HelloTriangleApplication::CreateIndexBuffer() {
    VkDeviceSize buffer_size = sizeof(kIndices[0]) * kIndices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
    void* data;
    vkMapMemory(device_, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, kIndices.data(), buffer_size);
    vkUnmapMemory(device_, staging_buffer_memory);

    CreateBuffer(buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer_, index_buffer_memory_);

    CopyBuffer(staging_buffer, index_buffer_, buffer_size);

    vkDestroyBuffer(device_, staging_buffer, nullptr);
    vkFreeMemory(device_, staging_buffer_memory, nullptr);
}

void HelloTriangleApplication::DestroyIndexBuffer() {
    if (device_ && index_buffer_ && index_buffer_memory_) {
        vkDestroyBuffer(device_, index_buffer_, nullptr);
        vkFreeMemory(device_, index_buffer_memory_, nullptr);
        index_buffer_ = VK_NULL_HANDLE;
        index_buffer_memory_ = VK_NULL_HANDLE;
    }
}

void HelloTriangleApplication::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings = &ubo_layout_binding;
    if (vkCreateDescriptorSetLayout(device_, &create_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void HelloTriangleApplication::DestroyDescriptorSetLayout() {
    if (device_ && descriptor_set_layout_) {
        vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
        descriptor_set_layout_ = VK_NULL_HANDLE;
    }
}

void HelloTriangleApplication::CreateUniformBuffers() {
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);

    uniform_buffers_.resize(kMaxFramesInFlight);
    uniform_buffers_memory_.resize(kMaxFramesInFlight);
    uniform_buffers_mapped_.resize(kMaxFramesInFlight);

    for (size_t i = 0; i < kMaxFramesInFlight; i++) {
        CreateBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffers_[i], uniform_buffers_memory_[i]);
        vkMapMemory(device_, uniform_buffers_memory_[i], 0, buffer_size, 0, &uniform_buffers_mapped_[i]);
    }
}

void HelloTriangleApplication::DestroyUniformBuffers() {
    if (device_ && !uniform_buffers_.empty() && !uniform_buffers_memory_.empty()) {
        for (size_t i = 0; i < kMaxFramesInFlight; i++) {
            vkDestroyBuffer(device_, uniform_buffers_[i], nullptr);
            vkFreeMemory(device_, uniform_buffers_memory_[i], nullptr);
        }
    }
}


void HelloTriangleApplication::UpdateUniformBuffer(uint32_t current_image) {
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapchain_extent_.width / (float)swapchain_extent_.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    memcpy(uniform_buffers_mapped_[current_image], &ubo, sizeof(ubo));
}

void HelloTriangleApplication::CreateDescriptorPool() {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);

    VkDescriptorPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = 1;
    create_info.pPoolSizes = &pool_size;
    create_info.maxSets = static_cast<uint32_t>(kMaxFramesInFlight);
    if (vkCreateDescriptorPool(device_, &create_info, nullptr,
        &descriptor_pool_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void HelloTriangleApplication::DestroyDescriptorPool() {
    if (device_ && descriptor_pool_) {
        vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }
}


void HelloTriangleApplication::CreateDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(kMaxFramesInFlight, descriptor_set_layout_);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(kMaxFramesInFlight);
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets_.resize(kMaxFramesInFlight);
    if (vkAllocateDescriptorSets(device_, &alloc_info, descriptor_sets_.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    for (size_t i = 0; i < kMaxFramesInFlight; i++) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniform_buffers_[i];
        buffer_info.offset = 0;
        buffer_info.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_sets_[i];
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;
        descriptor_write.pImageInfo = nullptr; // Optional
        descriptor_write.pTexelBufferView = nullptr; // Optional
        vkUpdateDescriptorSets(device_, 1, &descriptor_write, 0, nullptr);
    }
}

VkSampleCountFlagBits HelloTriangleApplication::GetMaxUsableSampleCount() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device_, &properties);

    VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT) {
        return VK_SAMPLE_COUNT_2_BIT;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

void HelloTriangleApplication::CreateImage(uint32_t width,
    uint32_t height,
    uint32_t mip_levels,
    VkSampleCountFlagBits num_samples,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& image_memory) {

    VkImageCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.extent.width = width;
    create_info.extent.height = height;
    create_info.extent.depth = 1;
    create_info.mipLevels = mip_levels;
    create_info.arrayLayers = 1;
    create_info.format = format;
    create_info.tiling = tiling;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage = usage;
    create_info.samples = num_samples;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device_, &create_info, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device_, image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &alloc_info, nullptr, &image_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device_, image, image_memory, 0);
}

void HelloTriangleApplication::CreateColorResources() {
    VkFormat color_format = swapchain_image_format_;

    CreateImage(swapchain_extent_.width, swapchain_extent_.height, 1, msaa_samples_, color_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, color_image_, color_image_memory_);
    color_image_view_ = CreateImageView(color_image_, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

VkImageView HelloTriangleApplication::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.subresourceRange.aspectMask = aspect_flags;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = mip_levels;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device_, &create_info, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return image_view;
}

void HelloTriangleApplication::DestroyColorResources() {
    if (device_ && color_image_view_ && color_image_ && color_image_memory_) {
        vkDestroyImageView(device_, color_image_view_, nullptr);
        vkDestroyImage(device_, color_image_, nullptr);
        vkFreeMemory(device_, color_image_memory_, nullptr);
    }
}