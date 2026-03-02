#if defined(USE_VULKAN_RAII) && (USE_VULKAN_RAII == 1)
#include <vulkan/vulkan_raii.hpp>
#else
#include <vulkan/vulkan.hpp>
#endif

#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC) && (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#define INIT_DISPATCH() VULKAN_HPP_DEFAULT_DISPATCHER.init()
#define INSTANCE_DISPATCH(instance_) VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_)
#else
#define INIT_DISPATCH() ((void)0)
#define INSTANCE_DISPATCH(instance_) ((void)0)
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// ----- GLOBAL WINDOW DIMENSIONS -----
const int WIDTH = 800;
const int HEIGHT = 600;

// ----- VALIDATION LAYERS -----
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif
// ----- END VALIDATION LAYERS -----

class HelloTriangleApplication {
    
 public:

    void run() {

        initWindow();
        initVulkan();
        mainLoop();
        cleanup();

    }

 private:

   /* * * * * * * * * * * * * * * *
    *          VARIABLES          *
    * * * * * * * * * * * * * * * */

    GLFWwindow* window = nullptr;
    vk::Instance instance{};
    vk::DebugUtilsMessengerEXT debugMessenger{};
    vk::PhysicalDevice physicalDevice{};

   /* * * * * * * * * * * * * * * *
    *          FUNCTIONS          *
    * * * * * * * * * * * * * * * */

    void initWindow() {
        
        if(glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("Failed to initialize GLFW.");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        if(window == nullptr) {
            throw std::runtime_error("Failed to create GLFW window.");
        }

    }

    void validationLayerCheck(std::vector<const char*> requiredLayers) {

        std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();
        auto unsupportedLayer = std::ranges::find_if(
            requiredLayers,
            [&layerProperties] (const char* requiredLayer) {
                return std::ranges::none_of(
                    layerProperties,
                    [requiredLayer] (const vk::LayerProperties &layerProperty) {
                        return strcmp(layerProperty.layerName, requiredLayer) == 0;
                    }
                );
            }
        );
        
        if(unsupportedLayer != requiredLayers.end()) {
            throw std::runtime_error(std::string("Validation layer requested, but not available: ") + *unsupportedLayer);
        }

    }

    void requiredExtensionCheck(std::vector<const char*> requiredExtensions) {
        
        std::vector<vk::ExtensionProperties> extensionProperties = vk::enumerateInstanceExtensionProperties();
        std::vector<const char*>::iterator unsupportedExtension = std::ranges::find_if(
            requiredExtensions,
            [&extensionProperties] (const char* requiredExtension) {
                return std::ranges::none_of(
                    extensionProperties,
                    [requiredExtension] (const vk::ExtensionProperties &extensionProperty) {
                        return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
                    }
                );
            }
        );
        
        if(unsupportedExtension != requiredExtensions.end()) {
            throw std::runtime_error(std::string("Required extension not available: ") + *unsupportedExtension);
        }

    }

    std::vector<const char*> getRequiredLayers() {

        std::vector<const char*> requiredLayers;
        if(enableValidationLayers) {
            requiredLayers.assign(
                validationLayers.begin(),
                validationLayers.end()  
            );
        }

        return requiredLayers;

    }

    std::vector<const char*> getRequiredInstanceExtensions() {

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (!glfwExtensions) {
            throw std::runtime_error("GLFW did not return required Vulkan extensions.");
        }
        
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if(enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;

    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {

        if( messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || 
            messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ) {

            std::cerr
                << "validation layer: type " << vk::to_string(messageType)
                << " msg: " << pCallbackData->pMessage <<
            std::endl;
        
        }

        return VK_FALSE;

    }

    void setupDebugMessenger() {
     
        if (!enableValidationLayers) return;

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        );

        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
        );

        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags,
            .messageType     = messageTypeFlags,
            .pfnUserCallback = &debugCallback,
            .pUserData       = nullptr
        };

        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);

    }

    void createInstance() {

        // --- Create appinfo ---
        constexpr vk::ApplicationInfo appInfo{
            .pApplicationName   = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = vk::ApiVersion14
        };

        // --- Get required layers ---  
        std::vector<const char*> requiredLayers = getRequiredLayers();
        validationLayerCheck(requiredLayers);

        // --- Get extensions required by GLFW ---
        std::vector<const char*> extensions = getRequiredInstanceExtensions();
        requiredExtensionCheck(extensions);

        // --- Create Instance Info ---
        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo        = &appInfo,
            .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames     = requiredLayers.empty() ? nullptr : requiredLayers.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };


        // --- Create Vulkan Instance ---
        vk::Result result = vk::createInstance(
            &createInfo, 
            nullptr, 
            &instance
        );

        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create instance!");
        }

    }

    void pickPhysicalDevice() {

        std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    }

    void initVulkan() {

        INIT_DISPATCH();
        createInstance();
        INSTANCE_DISPATCH(instance);
        setupDebugMessenger();

    }

    void mainLoop() {

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }

    }

    void cleanup() {

        if(window != nullptr) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();

        if (debugMessenger != vk::DebugUtilsMessengerEXT{} &&
            instance != vk::Instance{}) {
            instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr);
            debugMessenger = vk::DebugUtilsMessengerEXT{};
        }

        if (instance != vk::Instance{}) {
            instance.destroy(nullptr);
            instance = vk::Instance{};
        }

    }

};

int main() {


    try {

        HelloTriangleApplication app;
        app.run();
    
    } catch(const std::exception& e) {

        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;

    }

    return EXIT_SUCCESS;

}
