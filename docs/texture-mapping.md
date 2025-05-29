# 🖼️ Texture Mapping – Sampling Images in Shaders

> Textures give your geometry visual detail by sampling images in fragment shaders.

---

## 🧱 Vulkan Texture Building Blocks

You need several Vulkan components to use a texture:

- `VkImage` – the actual image data (e.g., .png)
- `VkDeviceMemory` – memory for the image
- `VkImageView` – how shaders interpret the image
- `VkSampler` – how the image is sampled (filtering, addressing)

---

## 🧬 Uploading Texture Data

1. **Load image** into CPU memory (e.g., with `stb_image`)
2. **Create staging buffer** for transfer
3. **Create `VkImage`** with `VK_IMAGE_TILING_OPTIMAL`
4. **Allocate and bind memory** to the image
5. **Transition layout** using `vkCmdPipelineBarrier`
6. **Copy buffer to image** using `vkCmdCopyBufferToImage`
7. **Create image view** for shader access
8. **Create sampler** for filtering and wrapping

🧠 Like a “photo” (image) being printed onto a “canvas” (geometry).

---

## 🪄 Descriptor Setup for Textures

- **Descriptor type**: `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`
- **Update with**:
  - `VkDescriptorImageInfo` (image view + sampler)
  - `VkWriteDescriptorSet`

📘 GLSL Shader Example:
```glsl
layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 color = texture(texSampler, fragTexCoord);
}
```

# 🧠 Quick Recap: Texture Mapping in Vulkan


| Step                  | Vulkan Functions                                         | Purpose                                    |
| --------------------- | -------------------------------------------------------- | ------------------------------------------ |
| Load image (CPU)      | `stb_image.h` or similar                                 | Get image pixels from disk                 |
| Create staging buffer | `vkCreateBuffer`, `vkAllocateMemory`, `vkMapMemory`      | Temp buffer to hold image for GPU transfer |
| Create image          | `vkCreateImage`, `vkAllocateMemory`, `vkBindImageMemory` | Allocate a GPU-resident image              |
| Transition layout     | `vkCmdPipelineBarrier`                                   | Ensure correct layout before copy/use      |
| Copy buffer to image  | `vkCmdCopyBufferToImage`                                 | Upload image to GPU                        |
| Create image view     | `vkCreateImageView`                                      | Interpret image as a 2D texture            |
| Create sampler        | `vkCreateSampler`                                        | Control filtering, address modes           |
| Descriptor setup      | `VkDescriptorImageInfo`, `vkUpdateDescriptorSets`        | Bind texture to the shader                 |
| Shader access         | `sampler2D` + `texture(...)`                             | Sample image in fragment shader            |



# Diagram
[texture-mapping](./diagrams/texture-mapping.md)