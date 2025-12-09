#include "vulkanexamplebase.h"

class VulkanExample : public VulkanExampleBase
{
public:
	VulkanExample() : VulkanExampleBase()
	{
		// todo
	}

	~VulkanExample() override
	{
		// todo
	}

	void prepare() override
	{
		// todo
		VulkanExample::prepare();
		//createSynchronizationPrimitives();
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
		std::cout << "Hello Triangle" << std::endl;
		if (!prepared)
		{
			return;
		}
	}

};

VULKAN_EXAMPLE_MAIN()