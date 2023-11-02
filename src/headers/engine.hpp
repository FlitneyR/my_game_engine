#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <libraries.hpp>

#include <chrono>

namespace mge {

class Engine {
public:
    static const char* s_engineName;
    static const uint32_t s_engineVersion;

    static const std::vector<std::string> s_requiredInstanceLayers;
    static const std::vector<std::string> s_requiredDeviceLayers;
    static const std::vector<std::string> s_requiredInstanceExtensions;
    static const std::vector<std::string> s_requiredDeviceExtensions;

    virtual constexpr int getMaxFramesInFlight() { return 2; }
    int m_currentInFlightFrame = 0;

    std::chrono::high_resolution_clock::time_point m_startTime, m_lastFrameTime;

    struct QueueFamilies {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool is_complete() { return graphicsFamily && presentFamily; }
    } m_queueFamilies;

    GLFWwindow* m_window;

    vk::Instance m_instance;
    vk::PhysicalDevice m_physicalDevice;
    vk::Device m_device;

    vk::SurfaceKHR m_surface;
    vk::SwapchainKHR m_swapchain;
    vk::SurfaceFormatKHR m_swapchainFormat;
    vk::Extent2D m_swapchainExtent;

    vk::Format m_depthImageFormat;
    vk::Image m_depthImage;
    vk::DeviceMemory m_depthImageMemory;
    vk::ImageView m_depthImageView;

    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::ImageView> m_swapchainImageViews;
    std::vector<vk::Framebuffer> m_framebuffers;

    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_commandBufferReadyFences;

    vk::RenderPass m_renderPass;
    vk::CommandPool m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;

    virtual std::string getGameName() { return "No Game"; }
    virtual uint32_t getGameVersion() { return VK_MAKE_VERSION(0, 1, 0); }

    static float randomRangeFloat(float low, float high) {
        float factor = (float)std::rand() / (float)RAND_MAX;
        return glm::lerp(low, high, factor);
    }

    static glm::vec3 randomUnitVector() {
        return glm::normalize(glm::vec3(
            randomRangeFloat(-1.f, 1.f),
            randomRangeFloat(-1.f, 1.f),
            randomRangeFloat(-1.f, 1.f)
        ));
    }

    // === setup functions ===
    void checkInstanceLayers();
    void checkInstanceExtensions();
    bool checkDeviceLayers(vk::PhysicalDevice device);
    bool checkDeviceExtensions(vk::PhysicalDevice device);

    void createInstance();
    void getSurface();
    QueueFamilies getQueueFamilies(vk::PhysicalDevice device) const;
    void getPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void rebuildSwapchain();
    void createSynchronisers();
    void createDepthBuffer();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffer();

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    // === main loop functions ===
    void draw();

    void init(uint32_t initWidth, uint32_t initHeight);
    void main();
    void cleanup();

    vk::ShaderModule loadShaderModule(std::string path);
    vk::ShaderModule compileShaderModule(std::vector<uint32_t>& code);

    virtual void start() {}
    virtual void recordDrawCommands(vk::CommandBuffer cmd) {}

    virtual void physicsUpdate(double deltaTime) {
        
    }

    virtual void update(double deltaTime) {
        
    }

    virtual void end() {}
};

}

#endif