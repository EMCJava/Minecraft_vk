#pragma once

#include <Include/GraphicAPI.hpp>

#include <unordered_map>

class QueueFamilyManager
{
    vk::PhysicalDevice m_vkPhysicalDevice;

    std::unordered_map<vk::QueueFlagBits, std::vector<uint32_t>> m_queueResources;
    std::vector<uint32_t> m_queueCounts;

public:
    explicit QueueFamilyManager( vk::PhysicalDevice& physicalDevice )
    {
        SetupDevice( physicalDevice );
    };

    void SetupDevice( vk::PhysicalDevice& physicalDevice );

    std::vector<std::pair<uint32_t, uint32_t>> GetQueue( const std::vector<vk::QueueFlagBits>& queue );
};