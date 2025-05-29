# 📦 Uniform Buffers & Descriptor Sets – Supplying Shader Parameters

> Uniform buffers hold dynamic values (like transformation matrices or lighting data) that shaders access every frame.

---

## 🎒 Uniform Buffers – Constant Data for Shaders

- `vkCreateBuffer` – create a buffer with `VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT`
- `vkAllocateMemory`, `vkBindBufferMemory` – allocate and attach memory
- `vkMapMemory`, `memcpy`, `vkUnmapMemory` – upload uniform data from CPU

**Purpose**: Provide dynamic per-frame or per-object data to shaders without recompiling them.

🧠 Think of it as:
- A **read-only inbox** that shaders peek into every frame
- Ideal for **matrices**, **camera data**, or **lighting uniforms**

---

## 🧩 Descriptor Sets – Binding Resources to Shaders

> Vulkan requires explicit declarations and bindings for shader resources (like uniform buffers, textures, etc.)

### Key components:
- `VkDescriptorSetLayout` – describes what types of resources are used
- `VkDescriptorPool` – memory manager for allocating descriptor sets
- `VkDescriptorSet` – actual resource bindings (like uniforms or textures)
- `vkUpdateDescriptorSets` – link a buffer to a descriptor set

📘 Usage Flow:
1. Define descriptor layout (`VkDescriptorSetLayoutBinding`)
2. Allocate descriptor set from pool
3. Fill descriptor with buffer info using `VkWriteDescriptorSet`

---

## 🛠️ Binding Uniforms in Shaders

You must bind the descriptor set **in the pipeline layout** and during command buffer recording:

- `vkCreatePipelineLayout` – takes in the descriptor set layout(s)
- `vkCmdBindDescriptorSets` – binds descriptor sets to a command buffer before drawing

📘 GLSL shader example:
```glsl
layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```

# 🧠 Quick Recap: Uniform Buffers & Descriptor Sets

| Step                     | Vulkan Functions                                              | Purpose                                     |
| ------------------------ | ------------------------------------------------------------- | ------------------------------------------- |
| Create uniform buffer    | `vkCreateBuffer` + `VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT`       | Reserve space for per-frame/per-object data |
| Allocate & bind memory   | `vkAllocateMemory`, `vkBindBufferMemory`                      | Attach memory to uniform buffer             |
| Upload uniform data      | `vkMapMemory`, `memcpy`, `vkUnmapMemory`                      | Send updated data from CPU                  |
| Create descriptor layout | `VkDescriptorSetLayoutBinding`, `vkCreateDescriptorSetLayout` | Define what the shader expects              |
| Allocate descriptor set  | `vkCreateDescriptorPool`, `vkAllocateDescriptorSets`          | Get memory for descriptors                  |
| Update descriptor set    | `vkUpdateDescriptorSets` with `VkWriteDescriptorSet`          | Link uniform buffer to descriptor           |
| Create pipeline layout   | `vkCreatePipelineLayout`                                      | Combine descriptor layouts into pipeline    |
| Bind during draw         | `vkCmdBindDescriptorSets`                                     | Activate the descriptor set before drawing  |


# Diagram
[uniform-buffers-descriptor-sets](./diagrams/uniform-buffers-descriptor-sets.md)