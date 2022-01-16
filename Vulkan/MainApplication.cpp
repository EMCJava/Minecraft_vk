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

void
MainApplication::run()
{

	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
	}
}

void
MainApplication::InitGraphicAPI()
{
	graphicAPIInfo();

	auto appInfo = vk::ApplicationInfo(
		"Hello Triangle",
		VK_MAKE_VERSION(1, 0, 0),
		"No Engine",
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_0
	);

	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	vk::InstanceCreateInfo createInfo{{}, &appInfo, 0, {}, glfwExtensionCount, glfwExtensions};

	m_vkInstance = vk::createInstance(createInfo);
}

void
MainApplication::InitWindow()
{
	glfwInit();

	m_screen_width = std::atoi(GlobalConfig::getConfigData()["screen_width"].get<std::string>().c_str());
	m_screen_height = std::atoi(GlobalConfig::getConfigData()["screen_height"].get<std::string>().c_str());
	m_window_resizable = GlobalConfig::getConfigData()["window_resizable"].get<std::string>() == "true";

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, m_window_resizable ? GLFW_TRUE : GLFW_FALSE);
	m_window = glfwCreateWindow(m_screen_width, m_screen_height, "Vulkan window", nullptr, nullptr);
}

void
MainApplication::cleanUp()
{
	m_vkInstance.destroy();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void
MainApplication::graphicAPIInfo()
{
	uint32_t extensionCount = 0;
	auto result = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	assert(result == vk::Result::eSuccess);

	Logger::getInstance().Log(extensionCount, "extensions supported:\n");

	std::vector<vk::ExtensionProperties> extensions(extensionCount);
	result = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	assert(result == vk::Result::eSuccess);

	for (auto &extension: extensions)
	{

		Logger::getInstance().Log(extension.extensionName, "\n");
	}
}
