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

	std::vector<VkSemaphore> presentCompleteSemaphores{};
	std::vector<VkSemaphore> renderCompleteSemaphores{};

	VkCommandPool commandPool{ VK_NULL_HANDLE };
	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{};
	std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{};

	uint32_t currentFrame{ 0 };

public:
	VulkanExample() : VulkanExampleBase()
	{
		title = "Triangle";
	}

	~VulkanExample() override
	{
		// todo
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

	void createSynchronizationPrimitives()
	{
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++)
		{
			VkFenceCreateInfo fenceCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT
			};
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &waitFences[i]));
		}

		presentCompleteSemaphores
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
		vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores, VK_NULL_HANDLE, imageIndex);
	}

};

VULKAN_EXAMPLE_MAIN()
