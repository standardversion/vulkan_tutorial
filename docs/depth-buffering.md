# 🌊 Depth Buffering – Managing Fragment Overlap

> Depth buffering ensures that only the closest fragments (pixels) are rendered when multiple surfaces overlap.

---

## 🧱 What is a Depth Buffer?

- A **2D image** (like a texture) storing depth values (Z-values) per pixel
- Used during rasterization to decide whether to discard or keep a fragment
- Format: typically `VK_FORMAT_D32_SFLOAT` or `VK_FORMAT_D24_UNORM_S8_UINT` (if using depth + stencil)

---

## 🏗️ Creating a Depth Buffer

1. **Choose a supported depth format**
   - Query device for supported depth formats with `vkGetPhysicalDeviceFormatProperties`
2. **Create a `VkImage`** with:
   - Usage: `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT`
   - Tiling: `VK_IMAGE_TILING_OPTIMAL`
3. **Allocate memory** and bind it to the image
4. **Create an `VkImageView`** to access the depth image
5. **Transition the layout** to `VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL`

---

## 🖼️ Adding Depth to the Render Pass

### In `VkAttachmentDescription`:
- Set format to the depth format
- LoadOp = `VK_ATTACHMENT_LOAD_OP_CLEAR`
- StoreOp = `VK_ATTACHMENT_STORE_OP_DONT_CARE` (or `STORE` if reading back)

### In `VkSubpassDescription`:
- Fill `pDepthStencilAttachment` with a reference to the depth attachment

---

## 🧩 Updating the Framebuffer

- Add the `VkImageView` of the depth buffer to the `VkFramebuffer`
- It becomes part of the attachment list along with color targets

---

## 🎮 Enabling Depth Test in the Pipeline

- Set up `VkPipelineDepthStencilStateCreateInfo`
  - `depthTestEnable = VK_TRUE`
  - `depthWriteEnable = VK_TRUE`
  - `depthCompareOp = VK_COMPARE_OP_LESS` (default behavior: closer fragments win)

Pass this struct during pipeline creation via `VkGraphicsPipelineCreateInfo`.

---

## 🧠 Quick Recap: Depth Buffering Steps

| Step                     | Vulkan Functions / Structs                           | Purpose                                      |
|--------------------------|------------------------------------------------------|----------------------------------------------|
| Choose depth format      | `vkGetPhysicalDeviceFormatProperties`               | Find compatible format (e.g., D32_SFLOAT)    |
| Create depth image       | `vkCreateImage`, `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT` | Reserve image for depth data     |
| Allocate memory          | `vkAllocateMemory`, `vkBindImageMemory`             | Allocate and bind GPU memory                 |
| Create image view        | `vkCreateImageView`                                 | View the image as a depth attachment         |
| Layout transition        | `vkCmdPipelineBarrier`                              | Transition to usable depth layout            |
| Add to render pass       | `VkAttachmentDescription`, `VkSubpassDescription`   | Include depth in the render pipeline         |
| Add to framebuffer       | `VkFramebuffer`                                     | Attach to render target                      |
| Enable depth test        | `VkPipelineDepthStencilStateCreateInfo`             | Configure Z-buffer behavior                  |

---

🧠 **Why Use It?**
- Prevents **overdraw** and visual glitches when geometry overlaps
- Essential for proper 3D rendering (e.g., showing a near object in front of a far one)

---

# Diagram
[depth-buffering](./diagrams/depth-buffering.md)