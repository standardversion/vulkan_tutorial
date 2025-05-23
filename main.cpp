#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

const uint32_t WIDTH{ 800 };
const uint32_t HEIGHT{ 600 };

class HelloTriangleApplication
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* window;
	VkInstance instance;

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // we don't want to create a opengl context
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // window resizing is more involved so disable it

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); // 1st nullptr is for monitor, 2nd for opengl

	}

	bool checkGLFWExtenstionSupport(const char** extensions, uint32_t count, const std::vector<VkExtensionProperties>& vkExtensions)
	{
		for (uint32_t i{ 0 }; i < count; ++i)
		{
			const char* requiredExt{ extensions[i] };
			bool found{ false };

			for (const auto& ext : vkExtensions)
			{
				if (strcmp(ext.extensionName, requiredExt) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				std::cerr << "Missing GLFW extension: " << requiredExt << std::endl;
				return false;
			}
		}
		return true;
	}

	void createInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		//Vulkan is a platform agnostic API, which means that you need an extension to interface
		//with the window system. GLFW has a handy built-in function that returns the extension(s) it needs
		
		uint32_t glfwExtensionCount{ 0 };
		const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = 0;

		// check extension support
		uint32_t extenstionCount{ 0 };
		//first arg is to filter, last arg is to store
		vkEnumerateInstanceExtensionProperties(nullptr, &extenstionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extenstionCount);
		//extensions.data() gives you a VkExtensionProperties*
		vkEnumerateInstanceExtensionProperties(nullptr, &extenstionCount, extensions.data());

		std::cout << "available extensions:\n";
		for (const auto& extension : extensions)
		{
			std::cout << '\n' << extension.extensionName << '\n';
		}

		if (!checkGLFWExtenstionSupport(glfwExtensions, glfwExtensionCount, extensions))
		{
			throw std::runtime_error("all required glfw extensions not supported!");
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
	}

	void initVulkan()
	{
		createInstance();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
};

int main()
{
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}