## High level overview of Vulkan initialization

1. Create a Vulkan instance
2. Set up validation layers (if enabled)
3. Create a surface (platform-specific)
4. Pick a physical device (GPU)
5. Create a logical device and queues
6. Create a swap chain
7. Create image views for swap chain images
8. Create a render pass
9. Create graphics pipeline (shaders, states, etc.)
10. Create framebuffers
11. Create command pool and command buffers
12. Record commands to draw
13. Create synchronization primitives
14. Draw loop (acquire image → record/submit commands → present)


# Vulkan Boilerplate Grouped by Chunks / Layers

---

## 🔹 Instance & Debug

- `vkCreateInstance`  
  Create the Vulkan instance, specify extensions and validation layers.

- `VkDebugUtilsMessengerEXT` (optional)  
  Set up a debug messenger to get detailed validation messages.

---

## 🔹 Device & Surface

- `vkCreateSurfaceKHR`  
  Create a window surface (platform-specific: GLFW, SDL, etc.)

- Choose `VkPhysicalDevice`  
  Select a GPU that supports necessary features and extensions.

- `vkCreateDevice`  
  Create a logical device.

- Retrieve `VkQueue`s  
  Get graphics and presentation queues.

---

## 🔹 Swapchain

- `vkCreateSwapchainKHR`  
  Create a swapchain for presenting images to the surface.

- `vkGetSwapchainImagesKHR`  
  Get the images from the swapchain.

- `vkCreateImageView`  
  Create an image view for each swapchain image.

---

## 🔹 Pipeline

- `vkCreateRenderPass`  
  Define attachments (e.g., color, depth), subpasses, and dependencies.

- `vkCreateShaderModule`  
  Load and compile SPIR-V shader binaries.

- `vkCreatePipelineLayout`  
  Define descriptor set layouts and push constants.

- `vkCreateGraphicsPipelines`  
  Create the graphics pipeline (fixed-function + shaders).

---

## 🔹 Framebuffer & Command Buffers

- `vkCreateFramebuffer`  
  Create a framebuffer for each image view.

- `vkCreateCommandPool`  
  Create a command pool for allocating command buffers.

- `vkAllocateCommandBuffers`  
  Allocate command buffers.

- Record drawing commands  
  Begin render pass, bind pipeline, draw, end render pass.

---

## 🔹 Synchronization & Draw Loop

- `vkCreateSemaphore` and `vkCreateFence`  
  Create synchronization objects.

- Main Loop Steps:
  1. Acquire image from swapchain
  2. Submit command buffer
  3. Present image to the surface

---