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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// ----- GLOBAL WINDOW DIMENSIONS -----
const int WIDTH  = 800;
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
    vk::Instance               instance                = nullptr;
    vk::DebugUtilsMessengerEXT debugMessenger          = nullptr;
    vk::SurfaceKHR             surface                 = nullptr;
    vk::PhysicalDevice         physicalDevice          = nullptr;
    vk::Device                 device                  = nullptr;
    vk::Queue                  queue                   = nullptr;
    vk::SwapchainKHR           swapChain               = nullptr;
    vk::SurfaceFormatKHR       swapChainSurfaceFormat;
    vk::Extent2D               swapChainExtent;
    std::vector<vk::Image>     swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    std::vector<const char*>   requiredDeviceExtension = {vk::KHRSwapchainExtensionName};

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
     * -- GLFW only deals with Vulkan C API, we're using C Struct to create the surface
     * 
     * - 4. Setup Debug messages
     *          
     * - 5. Create Physical Device
     * -- Enumarate all physical devices detected by the system
     * -- Check the suitability of the physical device
     * --- Check if the device supports Vulkan 1.3 api
     * --- Check if the device supports graphics operations + presentation
     * --- Check if all required extensions are available
     * --- Check if the device supports required features
     * --- Check if the device supports at leats the efifo presentation queue
     * --- Check if the device supports any surfaceKHR format
     * -- Get the best device that supports graphics and presentation
     * --- This process evals the device properties and features
     * 
     * - 6. Create Logical Device
     * -- Get graphics the queue family properties
     * -- Get the first index into queueFamilyProperties (graphics queue and presentation queue)
     * -- Create a Chain of structures to query for Vulkan 1.3 features
     * --- PhysicalDeviceFeatures2(empty), PghysicalDeviceVulkan13Features(enable dynamic rendering), PhysicalDeviceExtendedDynamicStateFeaturesEXT(enable extended dynamic state from extension)
     * -- Create the queuee info
     * -- Create the device info
     * -- Create the Device
     * -- Create the graphics queue for the device
     * 
     * - 7. Create Swapchain
     * -- The process of creating a swapchain involves a lot of steps and configurations,
     *    we have to check all available supports and extensions, then create and detail
     *    all queue and queue setings. (imagine swapchain as a "structed array of framebuffers")
     * -- Check for swapchain support
     * -- Load the swapchain extent(resolution) and minimum image count
     * -- Load the swapchain surface format (colorchannel and color space)
     * -- Load the swapchain present mode (v-sync like, triple buffering, etc)
     * -- Create the swapchaininfo then load into the device
     * -- Create the swapchain images
     * 
     *  - 8. Create ImageViews
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
    void validationLayerCheck(std::vector<const char*> &requiredLayers) {

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

    void requiredExtensionCheck(std::vector<const char*> &requiredExtensions) {
        
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

        VkSurfaceKHR _surface;
        if(glfwCreateWindowSurface(instance, window, nullptr, &_surface) != 0) {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::SurfaceKHR{_surface};

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
    bool isDeviceSuitable(const vk::PhysicalDevice &physicalDevice) {
        
        // Check if the physicalDevice supports Vulkan 1.3 API Version
        bool suportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

        // Check if the physicalDevice supports graphics operations + presentation
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        bool supportsGraphicsQueue = false;
        for(uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {

            if((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
               physicalDevice.getSurfaceSupportKHR(qfpIndex, surface)) {

                supportsGraphicsQueue = true;

            }

        }

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

        // Check if the physicalDevice supports at leat the required present mode
        std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);
        bool supportPresentMode = std::ranges::any_of(
            availablePresentModes,
            [](const vk::PresentModeKHR presentMode) {
                return presentMode == vk::PresentModeKHR::eFifo;
            }   
        );

        // Check if the physicalDevice supports any supported format
        std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(surface);
        bool supportFormat = false;
        if(availableFormats.size() > 0) {
            supportFormat = true;
        }

        // Return true if the physicalDevice meets all the criteria
        return suportsVulkan1_3 && supportsGraphicsQueue && supportsAllRequiredExtensions && suppportsRequiredFeatures && supportPresentMode && supportFormat;

    }

    vk::PhysicalDevice getBetterDevice(const std::vector<vk::PhysicalDevice> &physicalDevices) {

        // --- Order map to sort candidates by score ---
        std::multimap<int, vk::PhysicalDevice> candidates;

        for(const vk::PhysicalDevice& pd : physicalDevices) {

            if(!isDeviceSuitable(pd)) {
                std::cout << "Device not suitable: " << pd.getProperties().deviceName << std::endl;
                continue;
            }

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

            // Check if the physicalDevice supports 8BitsRGB format and SRGB color space to the window surface
            std::vector<vk::SurfaceFormatKHR> availableFormats = pd.getSurfaceFormatsKHR(surface);
            std::ranges::any_of(
                availableFormats,
                [](const vk::SurfaceFormatKHR &format) {
                    return format.format     == vk::Format::eB8G8R8A8Srgb      &&
                           format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                }
            ) ? score += 1000 : score += 0;

            // Check if the physicalDevice supports eMailbox presentation to the window surface
            std::vector<vk::PresentModeKHR> availablePresentModes = pd.getSurfacePresentModesKHR(surface);
            std::ranges::any_of(
                availablePresentModes,
                [](const vk::PresentModeKHR value) {
                    return value == vk::PresentModeKHR::eMailbox;
                }
            ) ? score += 1000 : score += 0;

            // --

            candidates.insert(std::make_pair(score, pd));

        }

        // --- Return the best candidate ---
        /* First = x value of map, second = y value of map                              *
         * so first = score, second = pd                                                *
         * so if candidates.rbegin() where rbegin() returns the first group of the map  *
         * and check if the first element (witch is the score) is greater than 0        *
         * then return the second element (which is the pd) of the map                  */
        if(candidates.empty()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        } else if(candidates.rbegin()->first > 0) {
            return candidates.rbegin()->second;
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
    void createLogicalDevice() {

        // --- Get graphics queue family index ---
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        // --- Get the first index into queueFamilyProperties wich supports graphics and presentation ---
        uint32_t queueIndex = ~0;
        for(uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {

            if((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
               physicalDevice.getSurfaceSupportKHR(qfpIndex, surface)) {

                queueIndex = qfpIndex;
                break;

            }

        }

        // --- Query for Vulkan 1.3 features ---
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {},                             // vk::PhysicalDeviceFeatures2
            {.dynamicRendering     = true},     // Enable dynamic rendering
            {.extendedDynamicState = true}  // Enable extended dynamic state from extension
        };

        // --- Create Device ---
        float queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
            .queueFamilyIndex = queueIndex,
            .queueCount       = 1,
            .pQueuePriorities = &queuePriority
        };
        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount    = 1,
            .pQueueCreateInfos       = &deviceQueueCreateInfo,
            .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtension.size()),
            .ppEnabledExtensionNames = requiredDeviceExtension.data()
        };

        device = physicalDevice.createDevice(deviceCreateInfo);
        queue  = device.getQueue(queueIndex, 0);

    }

    // - 7. Create Swap Chain
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
            
        const std::vector<vk::SurfaceFormatKHR>::const_iterator formatIt = std::ranges::find_if(
            availableFormats,
            [](const vk::SurfaceFormatKHR &format) {
                return format.format     == vk::Format::eB8G8R8A8Srgb      &&
                       format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            }
        );

        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];

    }

    /* IMPORTANT POINTS ABOUT PRESENTENTION MODE

       There is 4 possible modes of presentation avaible in Vulkan:
       
       - vk::PresentModeKHR::eImmediate: Images submitted by your app are transferred to the
                                         screen immediately, it may resulting in tearing.
       
       - vk::PresentModeKHR::eFifo: Display the first image from the queue and inserts
                                    the second image at the end of the queue. If the
                                    queue is full, the programs has to wait (similar to v-sync)

       - vk::PresentModeKHR::eFifoRelaxed: Like eFifo, but if the application misses a refresh, the
                                           next image is displayed anyway. This may result in tearing

       - vk::PresentModeKHR::MailBox: Known as triple buffering, reuses the eFifo ideia, but instead
                                      of waiting if the queue if full, it will replace the oldest image
                                      with the new one.                                                     */
    vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> &availablePresentModes) {

        return std::ranges::any_of(
            availablePresentModes,
            [](const vk::PresentModeKHR value) {
                return value == vk::PresentModeKHR::eMailbox;
            }
        ) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;



    }

    uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities) {
        
        unsigned int minImageCount = std::max(3u, surfaceCapabilities.minImageCount);

        if((0 < surfaceCapabilities.maxImageCount) && (minImageCount > surfaceCapabilities.maxImageCount)) {
            minImageCount = surfaceCapabilities.maxImageCount;
        }

        return minImageCount;

    }

    // --- Resolution of the swapchain ---
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities) {
        
        if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return{
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };

    }

    void createSwapChain() {
        
        vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        swapChainExtent                                = chooseSwapExtent(surfaceCapabilities);
        uint32_t minImageCount                         = chooseSwapMinImageCount(surfaceCapabilities);

        std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(surface);
        swapChainSurfaceFormat                             = chooseSwapSurfaceFormat(availableFormats);

        std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);
        vk::PresentModeKHR presentMode                        = chooseSwapPresentMode(availablePresentModes);

        vk::SwapchainCreateInfoKHR swapChainCreateInfo = {
            .surface          = surface,
            .minImageCount    = minImageCount,   
            .imageFormat      = swapChainSurfaceFormat.format,
            .imageColorSpace  = swapChainSurfaceFormat.colorSpace,
            .imageExtent      = swapChainExtent,
            .imageArrayLayers = 1, // Number of layers each image consist. Aways 1, unless you want a 3D stereoscopic
            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform     = surfaceCapabilities.currentTransform,
            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode      = presentMode,
            .clipped          = true
        };

        swapChain = device.createSwapchainKHR(swapChainCreateInfo);
        swapChainImages = device.getSwapchainImagesKHR(swapChain);

    }

    // - 8. Create Image Views
    void createImageViews() {

        assert(swapChainImages.empty());

        vk::ImageViewCreateInfo imageViewCreateInfo = {
            .viewType         = vk::ImageViewType::e2D,
            .format           = swapChainSurfaceFormat.format,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits ::eColor, 
                .levelCount = 1,
                .layerCount = 1
            }
        };
            
        for(vk::Image &image : swapChainImages) {

            imageViewCreateInfo.image = image;
            swapChainImageViews.emplace_back(device, imageViewCreateInfo);

        }

    }

    // - 9. Graphics Pipeline
    void createGraphicsPipeline() {

        

    }

    void initVulkan() {

        INIT_DISPATCH();
        createInstance();
        DEBUGUTILS_DISPATCH(instance);
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();

    }

    void mainLoop() {

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }

    }

    // --- Cleanup Function ---
    void cleanup() {

        for(vk::ImageView &imageView : swapChainImageViews) {
            device.destroyImageView(imageView, nullptr);
        }

        if(swapChain != vk::SwapchainKHR{}) {

            device.destroySwapchainKHR(swapChain, nullptr);
            swapChain = vk::SwapchainKHR{};

        }

        if(device != vk::Device{}) {
        
            device.destroy(nullptr);
            device = vk::Device{};
        
        }

        if(physicalDevice != vk::PhysicalDevice{}) {
            physicalDevice = vk::PhysicalDevice{};
        }

        if(surface != vk::SurfaceKHR{} &&
            instance != vk::Instance{}) {

            instance.destroySurfaceKHR(surface, nullptr);
            surface = vk::SurfaceKHR{};

        }

        if(debugMessenger != vk::DebugUtilsMessengerEXT{} &&
           instance != vk::Instance{}) {

            instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr);
            debugMessenger = vk::DebugUtilsMessengerEXT{};

        }

        if(instance != vk::Instance{}) {

            instance.destroy(nullptr);
            instance = vk::Instance{};

        }

        if(window != nullptr) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();

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
