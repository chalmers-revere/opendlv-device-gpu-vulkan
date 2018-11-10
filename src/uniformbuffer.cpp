/*
 * Copyright (C) 2018 Ola Benderius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstring>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "device.hpp"
#include "uniformbuffer.hpp"
#include "uniformbufferobject.hpp"
#include "swapchain.hpp"

UniformBuffer::UniformBuffer(Device const &a_device, 
    Swapchain const &a_swapchain): 
  m_vulkan_uniform_buffers(),
  m_vulkan_uniform_buffers_memory()
{
  uint32_t const swapchain_image_count = a_swapchain.GetSwapchainImageCount();
  CreateVulkanUniformBuffers(a_device, swapchain_image_count); 
}

UniformBuffer::~UniformBuffer()
{
}

int8_t UniformBuffer::CreateVulkanUniformBuffers(Device const &a_device, 
    uint32_t const a_uniform_buffer_count)
{
  auto vulkan_device = a_device.GetVulkanDevice();

  VkDeviceSize buffer_size = sizeof(UniformBufferObject);

  m_vulkan_uniform_buffers.resize(a_uniform_buffer_count);
  m_vulkan_uniform_buffers_memory.resize(a_uniform_buffer_count);

  for (size_t i = 0; i < a_uniform_buffer_count; i++) {
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = buffer_size;
    buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    m_vulkan_uniform_buffers[i].reset(new VkBuffer, 
        [vulkan_device](VkBuffer *a_uniform_buffer) {
          vkDestroyBuffer(*vulkan_device, *a_uniform_buffer, nullptr);
          std::cout << "DEBUG: Uniform buffer deleted." << std::endl;
        });

    uint32_t res = vkCreateBuffer(*vulkan_device, &buffer_info, nullptr, 
        &(*m_vulkan_uniform_buffers[i]));
    if (res != VK_SUCCESS) {
      std::cerr << "Failed to create uniform buffer." << std::endl;
      return -1;
    }

    VkMemoryPropertyFlags uniform_memory_properties = 
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkMemoryRequirements uniform_memory_requirements;
    vkGetBufferMemoryRequirements(*vulkan_device, *m_vulkan_uniform_buffers[i], 
        &uniform_memory_requirements);

    bool has_uniform_memory_type = a_device.HasMemoryType(
        uniform_memory_requirements, uniform_memory_properties);
    if (!has_uniform_memory_type) {
      std::cerr << "Failed to find suitable memory type for uniform buffer." 
        << std::endl;
      return -1;
    }

    uint32_t uniform_memory_type_index = a_device.FindMemoryTypeIndex(
        uniform_memory_requirements, uniform_memory_properties);

    VkMemoryAllocateInfo allocation_info = {};
    allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation_info.allocationSize = uniform_memory_requirements.size;
    allocation_info.memoryTypeIndex = uniform_memory_type_index;

    m_vulkan_uniform_buffers_memory[i].reset(new VkDeviceMemory, 
        [vulkan_device](VkDeviceMemory *a_uniform_buffer_memory) {
          vkFreeMemory(*vulkan_device, *a_uniform_buffer_memory, nullptr);
          std::cout << "DEBUG: Uniform buffer memory freed." << std::endl;
        });

    res = vkAllocateMemory(*vulkan_device, &allocation_info, nullptr, 
        &(*m_vulkan_uniform_buffers_memory[i]));
    if (res != VK_SUCCESS) {
      std::cerr << "Failed to allocate uniform buffer memory." 
        << std::endl;
      return -1;
    }

    vkBindBufferMemory(*vulkan_device, *m_vulkan_uniform_buffers[i], 
        *m_vulkan_uniform_buffers_memory[i], 0);
  }
  
  return 0;
}

std::shared_ptr<VkBuffer> UniformBuffer::GetVulkanUniformBuffer(
    uint32_t const a_index) const
{
  return m_vulkan_uniform_buffers[a_index];
}

void UniformBuffer::Update(Device const &a_device, Swapchain const &a_swapchain,
    float const a_time, uint32_t const a_current_image)
{
  auto vulkan_device = a_device.GetVulkanDevice();

  VkExtent2D vulkan_swapchain_extent = a_swapchain.GetVulkanSwapchainExtent();
  float const width = static_cast<float>(vulkan_swapchain_extent.width);
  float const height = static_cast<float>(vulkan_swapchain_extent.height);
  float const aspect_ratio = width / height;

  UniformBufferObject ubo = {};
  ubo.model = glm::rotate(glm::mat4(1.0f), a_time * glm::radians(90.0f),
      glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), 
      glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.projection = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 
      10.0f);
  ubo.projection[1][1] *= -1;

  void *data;
  vkMapMemory(*vulkan_device, *m_vulkan_uniform_buffers_memory[a_current_image], 
      0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(*vulkan_device, 
      *m_vulkan_uniform_buffers_memory[a_current_image]);
}
