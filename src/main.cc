#if defined(USE_VULKAN_RAII) && (USE_VULKAN_RAII == 1)
#include <vulkan/vulkan_raii.hpp>
#else
#include <vulkan/vulkan.hpp>
#endif

#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC) && (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)

    VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
    #define INIT_DISPATCH() VULKAN_HPP_DEFAULT_DISPATCHER.init()
    #define DEBUGUTILS_DISPATCH(instance_) VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_)
    #define DEVICE_DISPATCH(device_) VULKAN_HPP_DEFAULT_DISPATCHER.init(device_)
    
#else

    #define INIT_DISPATCH() ((void)0)
    #define DEBUGUTILS_DISPATCH(instance_) ((void)0)
    #define DEVICE_DISPATCH(device_) ((void)0)

#endif

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
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

        try {

            initWindow();
            initVulkan();
            mainLoop();

        } catch(const std::exception& e) {
        
            cleanup();
            throw;
        
        }

        cleanup();

    }

 private:

   /* * * * * * * * * * * * * * * *
    *          VARIABLES          *
    * * * * * * * * * * * * * * * */

    // --- GlFW ---
    GLFWwindow* window = nullptr;

    // --- Vulkan ---
    vk::Instance instance = nullptr;
    vk::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::PhysicalDevice physicalDevice = nullptr;
    vk::Device device = nullptr;
    vk::Queue graphicsQueue = nullptr;
    vk::PhysicalDeviceFeatures deviceFeatures = {};
    vk::SurfaceKHR surface = nullptr;
    std::vector<const char*> requiredDeviceExtension = {vk::KHRSwapchainExtensionName};

   /* * * * * * * * * * * * * * * *
    *          FUNCTIONS          *
    * * * * * * * * * * * * * * * */

    /* First Steps setup functions
     *
     * - 1. Initializes GLFW and creates a window 
     * 
     * - 2. Create Instances
     * -- Creates appinfo then get all required layers and extensions
     * -- The procces of getting required layers and extensions is done in the validationLayerCheck function
     * -- Creates a Instace info, that stores all aplications info, layers and extensions, then create the instance
     *
     * - 3. Create Surface
     * -- Define a new appp info to win32 khr context
     * -- Create the surface with the new app info and instance
     * 
     * - 4. Setup Debug messages
     *          
     * - 5. Create Physical Device
     * -- Enumarate all physical devices detected by the system
     * -- Get the best device that supports graphics and presentation
     * --- This process evals the device properties and features
     * -- Check if the best device suports API version 1.3
     * -- Check if the best device suports the queue family
     * -- Check if the best device suports the required extensions
     * -- Check if the best device suports the required features
     * 
     * - 6. Create Logical Device
     * -- Get graphics the queue family properties
     * -- Get the first index into queueFamilyProperties (graphics queue)
     * -- Create a Chain of structures to query for Vulkan 1.3 features
     * --- PhysicalDeviceFeatures2(empty), PghysicalDeviceVulkan13Features(enable dynamic rendering), PhysicalDeviceExtendedDynamicStateFeaturesEXT(enable extended dynamic state from extension)
     * -- Create the queuee info
     * -- Create the device info
     * -- Create the Device
     * -- Create the graphics queue for the device
     * 
     */
    
    // - 1. Initializes GLFW and creates a window
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

    // - 2. Create Instances
    void validationLayerCheck(std::vector<const char*>& requiredLayers) {

        std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();
        std::vector<const char*>::iterator unsupportedLayer = std::ranges::find_if(
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

    void requiredExtensionCheck(std::vector<const char*>& requiredExtensions) {
        
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

    // - 3. Create Surface
    void createSurface() {
        
        vk::Win32SurfaceCreateInfoKHR createInfo{
            .hinstance = GetModuleHandle(nullptr),
            .hwnd      = glfwGetWin32Window(window)
        };

        vk::Result result = instance.createWin32SurfaceKHR(&createInfo, nullptr, &surface);
        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create window surface!");
        }

    }

    // - 4. Setu Debug messages
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

    // - 5. Select Physical Device
    bool isDeviceSuitable(const vk::PhysicalDevice& physicalDevice) {
        
        // Check if the physicalDevice supports Vulkan 1.3 API Version
        bool suportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

        // Check if the physicalDevice supports graphics operations
        std:: vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
        bool supportsGraphicsQueue = std::ranges::any_of(
            queueFamilies,
            [](vk::QueueFamilyProperties const &queueFamilyProperty) {
                return !!(queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics);
            }
        );

        // Check if all required physicalDevice extensions are available
        std::vector<vk::ExtensionProperties> availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions = std::ranges::all_of(
            requiredDeviceExtension,
            [&availableDeviceExtensions](const char* & requiredDeviceExtension) {
                return std::ranges::any_of(
                    availableDeviceExtensions,
                    [requiredDeviceExtension](const vk::ExtensionProperties& availableDeviceExtension) {
                        return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
                    }
                );
            }
        );

        // Check if the physicalDevice supports the required features
        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan13Features, 
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
            features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
                                                            vk::PhysicalDeviceVulkan13Features,
                                                            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool suppportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                         features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;      

        // Return true if the physicalDevice meets all the criteria
        return suportsVulkan1_3 && supportsGraphicsQueue && supportsAllRequiredExtensions && suppportsRequiredFeatures;

    }

    vk::PhysicalDevice getBetterDevice(const std::vector<vk::PhysicalDevice>& physicalDevices) {

        // --- Order map to sort candidates by score ---
        std::multimap<int, vk::PhysicalDevice> candidates;

        for(const vk::PhysicalDevice& pd : physicalDevices) {

            vk::PhysicalDeviceProperties deviceProperties = pd.getProperties();
            vk::PhysicalDeviceFeatures deviceFeatures = pd.getFeatures();
            uint32_t score = 0;

            // --- Increase score of dedicated GPU's ---
            if(deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                score += 1000;
            }

            // --- Increase score based on max size of textures ---
            score += deviceProperties.limits.maxImageDimension2D;

            // --- Application can't function without geometry shader ---
            if(!deviceFeatures.geometryShader) {
                continue;
            }

            candidates.insert(std::make_pair(score, pd));

        }

        // --- Return the best candidate ---
        /* First = x value of map, second = y value of map                              *
         * so first = score, second = pd                                            *
         * so if candidates.rbegin() where rbegin() returns the first group of the map  *
         * and check if the first element (witch is the score) is greater than 0        *
         * then return the second element (which is the pd) of the map              */
        if(candidates.empty()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        } else if(candidates.rbegin()->first > 0) {

            if(isDeviceSuitable(candidates.rbegin()->second)) {
                return candidates.rbegin()->second;
            } else {
                throw std::runtime_error("failed to find a suitable GPU!");
            }

        }

        throw std::runtime_error("candidates are not empty! but still failed to find a suitable GPU! (score <= 0)");

    }

    void pickPhysicalDevice() {

        std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
        if(physicalDevices.empty()) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        physicalDevice = getBetterDevice(physicalDevices);

    }

    // - 6. Create Logical Device
    uint32_t findQueueFamilies(const vk::PhysicalDevice& physicalDevice) {

        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        std::vector<vk::QueueFamilyProperties>::iterator graphicsQueueFamilyProperty = std::find_if(
            queueFamilyProperties.begin(),
            queueFamilyProperties.end(),
            [](const vk::QueueFamilyProperties& queueFamilyProperty) {
                return queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics;
            }
        );

        return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
        
    }

    void createLogicalDevice() {

        // --- Get graphics queue family index ---
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        // --- Get the first index into queueFamilyProperties wich supports graphics ---
        std::vector<vk::QueueFamilyProperties>::iterator graphicsQueueFamilyProperty = std::ranges::find_if(
            queueFamilyProperties, [](vk::QueueFamilyProperties const &queueFamilyProperty) {
                return (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlagBits>(0);
            }
        );
        assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() && "No graphics queue family found!");
        uint32_t graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
        
        // --- Query for Vulkan 1.3 features ---
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {},                             // vk::PhysicalDeviceFeatures2
            {.dynamicRendering = true},     // Enable dynamic rendering
            {.extendedDynamicState = true}  // Enable extended dynamic state from extension
        };

        // --- Create Device ---
        float queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
            .queueFamilyIndex = graphicsIndex,
            .queueCount       = 1,
            .pQueuePriorities = &queuePriority
        };
        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &deviceQueueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
            .ppEnabledExtensionNames = requiredDeviceExtension.data()
        };

        device = physicalDevice.createDevice(deviceCreateInfo);
        graphicsQueue = device.getQueue(graphicsIndex, 0);

    }

    void initVulkan() {

        INIT_DISPATCH();
        createInstance();
        DEBUGUTILS_DISPATCH(instance);
        setupDebugMessenger();
        pickPhysicalDevice();
        createLogicalDevice();

    }

    void mainLoop() {

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }

    }

    // --- Cleanup Function ---
    void cleanup() {

        if(window != nullptr) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();

        if(debugMessenger != vk::DebugUtilsMessengerEXT{} &&
            instance != vk::Instance{}) {

            instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr);
            debugMessenger = vk::DebugUtilsMessengerEXT{};

        }

        // Only reset physicalDevice handle if it has been created
        if(physicalDevice != vk::PhysicalDevice{}) {
            physicalDevice = vk::PhysicalDevice{};
        }

        if(instance != vk::Instance{}) {

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
