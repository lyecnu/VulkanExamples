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
		camera.type = Camera::CameraType::lookat;
	}

	~VulkanExample() override
	{
		// todo
	}

	void createSynchronizationPrimitives()
	{
		for (auto& fence : waitFences)
		{
			VkFenceCreateInfo fenceCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT
			};
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
		}

		for (auto& semaphore : presentCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			};
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore));
		}

		renderCompleteSemaphores.resize(swapChain.images.size());
		for (auto& semaphore : renderCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			};
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore));
		}
	}

	void createCommandBuffers()
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = swapChain.queueNodeIndex
		};
		VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));

		VkCommandBufferAllocateInfo commandBufferAllocateInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(commandBuffers.size())
		};
	}

	void createVertexBuffer()
	{
		std::vector<Vertex> vertexBuffer =
		{
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		};

		VkBufferCreateInfo vertexBufferCreateInfo
		{

		};

		vkCreateBuffer(device, &vertexBufferCreateInfo, nullptr, &vertices.buffer);
			UniformBuffer
	}


	void prepare() override
	{
		VulkanExampleBase::prepare();
		createSynchronizationPrimitives();
		//createCommandBuffers();
		//createVertexBuffer();
		//createUniformBuffers();
		//createDescriptorSetLayout();
		//createDescriptorPool();
		//createDescriptorSets();
		//createPipelines();
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
		memccpy(uniformBuffers[currentFrame].mapped, &shaderData, sizeof(ShaderData), sizeof(ShaderData));

	}

};

VULKAN_EXAMPLE_MAIN()
