//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_UTILITY_VULKAN_VALIDATIONLAYER_HPP
#define MINECRAFT_VK_UTILITY_VULKAN_VALIDATIONLAYER_HPP

#include <Vulkan/GraphicAPI.hpp>

#include <vector>

class ValidationLayer
{
private:
    std::vector<std::string> m_requiredLayer;
    std::vector<const char*> m_requiredLayerCStr;
    std::vector<vk::LayerProperties> m_availableLayers;

    void LoadRequiredLayers();
    void LoadLayers();

    bool m_isAvailable = true;

public:
    ValidationLayer();

    [[nodiscard]] inline bool
    IsAvailable() const
    {
        return m_isAvailable;
    }

    inline uint32_t
    RequiredLayerCount()
    {
        return m_requiredLayer.size();
    }

    inline const char**
    RequiredLayerStrPtr()
    {
        m_requiredLayerCStr.clear();

        std::ranges::transform(
            m_requiredLayer, std::back_inserter(m_requiredLayerCStr),
            [](const std::string& str) { return str.c_str(); });

        return m_requiredLayerCStr.data();
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    static void glfwErrorCallback(int code, const char* description);
};

#endif // MINECRAFT_VK_UTILITY_VULKAN_VALIDATIONLAYER_HPP
