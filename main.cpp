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
#include <fstream>

const uint32_t WIDTH{ 800 };
const uint32_t HEIGHT{ 600 };
/*
We choose the number 2 because we don’t want the CPU to get too far ahead
of the GPU. With 2 frames in flight, the CPU and the GPU can be working
on their own tasks at the same time. If the CPU finishes early, it will wait
till the GPU finishes rendering before submitting more work. With 3 or more
frames in flight, the CPU could get ahead of the GPU, adding frames of latency.
Generally, extra latency isn’t desired.
*/
const int MAX_FRAMES_IN_FLIGHT{ 2 };
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
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	bool framebufferResized{ false };
	uint32_t currentFrame{ 0 };

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // we don't want to create a opengl context
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // window resizing is more involved so disable it

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); // 1st nullptr is for monitor, 2nd for opengl
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

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
		/*
		Before we can finish creating the pipeline, we need to tell Vulkan about the
		framebuffer attachments that will be used while rendering. We need to specify
		how many color and depth buffers there will be, how many samples to use
		for each of them and how their contents should be handled throughout the
		rendering operations. All of this information is wrapped in a render pass object,
		for which we’ll create a new createRenderPass function.
		*/
		createRenderPass();
		/*
		The graphics pipeline is the sequence
		of operations that take the vertices and textures of your meshes all the
		way to the pixels in the render targets.
		*/
		createGraphicsPipeline();
		/*
		The attachments specified during render pass creation are bound by wrapping
		them into a VkFramebuffer object. A framebuffer object references all of the
		VkImageView objects that represent the attachments. In our case that will be
		only a single one: the color attachment. However, the image that we have to
		use for the attachment depends on which image the swap chain returns when we
		retrieve one for presentation. That means that we have to create a framebuffer
		for all of the images in the swap chain and use the one that corresponds to the
		retrieved image at drawing time.
		*/
		createFramebuffers();
		/*
		Commands in Vulkan, like drawing operations and memory transfers, are not
		executed directly using function calls. You have to record all of the operations
		you want to perform in command buffer objects. The advantage of this is
		that when we are ready to tell the Vulkan what we want to do, all of the
		commands are submitted together and Vulkan can more efficiently process the
		commands since all of them are available together. In addition, this allows
		command recording to happen in multiple threads if so desired.

		We have to create a command pool before we can create command buffers.
		Command pools manage the memory that is used to store the buffers and command
		buffers are allocated from them.
		*/
		createCommandPool();
		createCommandBuffers();

		createSyncObjects();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(device);
	}

	void cleanup()
	{
		cleanupSwapChain();

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		vkDestroyRenderPass(device, renderPass, nullptr);

		for (size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);
		
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

	void cleanupSwapChain()
	{
		//delete the framebuffers before the image views and render pass that
		//they are based on, but only after we’ve finished rendering
		for (auto framebuffer : swapChainFramebuffers)
		{
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}
		for (auto imageView : swapChainImageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}
		//Destroy swap chain
		vkDestroySwapchainKHR(device, swapChain, nullptr);
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

	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		/*
		The loadOp and storeOp determine what to do with the data in the attachment
		before rendering and after rendering. We have the following choices for loadOp:
		• VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
		• VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the
		start
		• VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined;
		we don’t care about them
		storeOp:
		• VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in
		memory and can be read later
		• VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will
		be undefined after the rendering operation
		*/
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		/*
		The loadOp and storeOp apply to color and depth data, and stencilLoadOp /
		stencilStoreOp apply to stencil data.
		*/
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		/*
		Textures and framebuffers in Vulkan are represented by VkImage objects with
		a certain pixel format, however the layout of the pixels in memory can change
		based on what you’re trying to do with an image.
		Some of the most common layouts are:
		• VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
		• VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap
		chain
		• VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination
		for a memory copy operation
		*/
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		/*
		The initialLayout specifies which layout the image will have before the render
		pass begins. The finalLayout specifies the layout to automatically transition
		to when the render pass finishes. Using VK_IMAGE_LAYOUT_UNDEFINED for
		initialLayout means that we don’t care what previous layout the image was
		in. The caveat of this special value is that the contents of the image are not
		guaranteed to be preserved, but that doesn’t matter since we’re going to clear it
		anyway. We want the image to be ready for presentation using the swap chain
		after rendering, which is why we use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as
		finalLayout.
		*/

		/*
		A single render pass can consist of multiple subpasses. Subpasses are subsequent
		rendering operations that depend on the contents of framebuffers in previous
		passes, for example a sequence of post-processing effects that are applied one
		after another.
		*/
		VkAttachmentReference colorAttachmentRef{};
		/*
		The attachment parameter specifies which attachment to reference by its
		index in the attachment descriptions array. Our array consists of a single
		VkAttachmentDescription, so its index is 0. The layout specifies which
		layout we would like the attachment to have during a subpass that uses this
		reference. Vulkan will automatically transition the attachment to this layout
		when the subpass is started. We intend to use the attachment to function as
		a color buffer and the VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout
		will give us the best performance, as its name implies.
		*/
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		/*
		The index of the attachment in this array is directly referenced from the fragment
		shader with the layout(location = 0)out vec4 outColor directive!
		The following other types of attachments can be referenced by a subpass:
		• pInputAttachments: Attachments that are read from a shader
		• pResolveAttachments: Attachments used for multisampling color attachments
		• pDepthStencilAttachment: Attachment for depth and stencil data
		• pPreserveAttachments: Attachments that are not used by this subpass,
		but for which the data must be preserved
		*/
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		/*
		The first two fields specify the indices of the dependency and the dependent subpass.
		The special value VK_SUBPASS_EXTERNAL refers to the implicit subpass before
		or after the render pass depending on whether it is specified in srcSubpass
		or dstSubpass. The index 0 refers to our subpass, which is the first and only
		one. The dstSubpass must always be higher than srcSubpass to prevent cycles
		in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL).
		*/
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		/*
		The next two fields specify the operations to wait on and the stages in which
		these operations occur. We need to wait for the swap chain to finish reading
		from the image before we can access it. This can be accomplished by waiting
		on the color attachment output stage itself.
		*/
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		/*
		The operations that should wait on this are in the color attachment stage and
		involve the writing of the color attachment. These settings will prevent the
		transition from happening until it’s actually necessary (and allowed): when we
		want to start writing colors to it.
		*/
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed creating render pass!");
		}
	}

	/*
	+------------------+
	| Vertex Input     |  <--- Input data (vertices, indices) FIXED FUNCTION
	+------------------+
			 |
			 v
	+------------------+
	| Vertex Shader    |  <--- Transform and process vertices
	+------------------+
			 |
			 v
	+---------------------------+
	| Tessellation Control Shader|  <--- Optional: control tessellation level
	+---------------------------+
			 |
			 v
	+---------------------------+
	| Tessellation Evaluation    |  <--- Optional: calculate tessellated vertices
	| Shader                    |
	+---------------------------+
			 |
			 v
	+------------------+
	| Geometry Shader |  <--- Optional: generate or modify geometry
	+------------------+
			 |
			 v
	+------------------+
	| Rasterization    |  <--- Convert geometry to fragments/pixels FIXED FUNCTION
	+------------------+
			 |
			 v
	+------------------+
	| Fragment Shader  |  <--- Compute color for each fragment (pixel)
	+------------------+
			 |
			 v
	+----------------------+
	| Color Blending       |  <--- Combine fragment colors with frame buffer FIXED FUNCTION
	+----------------------+
			 |
			 v
	+-------------------+
	| Framebuffer Output |  <--- Final image to the screen 
	+-------------------+

	*/
	void createGraphicsPipeline()
	{
		auto vertShaderCode{ readFile("shaders/vert.spv") };
		auto fragShaderCode{ readFile("shaders/frag.spv") };

		/*
		The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU
		doesn’t happen until the graphics pipeline is created. That means that we’re allowed
		to destroy the shader modules again as soon as pipeline creation is finished,
		which is why we’ll make them local variables in the createGraphicsPipeline
		function instead of class members:
		*/
		VkShaderModule vertShaderModule{ createShaderModule(vertShaderCode) };
		VkShaderModule fragShaderModule{ createShaderModule(fragShaderCode) };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main"; // entrypoint func

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main"; // entrypoint func

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		/*
		we’re hard coding the vertex data directly in the vertex shader, we’ll
		fill in this structure to specify that there is no vertex data to load for now.
		*/
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		/*
		The VkPipelineInputAssemblyStateCreateInfo struct describes two things:
		what kind of geometry will be drawn from the vertices and if primitive restart
		should be enabled. The former is specified in the topology member and can
		have values like:
		• VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
		• VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without
		reuse
		• VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used
		as start vertex for the next line
		• VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices
		without reuse
		• VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex
		of every triangle are used as first two vertices of the next triangle
		Normally, the vertices are loaded from the vertex buffer by index in sequential
		order, but with an element buffer you can specify the indices to use yourself.
		This allows you to perform optimizations like reusing vertices. If you set the
		primitiveRestartEnable member to VK_TRUE, then it’s possible to break up
		lines and triangles in the _STRIP topology modes by using a special index of
		0xFFFF or 0xFFFFFFFF.
		*/
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		/*
		A viewport basically describes the region of the framebuffer that the output will
		be rendered to. This will almost always be (0, 0) to (width, height)
		*/
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		/*
		While viewports define the transformation from the image to the framebuffer,
		scissor rectangles define in which regions pixels will actually be stored. Any
		pixels outside the scissor rectangles will be discarded by the rasterizer. They
		function like a filter rather than a transformation.
		*/
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		/*
		The rasterizer takes the geometry that is shaped by the vertices from the
		vertex shader and turns it into fragments to be colored by the fragment
		shader. It also performs depth testing, face culling and the scissor test,
		and it can be configured to output fragments that fill entire polygons
		or just the edges (wireframe rendering).
		*/
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		/*
		If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes
		through the rasterizer stage. This basically disables any output to the framebuffer.
		*/
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		/*
		The polygonMode determines how fragments are generated for geometry. The
		following modes are available:
		• VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
		• VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
		• VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
		Using any mode other than fill requires enabling a GPU feature.
		*/
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		/*
		The lineWidth member describes the thickness of lines
		in terms of number of fragments. The maximum line width that is supported
		depends on the hardware and any line thicker than 1.0f requires you to enable
		the wideLines GPU feature.
		*/
		rasterizer.lineWidth = 1.0f;
		/*
		The cullMode variable determines the type of face culling to use. You can
		disable culling, cull the front faces, cull the back faces or both. The frontFace
		variable specifies the vertex order for faces to be considered front-facing and can
		be clockwise or counterclockwise.
		*/
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		/*
		The VkPipelineMultisampleStateCreateInfo struct configures multisampling,
		which is one of the ways to perform anti-aliasing. It works by combining
		the fragment shader results of multiple polygons that rasterize to the same
		pixel. This mainly occurs along edges, which is also where the most noticeable
		aliasing artifacts occur. Because it doesn’t need to run the fragment shader
		multiple times if only one polygon maps to a pixel, it is significantly less
		expensive than simply rendering to a higher resolution and then downscaling.
		Enabling it requires enabling a GPU feature.
		*/
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;


		/*
		After a fragment shader has returned a color, it needs to be combined with the
		color that is already in the framebuffer. This transformation is known as color
		blending and there are two ways to do it:
		• Mix the old and new value to produce a final color
		• Combine the old and new value using a bitwise operation
		There are two types of structs to configure color blending. The first struct,
		VkPipelineColorBlendAttachmentState contains the configuration per attached
		framebuffer and the second struct, VkPipelineColorBlendStateCreateInfo
		contains the global color blending settings
		*/
		VkPipelineColorBlendAttachmentState colorBlendAttachement{};
		colorBlendAttachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachement.blendEnable = VK_FALSE;
		colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachement;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		/*
		While most of the pipeline state needs to be baked into the pipeline state, a
		limited amount of the state can actually be changed without recreating the
		pipeline at draw time. Examples are the size of the viewport, line width and
		blend constants.
		*/
		std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

		/*
		You can use uniform values in shaders, which are globals similar to dynamic
		state variables that can be changed at drawing time to alter the behavior of
		your shaders without having to recreate them. They are commonly used to pass
		the transformation matrix to the vertex shader, or to create texture samplers
		in the fragment shader.
		These uniform values need to be specified during pipeline creation by creating a
		VkPipelineLayout object.
		*/
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		// The structure also specifies push constants, which are another way of passing dynamic values to shaders
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		/*
		We can now combine all of the structures and objects from the previous chapters
		to create the graphics pipeline! Here’s the types of objects we have now, as a
		quick recap:
		• Shader stages: the shader modules that define the functionality of the
		programmable stages of the graphics pipeline
		• Fixed-function state: all of the structures that define the fixed-function
		stages of the pipeline, like input assembly, rasterizer, viewport and color
		blending
		• Pipeline layout: the uniform and push values referenced by the shader
		that can be updated at draw time
		• Render pass: the attachments referenced by the pipeline stages and their
		usage
		*/

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0; //index of the sub pass
		/*
		Vulkan allows you to create a new graphics pipeline by
		deriving from an existing pipeline. The idea of pipeline derivatives is that
		it is less expensive to set up pipelines when they have much functionality in
		common with an existing pipeline and switching between pipelines from the
		same parent can also be done quicker. You can either specify the handle of an
		existing pipeline with basePipelineHandle or reference another pipeline that
		is about to be created by index with basePipelineIndex. Right now there is
		only a single pipeline, so we’ll simply specify a null handle and an invalid index.
		These values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag
		is also specified in the flags field of VkGraphicsPipelineCreateInfo.
		*/
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}

	VkShaderModule createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		/*
		the size of the bytecode is specified in bytes, but the bytecode pointer is a uint32_t pointer
		rather than a char pointer. Therefore we will need to cast the pointer with
		reinterpret_cast as shown below. When you perform a cast like this, you also
		need to ensure that the data satisfies the alignment requirements of uint32_t.
		Lucky for us, the data is stored in an std::vector where the default allocator
		already ensures that the data satisfies the worst case alignment requirements.
		*/
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			std::runtime_error("failed to create shader module!");
		}
		return shaderModule;
	}

	void createFramebuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i{ 0 }; i < swapChainImageViews.size(); i++)
		{
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices{ findQueueFamilies(physicalDevice) };
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		/*
		There are two possible flags for command pools:
		• VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers
		are rerecorded with new commands very often (may change memory allocation
		behavior)
		• VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command
		buffers to be rerecorded individually, without this flag they all have
		to be reset together
		We will be recording a command buffer every frame, so we want to be able to reset
		and rerecord over it. Thus, we need to set the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		flag bit for our command pool.
		Command buffers are executed
		*/
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		// We’re going to record commands for drawing, which is why we’ve chosen the graphics queue family.
		poolInfo.queueFamilyIndex = queueFamilyIndices.grahicsFamily.value();

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createCommandBuffers()
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		/*
		The level parameter specifies if the allocated command buffers are primary or
		secondary command buffers.
		• VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for
		execution, but cannot be called from other command buffers.
		• VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly,
		but can be called from primary command buffers. Is helpful to reuse
		common operations from primary command buffers.
		*/
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command buffer!");
		}
	}

	void createSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// so that the draw call does not wait on the frame which doesn't exits when doing the 1st frame
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create semaphores!");
			}
		}
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffers, uint32_t imageIndex)
	{
		/*
		We always begin recording a command buffer by calling vkBeginCommandBuffer
		with a small VkCommandBufferBeginInfo structure as argument that specifies
		some details about the usage of this specific command buffer.
		*/
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		/*
		The flags parameter specifies how we’re going to use the command buffer. The
		following values are available:
		• VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command
		buffer will be rerecorded right after executing it once.
		• VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary
		command buffer that will be entirely within a single render pass.
		• VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command
		buffer can be resubmitted while it is also already pending execution.
		*/
		beginInfo.flags = 0;
		/*
		The pInheritanceInfo parameter is only relevant for secondary command
		buffers. It specifies which state to inherit from the calling primary command
		buffers.
		*/
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffers, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		/*
		The first parameters are the render pass itself and the attachments to bind. We
		created a framebuffer for each swap chain image where it is specified as a color
		attachment. Thus we need to bind the framebuffer for the swapchain image we
		want to draw to. Using the imageIndex parameter which was passed in, we can
		pick the right framebuffer for the current swapchain image.
		*/
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;
		/*
		The last two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR,
		which we used as load operation for the color attachment.
		*/
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		/*
		The final parameter controls how the drawing commands
		within the render pass will be provided. It can have one of two values:
		• VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded
		in the primary command buffer itself and no secondary command
		buffers will be executed.
		• VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass
		commands will be executed from secondary command buffers.
		*/
		vkCmdBeginRenderPass(commandBuffers, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		//The second parameter specifies if the pipeline object is a graphics or compute pipeline.
		vkCmdBindPipeline(commandBuffers, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		/*
		we did specify viewport and scissor
		state for this pipeline to be dynamic. So we need to set them in the command
		buffer before issuing our draw command:
		*/
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = swapChainExtent.width;
		viewport.height = swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffers, 0, 1, &scissor);

		/*
		• vertexCount: Even though we don’t have a vertex buffer, we technically
		still have 3 vertices to draw.
		• instanceCount: Used for instanced rendering, use 1 if you’re not doing
		that.
		• firstVertex: Used as an offset into the vertex buffer, defines the lowest
		value of gl_VertexIndex.
		• firstInstance: Used as an offset for instanced rendering, defines the
		lowest value of gl_InstanceIndex.
		*/
		vkCmdDraw(commandBuffers, 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffers);

		if (vkEndCommandBuffer(commandBuffers) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	/*
	At a high level, rendering a frame in Vulkan consists of a common set of steps:
	• Wait for the previous frame to finish
	• Acquire an image from the swap chain
	• Record a command buffer which draws the scene onto that image
	• Submit the recorded command buffer
	• Present the swap chain image
	*/
	void drawFrame()
	{
		/*
		At the start of the frame, we want to wait until the previous frame has finished,
		so that the command buffer and semaphores are available to use.
		The VK_TRUE we pass here indicates that we want to wait for all fences, but in the case of a
		single one it doesn’t matter. This function also has a timeout parameter that
		we set to the maximum value of a 64 bit unsigned integer, UINT64_MAX, which
		effectively disables the timeout.
		*/
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		/*
		The last parameter specifies a variable to output the index of the swap
		chain image that has become available. The index refers to the VkImage
		in our swapChainImages array. We’re going to use that index to pick the
		VkFrameBuffer
		*/
		VkResult result{ 
			vkAcquireNextImageKHR(device, swapChain, UINT32_MAX, imageAvailableSemaphores[currentFrame],
				VK_NULL_HANDLE, &imageIndex)
		};

		/*
		The vkAcquireNextImageKHR and vkQueuePresentKHR functions can return the following
		special values.
		• VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible
		with the surface and can no longer be used for rendering. Usually happens
		after a window resize.
		• VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully
		present to the surface, but the surface properties are no longer matched
		exactly.
		*/
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		// Only reset the fence if we are submitting work
		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		/*
		The first three parameters specify which semaphores to wait on before execution
		begins and in which stage(s) of the pipeline to wait. We want to wait with
		writing colors to the image until it’s available, so we’re specifying the stage of
		the graphics pipeline that writes to the color attachment. That means that
		theoretically the implementation can already start executing our vertex shader
		and such while the image is not yet available. Each entry in the waitStages
		array corresponds to the semaphore with the same index in pWaitSemaphores.
		*/
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		/*
		The signalSemaphoreCount and pSignalSemaphores parameters specify which
		semaphores to signal once the command buffer(s) have finished execution. In
		our case we’re using the renderFinishedSemaphore for that purpose.
		*/
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		/*
		The first two parameters specify which semaphores to wait on before presentation
		can happen, just like VkSubmitInfo. Since we want to wait on the command
		buffer to finish execution, thus our triangle being drawn, we take the semaphores
		which will be signalled and wait on them, thus we use signalSemaphores.
		*/
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		/*
		The next two parameters specify the swap chains to present images to and the
		index of the image for each swap chain. This will almost always be a single one.
		*/
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		/*
		There is one last optional parameter called pResults. It allows you to specify
		an array of VkResult values to check for every individual swap chain if presentation
		was successful. It’s not necessary if you’re only using a single swap chain,
		because you can simply use the return value of the present function.
		*/
		presentInfo.pResults = nullptr;

		//The vkQueuePresentKHR function submits the request to present an image to the swap chain.
		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}

		//By using the modulo (%) operator, we ensure that the frame index loops around
		//after every MAX_FRAMES_IN_FLIGHT enqueued frames.
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void recreateSwapChain()
	{
		int width{ 0 };
		int height{ 0 };
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}
		/*
		We first call vkDeviceWaitIdle, because we
		shouldn’t touch resources that may still be in use. Obviously, we’ll have to
		recreate the swap chain itself. The image views need to be recreated because
		they are based directly on the swap chain images. Finally, the framebuffers
		directly depend on the swap chain images, and thus must be recreated as well.
		*/
		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createFramebuffers();
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

	static std::vector<char> readFile(const std::string& filename)
	{
		/*
		• ate: Start reading at the end of the file
		• binary: Read the file as binary file (avoid text transformations)
		The advantage of starting to read at the end of the file is that we can use the
		read position to determine the size of the file and allocate a buffer:
		*/
		std::ifstream file{ filename, std::ios::ate | std::ios::binary };
		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}
		size_t fileSize{ (size_t)file.tellg() };
		std::vector<char> buffer(fileSize);
		//seek back to the beginning of the file and read all of the bytes at once
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app{ reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window)) };
		app->framebufferResized = true;
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