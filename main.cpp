#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>

const uint32_t WIDTH{ 800 };
const uint32_t HEIGHT{ 600 };
//All of the useful standard validation is bundled into
//a layer included in the SDK that is known as VK_LAYER_KHRONOS_validation.
const std::vector<const char*> validationLayers{ "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
	const bool enableValidationLayers{ false };
#else
	const bool enableValidationLayers{ true };
#endif // NDEBUG

//vkCreateDebugUtilsMessengerEXT is an extension function it is not automatically loaded
//we've to get the address to the function using vkGetInstanceProcAddr
VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
)
{
	//casting the result of vkGetInstanceProcAddr to this specific function pointer type
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) // pointer to function
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

//vkDestroyDebugUtilsMessengerEXT is an extension function it is not automatically loaded
//we've to get the address to the function using vkGetInstanceProcAddr
void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator
)
{
	//casting the result of vkGetInstanceProcAddr to this specific function pointer type
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) // pointer to function
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

struct QueueFamilyIndices
{
	std::optional<uint32_t> grahicsFamily;
	/*
	* presentation is a queue-specific feature
	It’s actually possible that the queue families supporting drawing commands and
	the ones supporting presentation do not overlap. Therefore we have to take into
	account that there could be a distinct presentation queue
	*/
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return grahicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};


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
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // we don't want to create a opengl context
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // window resizing is more involved so disable it

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); // 1st nullptr is for monitor, 2nd for opengl

	}

	void initVulkan()
	{
		/*
		The very first thing you need to do is initialize the Vulkan library by creating
		an instance. The instance is the connection between your application and
		the Vulkan library and creating it involves specifying some details about your
		application to the driver.
		*/
		createInstance();
		/*
		Setting up the debug messenger function requires the instance to be created first
		Since vkCreateDebugUtilsMessengerEXT func is an extension func the instance is required
		to retrieve it's function pointer. Debug Messenger will handle output of validation layers
		*/
		setupDebugMessenger();
		/*
		The window surface needs to be created right after the instance creation, because
		it can actually influence the physical device selection.
		Window surfaces are an entirely optional component in Vulkan, if you just need off-screen rendering.
		*/
		createSurface();
		/*
		After initializing the Vulkan library through a VkInstance we need to look for
		and select a graphics card in the system that supports the features we need
		*/
		pickPhysicalDevice();
		/*
		Create a logical device to interface with the physical device.
		The logical device creation process is similar to the instance
		creation process and describes the features we want to use. We also need to
		specify which queues to create now that we’ve queried which queue families are
		available. You can even create multiple logical devices from the same physical
		device if you have varying requirements.
		*/
		createLogicalDevice();
		/*
		With the logical device and queue handles we can now actually start using the
		graphics card to do things!
		Make sure to call createSwapChain after logical device creation.
		*/
		createSwapChain();
		/*
		To use any VkImage, including those in the swap chain, in the render pipeline
		we have to create a VkImageView object. An image view is quite literally a
		view into an image. It describes how to access the image and which part of
		the image to access, for example if it should be treated as a 2D texture depth
		texture without any mipmapping levels.
		*/
		createImageViews();
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
		for (auto imageView : swapChainImageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}
		//Destroy swap chain
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		//Logical devices don’t interact directly with instances, which is why it’s not included as a parameter.
		vkDestroyDevice(device, nullptr);
		if (enableValidationLayers)
		{
			//The VkDebugUtilsMessengerEXT object also needs to be cleaned up with a call
			//to vkDestroyDebugUtilsMessengerEXT.Similarly to vkCreateDebugUtilsMessengerEXT
			//the function needs to be explicitly loaded.
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		//GLFW doesn’t offer a special function for destroying a surface, 
		//but that can easily be done through the original API
		//Make sure that the surface is destroyed before the instance.
		vkDestroySurfaceKHR(instance, surface, nullptr);
		//The VkInstance should be only destroyed right before the program exits.
		vkDestroyInstance(instance, nullptr);
		//Once the window is closed, we need to clean up resources by destroying it and terminating GLFW itself
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void createInstance()
	{
		/*
		The instance is the connection between your application and
		the Vulkan library and creating it involves specifying some details about your
		application to the driver.
		*/
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		/*
		This next struct is not optional and tells
		the Vulkan driver which global extensions and validation layers we want to use.
		Global here means that they apply to the entire program and not a specific device
		*/
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		/*
		Vulkan is a platform agnostic API, which means that you need an extension to interface
		with the window system. GLFW has a handy built-in function that returns the
		extension(s) it needs to do that which we can pass to the struct
		*/
		std::vector<const char*> requiredExtensions{ getRequiredExtensions() };
		createInfo.enabledExtensionCount = requiredExtensions.size();
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		createInfo.enabledLayerCount = 0;

		/*
		To retrieve a list of supported extensions before creating an instance,
		there’s the vkEnumerateInstanceExtensionProperties function.
		*/
		uint32_t extenstionCount{ 0 };
		//first arg is to filter, last arg is to store
		vkEnumerateInstanceExtensionProperties(nullptr, &extenstionCount, nullptr);
		std::vector<VkExtensionProperties> extensionProperties(extenstionCount);
		//extensions.data() gives you a VkExtensionProperties*
		vkEnumerateInstanceExtensionProperties(nullptr, &extenstionCount, extensionProperties.data());

		if (!checkExtenstionSupport(requiredExtensions, extensionProperties))
		{
			throw std::runtime_error("all required glfw extensions not supported!");
		}
		
		/*
		The Vulkan API is designed around the idea of minimal driver overhead and one
		of the manifestations of that goal is that there is very limited error checking in
		the API by default. Even mistakes as simple as setting enumerations to incorrect
		values or passing null pointers to required parameters are generally not explicitly
		handled and will simply result in crashes or undefined behavior. Because Vulkan
		requires you to be very explicit about everything you’re doing, it’s easy to make
		many small mistakes like using a new GPU feature and forgetting to request it
		at logical device creation time.
		However, that doesn’t mean that these checks can’t be added to the API. Vulkan
		introduces an elegant system for this known as validation layers. Validation
		layers are optional components that hook into Vulkan function calls to apply
		additional operations. Common operations in validation layers are:
		• Checking the values of parameters against the specification to detect misuse
		• Tracking creation and destruction of objects to find resource leaks
		• Checking thread safety by tracking the threads that calls originate from
		• Logging every call and its parameters to the standard output
		• Tracing Vulkan calls for profiling and replaying
		*/
		// check validation layer support
		// The debugCreateInfo variable is placed outside the if statement to ensure
		// that it is not destroyed before the vkCreateInstance call.
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers)
		{
			uint32_t layerCount{ 0 };
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			if (!checkValidationLayerSupport(availableLayers))
			{
				throw std::runtime_error("all required validation layers not supported");
			}
			createInfo.enabledLayerCount = validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
	}

	std::vector<const char*> getRequiredExtensions()
	{
		//Vulkan is a platform agnostic API, which means that you need an extension to interface
		//with the window system. GLFW has a handy built-in function that returns the extension(s) it needs

		uint32_t glfwExtensionCount{ 0 };
		const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers)
		{
			//To set up a callback in the program to handle messages and the associated
			//details, we have to set up a debug messenger with a callback using the
			//VK_EXT_debug_utils extension. VK_EXT_debug_utils = VK_EXT_DEBUG_UTILS_EXTENSION_NAME
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	bool checkExtenstionSupport(std::vector<const char*> requiredExtensions, const std::vector<VkExtensionProperties>& vkExtensionProperties)
	{
		for (const auto& requiredExt: requiredExtensions)
		{
			bool found{ false };

			for (const auto& extensionProperty : vkExtensionProperties)
			{
				if (strcmp(extensionProperty.extensionName, requiredExt) == 0)
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

	bool checkValidationLayerSupport(const std::vector<VkLayerProperties>& availableLayers)
	{
		for (const auto& layer : validationLayers)
		{
			bool found{ false };

			for (const auto& availableLayer : availableLayers)
			{
				if (strcmp(layer, availableLayer.layerName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				std::cerr << "Missing validation layer: " << layer << std::endl;
				return false;
			}
		}
		return true;
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		//The messageSeverity field allows you to specify all the types of severities
		//you would like your callback to be called for.
		createInfo.messageSeverity =
			//Diagnostic message
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			//Message about behavior that is not necessarily an error, but very likely a bug in your application
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			//Message about behavior that is invalid and may cause crashes
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		// messageType field lets you filter which types of messages your callback is notified about.
		createInfo.messageType =
			//Some event has happened that is unrelated to the specification or performance
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			//Something has happened that violates the specification or indicates a possible mistake
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			//Potential nonoptimal use of Vulkan
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		/*
		The validation layers will print debug messages to the standard output by default,
		but we can also handle them ourselves by providing an explicit callback
		in our program. This will also allow you to decide which kind of messages you
		would like to see, because not all are necessarily (fatal) errors.
		*/
		createInfo.pfnUserCallback = debugCallback;
	}

	void setupDebugMessenger()
	{
		if (!enableValidationLayers) return;
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to setup debug messenger!");
		}
	}

	void createSurface()
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void pickPhysicalDevice()
	{
		uint32_t deviceCount{ 0 };
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		/*
		we need to evaluate each of them and check if they are suitable for the
		operations we want to perform, because not all graphics cards are created equal.
		*/
		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device))
			{
				physicalDevice = device;
				break;
			}
		}
		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find suitable GPU!");
		}
	}

	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		/*
		Almost every operation in Vulkan, anything from drawing to uploading textures, requires commands to be submitted
		to a queue. There are different types of queues that originate from different
		queue families and each family of queues allows only a subset of commands.
		*/
		QueueFamilyIndices indices{ findQueueFamilies(device) };
		bool extensionsSupported{ checkDeviceExtensionSupport(device) };
		bool swapChainAdequate{ false };
		if (extensionsSupported)
		{
			/*
			Vulkan does not have the concept of a “default framebuffer”, hence it requires
			an infrastructure that will own the buffers we will render to before we visualize
			them on the screen. This infrastructure is known as the swap chain and must
			be created explicitly in Vulkan. The swap chain is essentially a queue of images
			that are waiting to be presented to the screen. Our application will acquire
			such an image to draw to it, and then return it to the queue. How exactly the
			queue works and the conditions for presenting an image from the queue depend
			on how the swap chain is set up, but the general purpose of the swap chain is
			to synchronize the presentation of images with the refresh rate of the screen.
			*/
			SwapChainSupportDetails swapChainSupport{ querySwapChainSupport(device) };
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}


		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount{ 0 };
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		/*
		The VkQueueFamilyProperties struct contains some details about the queue
		family, including the type of operations that are supported and the number of
		queues that can be created based on that family.
		*/
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i{ 0 };
		for (const auto& queueFamily : queueFamilies)
		{
			//We need to find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT.
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.grahicsFamily = i;
			}
			/*
			Although the Vulkan implementation may support window system integration,
			that does not mean that every device in the system supports it. we to ensure that
			a device can present images to the surface we created. 
			Since the presentation is a queue-specific feature, the
			problem is actually about finding a queue family that supports presenting to
			the surface we created.
			*/
			VkBool32 presentSupport{ false };
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport)
			{
				indices.presentFamily = i;
			}
			if (indices.isComplete())
			{
				break;
			}
			i++;
		}
		return indices;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		/*
		Not all graphics cards are capable of presenting images directly to a screen for
		various reasons, for example because they are designed for servers and don’t
		have any display outputs. Secondly, since image presentation is heavily tied
		into the window system and the surfaces associated with windows, it is not
		actually part of the Vulkan core. You have to enable the VK_KHR_swapchain
		device extension after querying for its support.
		*/
		uint32_t extensionsCount{ 0 };
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtensions.data());
		for (const auto& extension : deviceExtensions)
		{
			bool found{ false };

			for (const auto& availableExtension : availableExtensions)
			{
				if (strcmp(extension, availableExtension.extensionName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				std::cerr << "Missing device extension: " << extension << std::endl;
				return false;
			}
		}
		return true;
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		/*
		Just checking if a swap chain is available is not sufficient, because it may not
		actually be compatible with our window surface. Creating a swap chain also
		involves a lot more settings than instance and device creation, so we need to
		query for some more details before we’re able to proceed.
		There are basically three kinds of properties we need to check:
		• Basic surface capabilities (min/max number of images in swap chain, min/-
		max width and height of images)
		• Surface formats (pixel format, color space)
		• Available presentation modes
		*/
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
		uint32_t formatCount{ 0 };
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}
		uint32_t presentModeCount{ 0 };
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}
		return details;
	}

	void createLogicalDevice()
	{
		/*
		When creating the logical device, you need to create one or more queues.
		If graphics and presentation are supported by the same queue family, you only need to create one queue.
		But if they’re in different families, you need one for each.
		*/
		QueueFamilyIndices indices{ findQueueFamilies(physicalDevice) };

		//This structure describes the number of queues we want for a single queue family.
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies{ {
				indices.grahicsFamily.value(),
				indices.presentFamily.value()
		} };
		/*
		The currently available drivers will only allow you to create a small number of
		queues for each queue family and you don’t really need more than one. That’s
		because you can create all of the command buffers on multiple threads and then
		submit them all at once on the main thread with a single low-overhead call.
		Vulkan lets you assign priorities to queues to influence the scheduling of command
		buffer execution using floating point numbers between 0.0 and 1.0. This
		is required even if there is only a single queue:
		*/
		float queuePriority{ 1.0f };
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		/*
		The next information to specify is the set of device features that we’ll
		be using. These are the features that we queried support for with
		vkGetPhysicalDeviceFeatures, like geometry
		shaders. Right now we don’t need anything special, so we can simply define
		it and leave everything to VK_FALSE.
		*/
		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = queueCreateInfos.size();
		createInfo.pEnabledFeatures = &deviceFeatures;
		/*
		The remainder of the information bears a resemblance to the VkInstanceCreateInfo
		struct and requires you to specify extensions and validation layers. The difference
		is that these are device specific this time.
		*/
		createInfo.enabledExtensionCount = deviceExtensions.size();
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		/*
		Previous implementations of Vulkan made a distinction between instance and device
		specific validation layers, but this is no longer the case. That means that the
		enabledLayerCount and ppEnabledLayerNames fields of VkDeviceCreateInfo
		are ignored by up-to-date implementations. However, it is still a good idea to
		set them anyway to be compatible with older implementations
		*/
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.grahicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport{ querySwapChainSupport(physicalDevice) };
		VkSurfaceFormatKHR surfaceFormat{ chooseSwapSurfaceFormat(swapChainSupport.formats) };
		VkPresentModeKHR presentMode{ chooseSwapPresentMode(swapChainSupport.presentModes) };
		VkExtent2D extent{ chooseSwapExtent(swapChainSupport.capabilities) };
		/*
		simply sticking to this minimum means that we may sometimes have
		to wait on the driver to complete internal operations before we can acquire
		another image to render to. Therefore it is recommended to request at least one
		more image than the minimum:
		*/
		uint32_t imageCount{ swapChainSupport.capabilities.minImageCount + 1 };
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		/*
		The imageArrayLayers specifies the amount of layers each image consists of.
		This is always 1 unless you are developing a stereoscopic 3D application. The
		imageUsage bit field specifies what kind of operations we’ll use the images in
		the swap chain for. We’re going to render directly to them, which
		means that they’re used as color attachment. It is also possible that you’ll render
		images to a separate image first to perform operations like post-processing. In
		that case you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead
		and use a memory operation to transfer the rendered image to a swap chain
		image.
		*/
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		/*
		we need to specify how to handle swap chain images that will be used
		across multiple queue families. That will be the case in our application if the
		graphics queue family is different from the presentation queue. We’ll be drawing
		on the images in the swap chain from the graphics queue and then submitting
		them on the presentation queue. There are two ways to handle images that are
		accessed from multiple queues:
		• VK_SHARING_MODE_EXCLUSIVE: An image is owned by one queue family
		at a time and ownership must be explicitly transferred before using it in
		another queue family. This option offers the best performance.
		• VK_SHARING_MODE_CONCURRENT: Images can be used across multiple queue
		families without explicit ownership transfers.
		*/
		QueueFamilyIndices indices{ findQueueFamilies(physicalDevice) };
		uint32_t queueFamilyIndices[]{ indices.grahicsFamily.value(), indices.presentFamily.value() };
		if (indices.grahicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		/*
		We can specify that a certain transform should be applied to images in the
		swap chain if it is supported (supportedTransforms in capabilities), like a
		90 degree clockwise rotation or horizontal flip. To specify that you do not want
		any transformation, simply specify the current transformation.
		*/
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		/*
		The compositeAlpha field specifies if the alpha channel should be used for blending
		with other windows in the window system. You’ll almost always want to
		simply ignore the alpha channel, hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
		*/
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		/*
		With Vulkan it’s possible that your
		swap chain becomes invalid or unoptimized while your application is running, for
		example because the window was resized. In that case the swap chain actually
		needs to be recreated from scratch and a reference to the old one must be
		specified in this field. This is a complex topic that we’ll learn more about in a
		future chapter. For now we’ll assume that we’ll only ever create one swap chain.
		*/
		createInfo.oldSwapchain = VK_NULL_HANDLE;
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		/*
		Each VkSurfaceFormatKHR entry contains a format and a colorSpace member.
		The format member specifies the color channels and types. For example,
		VK_FORMAT_B8G8R8A8_SRGB means that we store the B, G, R and alpha channels
		in that order with an 8 bit unsigned integer for a total of 32 bits per pixel.
		The colorSpace member indicates if the SRGB color space is supported or
		not using the VK_COLOR_SPACE_SRGB_NONLINEAR_KHR flag. Note that this flag
		used to be called VK_COLORSPACE_SRGB_NONLINEAR_KHR in old versions of the
		specification.
		For the color space we’ll use SRGB if it is available, because it results in more
		accurate perceived colors. It is also pretty much the standard color space
		for images, like the textures we’ll use later on. Because of that we should
		also use an SRGB color format, of which one of the most common ones is
		VK_FORMAT_B8G8R8A8_SRGB.
		*/
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}
		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		/*
		The presentation mode is arguably the most important
		setting for the swap chain, because it represents the actual conditions for showing
		images to the screen. There are four possible modes available in Vulkan:
		• VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application
		are transferred to the screen right away, which may result in tearing.
		• VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display
		takes an image from the front of the queue when the display is refreshed
		and the program inserts rendered images at the back of the queue. If the
		queue is full then the program has to wait. This is most similar to vertical
		sync as found in modern games. The moment that the display is refreshed
		is known as “vertical blank”.
		• VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the
		previous one if the application is late and the queue was empty at the last
		vertical blank. Instead of waiting for the next vertical blank, the image is
		transferred right away when it finally arrives. This may result in visible
		tearing.
		• VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second
		mode. Instead of blocking the application when the queue is full, the
		images that are already queued are simply replaced with the newer ones.
		This mode can be used to render frames as fast as possible while still
		avoiding tearing, resulting in fewer latency issues than standard vertical
		sync. This is commonly known as “triple buffering”, although the existence
		of three buffers alone does not necessarily mean that the framerate
		is unlocked.
		*/
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	/*
	The swap extent is the resolution of the swap chain images and it’s almost always
	exactly equal to the resolution of the window that we’re drawing to in pixels
	GLFW uses two units when measuring sizes: pixels and screen coordinates. For
	example, the resolution {WIDTH, HEIGHT} that we specified earlier when creating
	the window is measured in screen coordinates. But Vulkan works with
	pixels, so the swap chain extent must be specified in pixels as well. Unfortunately,
	if you are using a high DPI display (like Apple’s Retina display), screen
	coordinates don’t correspond to pixels. Instead, due to the higher pixel density,
	the resolution of the window in pixel will be larger than the resolution in screen
	coordinates. So if Vulkan doesn’t fix the swap extent for us, we can’t just use
	the original {WIDTH, HEIGHT}. Instead, we must use glfwGetFramebufferSize
	to query the resolution of the window in pixel before matching it against the
	minimum and maximum image extent.
	*/
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width;
			int height;
			glfwGetFramebufferSize(window, &width, &height);
			VkExtent2D actualExtent{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height),
			};
			actualExtent.width = std::clamp(
				actualExtent.width,
				capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width
			);
			actualExtent.height = std::clamp(
				actualExtent.height,
				capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height
			);
			return actualExtent;
		}
	}

	void createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i{ 0 }; i < swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			/*
			The viewType and format fields specify how the image data should be interpreted.
			The viewType parameter allows you to treat images as 1D textures, 2D
			textures, 3D textures and cube maps.
			*/
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			/*
			The components field allows you to swizzle the color channels around. For
			example, you can map all of the channels to the red channel for a monochrome
			texture. You can also map constant values of 0 and 1 to a channel. In our case
			we’ll stick to the default mapping.
			*/
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			/*
			The subresourceRange field describes what the image’s purpose is and which
			part of the image should be accessed. Our images will be used as color targets
			without any mipmapping levels or multiple layers.
			*/
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
			/*
			If you were working on a stereographic 3D application, then you would create
			a swap chain with multiple layers. You could then create multiple image views
			for each image representing the views for the left and right eyes by accessing
			different layers.
			*/
			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	/*
		The first parameter specifies the severity of the message, which is one of the
		following flags:
		• VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic
		message
		• VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message
		like the creation of a resource
		• VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about
		behavior that is not necessarily an error, but very likely a bug in your
		application
		• VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about
		behavior that is invalid and may cause crashes

		The messageType parameter can have the following values:
		• VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Some event has happened
		that is unrelated to the specification or performance
		• VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has
		happened that violates the specification or indicates a possible mistake
		• VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential nonoptimal
		use of Vulkan

		The pCallbackData parameter refers to a VkDebugUtilsMessengerCallbackDataEXT
		struct containing the details of the message itself, with the most important
		members being:
		• pMessage: The debug message as a null-terminated string
		• pObjects: Array of Vulkan object handles related to the message
		• objectCount: Number of objects in array

		The pUserData parameter contains a pointer that was specified during
		the setup of the callback and allows you to pass your own data to it.
		The callback returns a boolean that indicates if the Vulkan call that triggered
		the validation layer message should be aborted. If the callback returns true, then
		the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error. This is
		normally only used to test the validation layers themselves, so you should always
		return VK_FALSE.

	*/
	//[storage-class specifier] [return type and attributes] [calling convention] functionName(parameters)
	/*
	static        VKAPI_ATTR         VkBool32           VKAPI_CALL   debugCallback(...)
	^ storage	  ^ attribute macro  ^ return type      ^ call conv  ^ function name

	static __declspec(dllexport) VkBool32 __stdcall debugCallback(...)

		__declspec(dllexport): make it visible to the dynamic loader (if needed)

		VkBool32: return a 32-bit int boolean

		__stdcall: use the correct calling convention for the Vulkan runtime to call it
	*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
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