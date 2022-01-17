//
// Created by loys on 16/1/2022.
//

#include "MainApplication.hpp"

#include <Include/GlobalConfig.hpp>

MainApplication::MainApplication()
{
    InitWindow();
    InitGraphicAPI();
}

MainApplication::~MainApplication()
{
    cleanUp();
}

void MainApplication::run()
{

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
    }
}

void MainApplication::InitGraphicAPI()
{
    // graphicAPIInfo();

    assert(m_vkValidationLayer.IsAvailable());

    auto appInfo = vk::ApplicationInfo(
        "Hello Triangle",
        VK_MAKE_VERSION(1, 0, 0),
        "No Engine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_0);

    vk::InstanceCreateInfo createInfo { {}, &appInfo,
        m_vkValidationLayer.RequiredLayerCount(), m_vkValidationLayer.RequiredLayerStrPtr(),
        m_vkExtension.ExtensionCount(), m_vkExtension.ExtensionStrPtr() };

    // Validation layer
    using SeverityFlag = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using MessageFlag = vk::DebugUtilsMessageTypeFlagBitsEXT;
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo { {},
        SeverityFlag::eError | SeverityFlag::eWarning | SeverityFlag::eInfo | SeverityFlag::eVerbose,
        MessageFlag::eGeneral | MessageFlag::ePerformance | MessageFlag::eValidation,
        ValidationLayer::debugCallback, {} };

    debugCreateInfo.messageSeverity ^= SeverityFlag::eInfo | SeverityFlag::eVerbose;

    // Validation layer for instance create & destroy
    if (m_vkExtension.isUsingValidationLayer())
    {
        createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }

    m_vkInstance = vk::createInstance(createInfo);
    m_vkDynamicDispatch.init(m_vkInstance, vkGetInstanceProcAddr);

    // Validation layer
    if (m_vkExtension.isUsingValidationLayer())
    {
        m_vkdebugMessenger = m_vkInstance.createDebugUtilsMessengerEXT(debugCreateInfo, nullptr, m_vkDynamicDispatch);
    }
}

void MainApplication::InitWindow()
{
    glfwInit();

    m_screen_width = GlobalConfig::getConfigData()["screen_width"].get<int>();
    m_screen_height = GlobalConfig::getConfigData()["screen_height"].get<int>();
    m_window_resizable = GlobalConfig::getConfigData()["window_resizable"].get<bool>();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, m_window_resizable ? GLFW_TRUE : GLFW_FALSE);
    m_window = glfwCreateWindow(m_screen_width, m_screen_height, "Vulkan window", nullptr, nullptr);
}

void MainApplication::cleanUp()
{
    if (m_vkExtension.isUsingValidationLayer())
    {
        m_vkInstance.destroyDebugUtilsMessengerEXT(m_vkdebugMessenger, {}, m_vkDynamicDispatch);
    }

    m_vkInstance.destroy();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void MainApplication::graphicAPIInfo()
{
    uint32_t extensionCount = 0;
    auto result = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    assert(result == vk::Result::eSuccess);

    Logger::getInstance().Log(extensionCount, "extensions supported:\n");

    std::vector<vk::ExtensionProperties> extensions(extensionCount);
    result = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    assert(result == vk::Result::eSuccess);

    for (auto& extension : extensions)
    {
        Logger::getInstance().Log(extension.extensionName, "\n");
    }
}
