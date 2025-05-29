# Vertex Buffers

# 🧶 Vertex Buffers – Feeding Geometry to the GPU

> Stores the vertex data (positions, colors, normals, etc.) your shaders will process.

- `vkCreateBuffer` – creates a buffer object  
- `vkAllocateMemory` & `vkBindBufferMemory` – reserve and bind GPU memory  
- `vkMapMemory`, `memcpy`, `vkUnmapMemory` – upload data to the buffer  

**Purpose**: Vertex buffers are **how you tell Vulkan what to draw**. They hold the raw geometry that your vertex shader consumes.

🧠 Think of it as:
- **Buffer** = a memory container (like an empty box)
- **Vertex Buffer** = a buffer with structured geometry inside
- **Binding** = telling the pipeline where your box is and how to interpret it

---

## 🧩 Input Descriptions – Making Sense of Vertex Data

> Vulkan needs explicit descriptions of what your vertex data looks like.

- `VkVertexInputBindingDescription` – stride, input rate  
- `VkVertexInputAttributeDescription` – format, offset, location  

These get passed into the pipeline creation (`VkPipelineVertexInputStateCreateInfo`).

**Purpose**: Tells Vulkan:  
_"Each vertex is X bytes, and here’s where each attribute lives."_

---

## 📤 Uploading Vertex Data – CPU → GPU

Steps:
1. Create a `VkBuffer` with usage `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`
2. Allocate memory (`vkAllocateMemory`) with appropriate flags (like `HOST_VISIBLE`)
3. Copy vertex data using `vkMapMemory` + `memcpy` + `vkUnmapMemory`
4. (Optional but preferred) Use a **staging buffer** and `vkCmdCopyBuffer` for best performance

🚀 **Staging Buffer Tip**:  
Upload from CPU to a temporary buffer (host-visible), then copy to a GPU-local buffer for efficient access during rendering.

---

## 🪢 Binding Buffers in the Draw Call

> Once you have a buffer, you need to bind it before drawing.

- `vkCmdBindVertexBuffers` – binds vertex buffer(s) to a command buffer  
- `vkCmdDraw` – draws geometry using currently bound buffers

📘 Example:
```cpp
VkBuffer vertexBuffers[] = { vertexBuffer };
VkDeviceSize offsets[] = { 0 };
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
```

# 🧠 Quick Recap: Vertex Buffers
| Step                   | Vulkan Functions                                                       | Purpose                                   |
| ---------------------- | ---------------------------------------------------------------------- | ----------------------------------------- |
| Create buffer          | `vkCreateBuffer`                                                       | Reserve buffer object for vertex data     |
| Allocate & bind memory | `vkAllocateMemory`, `vkBindBufferMemory`                               | Attach memory to the buffer               |
| Upload data            | `vkMapMemory`, `memcpy`, `vkUnmapMemory`                               | Copy CPU-side vertex data into GPU memory |
| (Optional) Staging     | `vkCmdCopyBuffer`                                                      | Upload data via intermediate buffer       |
| Input description      | `VkVertexInputBindingDescription`, `VkVertexInputAttributeDescription` | Describe vertex format                    |
| Bind in draw pass      | `vkCmdBindVertexBuffers`, `vkCmdDraw`                                  | Tell Vulkan what geometry to use          |

# Index Buffers

# 🔢 Index Buffers – Reusing Vertex Data Efficiently

> Stores the indices that reference vertices in your vertex buffer, allowing reuse and reducing duplication.

- `vkCreateBuffer` – create a buffer for indices  
- `vkAllocateMemory` & `vkBindBufferMemory` – allocate and bind memory  
- `vkMapMemory`, `memcpy`, `vkUnmapMemory` – upload index data to the buffer  

**Purpose**: Index buffers enable **efficient reuse** of vertex data. Instead of duplicating vertices, you reference them by index — especially useful for drawing complex meshes.

🧠 Think of it as:
- **Vertex Buffer** = list of points (e.g., A, B, C, D)
- **Index Buffer** = instructions on how to connect them (e.g., draw triangle A-B-C, then A-C-D)

---

## 🧮 Index Types – 16-bit vs. 32-bit

> Vulkan supports different index formats, based on your mesh size.

- `VK_INDEX_TYPE_UINT16` – up to 65,535 unique vertices  
- `VK_INDEX_TYPE_UINT32` – for large models with more vertices  

💡 Use `uint16_t` when possible to save memory and bandwidth.

---

## 📤 Uploading Index Data – CPU → GPU

Steps:
1. Create a `VkBuffer` with `VK_BUFFER_USAGE_INDEX_BUFFER_BIT`
2. Allocate memory with `vkAllocateMemory` and bind with `vkBindBufferMemory`
3. Upload the index array using `vkMapMemory`, `memcpy`, and `vkUnmapMemory`
4. (Optional) Use a **staging buffer** and copy it to GPU-local memory via `vkCmdCopyBuffer`

---

## 🪢 Binding and Using Index Buffers in Draw Call

- `vkCmdBindIndexBuffer` – binds an index buffer to a command buffer  
- `vkCmdDrawIndexed` – issues a draw using indices from the bound buffer

📘 Example:
```cpp
vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
```


# 🧠 Quick Recap: Index Buffers
| Step                   | Vulkan Functions                               | Purpose                                |
| ---------------------- | ---------------------------------------------- | -------------------------------------- |
| Create buffer          | `vkCreateBuffer`                               | Allocate space for index data          |
| Allocate & bind memory | `vkAllocateMemory`, `vkBindBufferMemory`       | Attach memory to the index buffer      |
| Upload data            | `vkMapMemory`, `memcpy`, `vkUnmapMemory`       | Transfer index data from CPU to GPU    |
| (Optional) Staging     | `vkCmdCopyBuffer`                              | Upload via intermediate staging buffer |
| Bind in draw pass      | `vkCmdBindIndexBuffer`, `vkCmdDrawIndexed`     | Draw geometry using indices            |
| Choose index type      | `VK_INDEX_TYPE_UINT16`, `VK_INDEX_TYPE_UINT32` | Match format to your mesh size         |


# Diagram
[vertex-index-buffers](./diagrams/vertex-index-buffers.md)