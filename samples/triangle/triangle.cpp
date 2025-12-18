#include "vulkanexamplebase.h"

constexpr uint32_t MAX_CONCURRENT_FRAMES = 2;

class VulkanExample : public VulkanExampleBase
{
public:
	struct Vertex
	{
		float position[3];
		float color[3];
	};

	struct
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer{ VK_NULL_HANDLE };
	} vertices;

	struct
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer{ VK_NULL_HANDLE };
		uint32_t count{ 0 };
	} indices;

	struct UniformBuffer
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer{ VK_NULL_HANDLE };
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
		uint8_t* mapped{ nullptr };
	};
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;
	
	struct ShaderData
	{
		glm::mat4 projectionMatrix;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
	};

	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> presentCompleteSemaphores{};
	std::vector<VkSemaphore> renderCompleteSemaphores{};

	VkCommandPool commandPool{ VK_NULL_HANDLE };
	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{};
	std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{};

	uint32_t currentFrame{ 0 };

public:
	VulkanExample() : VulkanExampleBase()
	{
		title = "Triangle";
		settings.overlay = false;
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
		camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);
	}

	~VulkanExample() override
	{
		if (device)
		{
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			vkDestroyBuffer(device, vertices.buffer, nullptr);
			vkFreeMemory(device, vertices.memory, nullptr);
			vkDestroyBuffer(device, indices.buffer, nullptr);
			vkFreeMemory(device, indices.memory, nullptr);
			vkDestroyCommandPool(device, commandPool, nullptr);
			for (size_t i = 0; i < presentCompleteSemaphores.size(); i++) {
				vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			}
			for (size_t i = 0; i < renderCompleteSemaphores.size(); i++) {
				vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			}
			for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
				vkDestroyFence(device, waitFences[i], nullptr);
				vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);
				vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
			}
		}
	}
	
	void setupDepthStencil() override
	{
		VkImageCreateInfo imageInfo = vks::initializers::imageCreateInfo();
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = depthFormat;
		imageInfo.extent = { width, height, 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &depthStencil.image));

		VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, depthStencil.image, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &depthStencil.memory));
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));

		VkImageViewCreateInfo depthStencilViewInfo = vks::initializers::imageViewCreateInfo();
		depthStencilViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilViewInfo.format = depthFormat;
		depthStencilViewInfo.subresourceRange = {};
		depthStencilViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT)
		{
			depthStencilViewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		depthStencilViewInfo.subresourceRange.baseMipLevel = 0;
		depthStencilViewInfo.subresourceRange.levelCount = 1;
		depthStencilViewInfo.subresourceRange.baseArrayLayer = 0;
		depthStencilViewInfo.subresourceRange.layerCount = 1;
		depthStencilViewInfo.image = depthStencil.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilViewInfo, nullptr, &depthStencil.view));
	}

	void setupRenderPass() override
	{
		std::array<VkAttachmentDescription, 2> attachments{};

		attachments[0].format = swapChain.colorFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		attachments[1].format = depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference{};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pResolveAttachments = nullptr;
		subpassDescription.pDepthStencilAttachment = &depthReference;
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;

		std::array<VkSubpassDependency, 2> dependencies{};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = 0;

		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = 0;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		dependencies[1].dependencyFlags = 0;

		VkRenderPassCreateInfo renderPassInfo = vks::initializers::renderPassCreateInfo();
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();
		vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
	}

	void setupFrameBuffer() override
	{
		frameBuffers.resize(swapChain.imageCount);
		for (size_t i = 0; i < frameBuffers.size(); i++)
		{
			std::array<VkImageView, 2> attchments{};
			attchments[0] = swapChain.imageViews[i];
			attchments[1] = depthStencil.view;

			VkFramebufferCreateInfo frameBufferInfo = vks::initializers::framebufferCreateInfo();
			frameBufferInfo.renderPass = renderPass;
			frameBufferInfo.attachmentCount = static_cast<uint32_t>(attchments.size());
			frameBufferInfo.pAttachments = attchments.data();
			frameBufferInfo.width = width;
			frameBufferInfo.height = height;
			frameBufferInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &frameBuffers[i]));
		}
	}

	void createSynchronizationPrimitives()
	{
		for (auto& fence : waitFences)
		{
			VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
			VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
		}

		for (auto& semaphore : presentCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreInfo = vks::initializers::semaphoreCreateInfo();
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
		}

		renderCompleteSemaphores.resize(swapChain.images.size());
		for (auto& semaphore : renderCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreInfo = vks::initializers::semaphoreCreateInfo();
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
		}
	}

	uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
	{
		for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
		{
			if (typeBits & 1)
			{
				if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}
			typeBits >>= 1;
		}
		throw std::runtime_error("Could not find a suitable memory type!");
	}

	void createCommandBuffers()
	{
		VkCommandPoolCreateInfo commandPoolInfo = vks::initializers::commandPoolCreateInfo();
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool));

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(commandBuffers.size()));
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data()));
	}

	void createVertexBuffer()
	{
		std::vector<Vertex> vertexBuffer =
		{
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(sizeof(Vertex) * vertexBuffer.size());

		std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
		indices.count = static_cast<uint32_t>(indexBuffer.size());
		uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

		VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memoryRequirements;

		struct StagingBuffer
		{
			VkDeviceMemory memory;
			VkBuffer buffer;
		};

		struct
		{
			StagingBuffer vertices;
			StagingBuffer indices;
		} stagingBuffers;

		void* data;

		// vertex
		VkBufferCreateInfo vertexBufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vertexBufferSize);
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer));
		vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &stagingBuffers.vertices.memory));
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0, vertexBufferSize, 0, &data));
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, stagingBuffers.vertices.memory);
		vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0);

		vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertices.buffer));
		vkGetBufferMemoryRequirements(device, vertices.buffer, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &vertices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

		// index
		VkBufferCreateInfo indexBufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, indexBufferSize);
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexBufferInfo, nullptr, &stagingBuffers.indices.buffer));
		vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &stagingBuffers.indices.memory));
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(device, stagingBuffers.indices.memory);
		vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);

		indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexBufferInfo, nullptr, &indices.buffer));
		vkGetBufferMemoryRequirements(device, indices.buffer, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &indices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, indices.buffer, indices.memory, 0));

		VkCommandBuffer copyCmd;
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &copyCmd));

		VkCommandBufferBeginInfo commandBufferBeginInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &commandBufferBeginInfo));
		VkBufferCopy copyRegion{};
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer, 1, &copyRegion);
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

		VkSubmitInfo submitInfo = vks::initializers::submitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCmd;

		VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(0);
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));

		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
		
		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);
		vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
		vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
		vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
		vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);
	}

	void createUniformBuffers()
	{
		VkMemoryRequirements memoryRequirements;
		VkBufferCreateInfo bufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ShaderData));
		VkMemoryAllocateInfo allocateInfo = vks::initializers::memoryAllocateInfo();

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++)
		{
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i].buffer));
			vkGetBufferMemoryRequirements(device, uniformBuffers[i].buffer, &memoryRequirements);
			allocateInfo.allocationSize = memoryRequirements.size;
			allocateInfo.memoryTypeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocateInfo, nullptr, &uniformBuffers[i].memory));
			VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].buffer, uniformBuffers[i].memory, 0));
			VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(ShaderData), 0, (void**)&uniformBuffers[i].mapped))
		}
	}

	void createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding layoutBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

		VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = vks::initializers::descriptorSetLayoutCreateInfo(&layoutBinding, 1);
		
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayout));
	}

	void createDescriptorPool()
	{
		VkDescriptorPoolSize descriptorTypeCounts[1]{};
		descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;

		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(1, descriptorTypeCounts, MAX_CONCURRENT_FRAMES);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void createDescriptorSets()
	{
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++)
		{
			VkDescriptorSetAllocateInfo descriptorSetAllocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &uniformBuffers[i].descriptorSet));

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i].buffer;
			bufferInfo.range = sizeof(ShaderData);

			VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(uniformBuffers[i].descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufferInfo);
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	void createPipelines()
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
		{
			loadShader(getShadersPath() + "triangle/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "triangle/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		std::vector<VkVertexInputBindingDescription> vertexInputBinding =
		{
			vks::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
		};
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = 
		{
			vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),
			vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color))
		};
		VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = vks::initializers::pipelineVertexInputStateCreateInfo(vertexInputBinding, vertexInputAttributes);

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

		VkPipelineViewportStateCreateInfo viewportStateInfo = vks::initializers::pipelineViewportStateCreateInfo(1, 1);

		VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		rasterizationStateInfo.depthClampEnable = VK_FALSE;
		rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateInfo.depthBiasEnable = VK_FALSE;
		rasterizationStateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampleStateInfo = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

		VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilStateInfo.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
		VkPipelineDynamicStateCreateInfo dynamicStateInfo = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		VkGraphicsPipelineCreateInfo pipelineInfo = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputStateInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyStateInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &rasterizationStateInfo;
		pipelineInfo.pMultisampleState = &multisampleStateInfo;
		pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
		pipelineInfo.pColorBlendState = &colorBlendStateInfo;
		pipelineInfo.pDynamicState = &dynamicStateInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline);
	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		createSynchronizationPrimitives();
		createCommandBuffers();
		createVertexBuffer();
		createUniformBuffers();
		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSets();
		createPipelines();
		prepared = true;
	}

	void render() override
	{
		if (!prepared)
		{
			return;
		}

		VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));

		uint32_t imageIndex;
		auto result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if ((result == VK_ERROR_OUT_OF_DATE_KHR))
		{
			windowResize();
			return;
		}
		else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR))
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		ShaderData shaderData{};
		shaderData.projectionMatrix = camera.matrices.perspective;
		shaderData.viewMatrix = camera.matrices.view;
		shaderData.modelMatrix = glm::mat4(1.0f);
		memcpy(uniformBuffers[currentFrame].mapped, &shaderData, sizeof(ShaderData));

		VK_CHECK_RESULT(vkResetCommandBuffer(commandBuffers[currentFrame], 0));

		VkCommandBufferBeginInfo commandBufferInfo = vks::initializers::commandBufferBeginInfo();
		VkClearValue clearValues[2]{};
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = frameBuffers[imageIndex];
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		VkCommandBuffer commandBuffer = commandBuffers[currentFrame];
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &commandBufferInfo));

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,1, &uniformBuffers[currentFrame].descriptorSet, 0, nullptr);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkDeviceSize offsets[1]{ 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, indices.count, 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo = vks::initializers::submitInfo();
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];
		submitInfo.pWaitDstStageMask = &waitStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[imageIndex];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentFrame]));

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[imageIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain.swapChain;
		presentInfo.pImageIndices = &imageIndex;
		result = vkQueuePresentKHR(queue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			windowResize();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Could not present the image to the swap chain!");
		}

		currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES;
	}
};

VULKAN_EXAMPLE_MAIN()
