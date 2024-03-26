/*
* Vulkan Example - glTF scene loading and rendering
*
* Copyright (C) 2020-2023 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/*
 * Shows how to load and display a simple scene from a glTF file
 * Note that this isn't a complete glTF loader and only basic functions are shown here
 * This means no complex materials, no animations, no skins, etc.
 * For details on how glTF 2.0 works, see the official spec at https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * Other samples will load models using a dedicated model loader with more features (see base/VulkanglTFModel.hpp)
 *
 * If you are looking for a complete glTF implementation, check out https://github.com/SaschaWillems/Vulkan-glTF-PBR/
 */

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#include "tiny_gltf.h"

#include "vulkanexamplebase.h"

class VulkanExample : public VulkanExampleBase
{
public:
	int32_t furRenderMethod = 0;
	int32_t furModel = 0;
	
	bool wireframe = false;

	float furLength = 0.2f;
	int furLayerNum = 32;
	float furDensity = 100.0f;
	float furAttenuation = 3.0f;
		
	// Setup vertices// Vertex layout used in this example
	struct Vertex {
		float position[3];
		float normal[3];
		float uv[2];
		float color[3];
	};
	
	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
		int32_t count;
	} planeVertices, planeIndices, sphereVertices, sphereIndices;

	struct ShaderData {
		vks::Buffer buffer;
		struct Values {
			glm::mat4 projection;
			glm::mat4 model;
			glm::vec4 lightPos = glm::vec4(5.0f, 5.0f, -5.0f, 1.0f);
			glm::vec4 viewPos;
		} values;
	} shaderData;
	
	struct FurData {
		vks::Buffer buffer;
		struct Values {
			float furLen = 0.2f;
			int furLayers = 32;
		} values;
	} furData;

	enum FurRenderMethod {
		multi_draw_shell = 0,
		geom_shell = 1,
		geom_shell_fin = 2,
		tess = 3,
		max,
	};
	std::array<VkPipeline, 4> pipelines = {
		VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
	};

	std::array<VkPipelineLayout, 4> pipelineLayouts = {
		VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
	};
	
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };	
	struct DescriptorSetLayouts {
		VkDescriptorSetLayout matrices{ VK_NULL_HANDLE };
	} descriptorSetLayouts;

	VkDescriptorPool descriptorPool2{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout2{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet2{ VK_NULL_HANDLE };

	VulkanExample() : VulkanExampleBase()
	{
		title = "Fur rendering";
		camera.type = Camera::CameraType::firstperson;
		camera.flipY = true;
		camera.setPosition(glm::vec3(0.0f, 1.0f, -2.0f));
		camera.setRotation(glm::vec3(-30.0f, 0.0f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.01f, 256.0f);
	}

	~VulkanExample()
	{
		if (device)
		{
			for (size_t i =0; i < FurRenderMethod::max; ++i)
			{
				if (i < pipelines.size() && pipelines[i] != VK_NULL_HANDLE)
				{
					vkDestroyPipeline(device, pipelines[i], nullptr);
				}
				if (i < pipelineLayouts.size() && pipelineLayouts[i] != VK_NULL_HANDLE)
				{
					vkDestroyPipelineLayout(device, pipelineLayouts[i], nullptr);
				}
			}
			
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.matrices, nullptr);
			
			shaderData.buffer.destroy();
		}
	}

	virtual void getEnabledFeatures()
	{
		// Fill mode non solid is required for wireframe display
		if (deviceFeatures.fillModeNonSolid) {
			enabledFeatures.fillModeNonSolid = VK_TRUE;
		};
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		const VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		const VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);

		for (size_t i = 0; i < drawCmdBuffers.size(); ++i)
		{			
			renderPassBeginInfo.framebuffer = frameBuffers[i];
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
			// Bind scene matrices descriptor to set 0
			if (0 <= furRenderMethod || furRenderMethod < pipelines.size())
			{
				if (pipelineLayouts[furRenderMethod] == VK_NULL_HANDLE)
					vks::tools::exitFatal("Fur buildCommandBuffers fail furRenderMethod:" + std::to_string(furRenderMethod) +
					" pipelineLayout is null:" + std::to_string(pipelineLayouts[furRenderMethod] == VK_NULL_HANDLE), -1);
				if (pipelines[furRenderMethod] == VK_NULL_HANDLE)
					vks::tools::exitFatal("Fur buildCommandBuffers fail furRenderMethod:" + std::to_string(furRenderMethod) +
					" pipelines is null:" + std::to_string(pipelines[furRenderMethod] == VK_NULL_HANDLE) , -1);
			}
			VkPipelineLayout pipelineLayout = pipelineLayouts[furRenderMethod];

			if (furModel == 0)// plane
			{
				VkDeviceSize offsets[1] = { 0 };
				vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &planeVertices.buffer, offsets);
				vkCmdBindIndexBuffer(drawCmdBuffers[i], planeIndices.buffer, 0, VK_INDEX_TYPE_UINT32);
			}
			else if (furModel == 1)// sphere
			{
				
			}
			else// suzanne.gltf model
			{
				
			}

			if (furRenderMethod == FurRenderMethod::multi_draw_shell)
			{
				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[furRenderMethod]);
				
				vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0
					, sizeof(float), &furLength);
				vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float) * 2
					, sizeof(float), &furDensity);
				vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float) * 3
					, sizeof(float), &furAttenuation);
			
				for (int32_t j= 0; j < furLayerNum ; ++j)
				{
					float layerRatio = static_cast<float>(j) / static_cast<float>(furLayerNum);
					vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float)
						, sizeof(float), &layerRatio);
					vkCmdDrawIndexed(drawCmdBuffers[i], planeIndices.count, 1, 0, 0, 1);
				}
			}
			else if (furRenderMethod == FurRenderMethod::geom_shell)
			{
				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet2, 0, nullptr);
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[furRenderMethod]);
				// vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_GEOMETRY_BIT, 0
				// 	, sizeof(float), &furLength);
				// float fFurLayerNum = static_cast<float>(furLayerNum);
				// vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_GEOMETRY_BIT, sizeof(float)
				// 	, sizeof(float), &fFurLayerNum);
				vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0
					, sizeof(float), &furDensity);
				vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float)
					, sizeof(float), &furAttenuation);
			
				vkCmdDrawIndexed(drawCmdBuffers[i], planeIndices.count, 1, 0, 0, 1);
			}

			drawUI(drawCmdBuffers[i]);
			vkCmdEndRenderPass(drawCmdBuffers[i]);
			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		createPlane();
		createSphere();
		loadModel();
	}
	
	void createPlane()
	{
		std::vector<Vertex> vertexBuffer{
				{ {  1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
				{ {  1.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
				{ { -1.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
				{ {  -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }
		};
		// Setup indices
		std::vector<uint32_t> indexBuffer{ 0, 1, 2, 2, 3, 0 };
		
		size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
		size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);

		planeVertices.count = vertexBuffer.size();
		planeIndices.count = indexBuffer.size();

		struct StagingBuffer {
			VkBuffer buffer;
			VkDeviceMemory memory;
		} vertexStaging, indexStaging;

		// Create host visible staging buffers (source)
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBufferSize,
			&vertexStaging.buffer,
			&vertexStaging.memory,
			vertexBuffer.data()));
		// Index data
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBufferSize,
			&indexStaging.buffer,
			&indexStaging.memory,
			indexBuffer.data()));
		
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBufferSize,
			&planeVertices.buffer,
			&planeVertices.memory));
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indexBufferSize,
			&planeIndices.buffer,
			&planeIndices.memory));

		// Copy data from staging buffers (host) do device local buffer (gpu)
		VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy copyRegion = {};

		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(
			copyCmd,
			vertexStaging.buffer,
			planeVertices.buffer,
			1,
			&copyRegion);

		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(
			copyCmd,
			indexStaging.buffer,
			planeIndices.buffer,
			1,
			&copyRegion);

		vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

		// Free staging resources
		vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
		vkFreeMemory(device, vertexStaging.memory, nullptr);
		vkDestroyBuffer(device, indexStaging.buffer, nullptr);
		vkFreeMemory(device, indexStaging.memory, nullptr);
	}

	void createSphere()
	{
		
	}

	void loadModel()
	{
		
	}

	void setupDescriptors()
	{
		// DescriptorPool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// DescriptorSetLayout
		VkDescriptorSetLayoutBinding setLayoutBinding = vks::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(&setLayoutBinding, 1);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.matrices));

		// DescriptorSet
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool,
			&descriptorSetLayouts.matrices, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		
		VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(descriptorSet,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &shaderData.buffer.descriptor);
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

		// Geometry shader shell rendering descriptor
		{
			// DescriptorPool
			poolSizes = {
				vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			};
			descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
			VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool2));

			// DescriptorSetLayout
			std::array<VkDescriptorSetLayoutBinding,2> setLayoutBindings = {
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					VK_SHADER_STAGE_GEOMETRY_BIT, 0),
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					VK_SHADER_STAGE_GEOMETRY_BIT, 1)
				};
			descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), 2);
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout2));

			// DescriptorSet
			allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool2,
				&descriptorSetLayout2, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet2));
		
			std::array<VkWriteDescriptorSet, 2> writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSet2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					0, &shaderData.buffer.descriptor),
				vks::initializers::writeDescriptorSet(descriptorSet2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					1, &furData.buffer.descriptor)
				};
			
			vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(),
				0, nullptr);
		}
	}

	void preparePipelines()
	{
		// Layout
		// The pipeline layout uses both descriptor sets (set 0 = matrices, set 1 = material)
		std::array<VkDescriptorSetLayout, 1> setLayouts = { descriptorSetLayouts.matrices };
		VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
		// We will use push constants to push the local matrices of a primitive to the vertex shader
		std::array<VkPushConstantRange, 2> pushConstantRanges1 = {
			vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, 0),
			vks::initializers::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float) * 2, sizeof(float) * 2)
			};
		// Push constant ranges are part of the pipeline layout
		pipelineLayoutCI.pushConstantRangeCount = pushConstantRanges1.size();
		pipelineLayoutCI.pPushConstantRanges = pushConstantRanges1.data();
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayouts[multi_draw_shell]));

		// Pipeline
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentStateCI = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentStateCI);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		const std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		// Vertex input bindings and attributes
		const std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			vks::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
		};
		const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),	// Location 0: Position
			vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),// Location 1: Normal
			vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, uv)),	// Location 2: Texture coordinates
			vks::initializers::vertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)),	// Location 3: Color
		};
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputStateCI.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputStateCI.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

		const std::array<VkPipelineShaderStageCreateInfo, 2> shader1Stages = {
			loadShader(getShadersPath() + "fur/mesh1.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "fur/mesh1.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayouts[multi_draw_shell], renderPass, 0);
		pipelineCI.pVertexInputState = &vertexInputStateCI;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shader1Stages.size());
		pipelineCI.pStages = shader1Stages.data();

		// Multiple drawing shell rendering pipeline
		{
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines[multi_draw_shell]));
		}
		
		// Geometry shader shell rendering pipeline
		{
			setLayouts = { descriptorSetLayout2 };
			pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
			std::array<VkPushConstantRange, 1> pushConstantRanges2 = {
				vks::initializers::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float) * 2, 0)
				};
			pipelineLayoutCI.pushConstantRangeCount = pushConstantRanges2.size();
			pipelineLayoutCI.pPushConstantRanges = pushConstantRanges2.data();
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayouts[geom_shell]));

			pipelineCI.layout = pipelineLayouts[geom_shell];			
			const std::array<VkPipelineShaderStageCreateInfo, 3> shader2Stages = {
				loadShader(getShadersPath() + "fur/mesh2.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
				loadShader(getShadersPath() + "fur/mesh2test.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT),
				loadShader(getShadersPath() + "fur/mesh2test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
			};
			pipelineCI.stageCount = static_cast<uint32_t>(shader2Stages.size());
			pipelineCI.pStages = shader2Stages.data();
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines[geom_shell]));
		}

		// Geometry shader shell and fin rendering pipeline
		{
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayouts[geom_shell_fin]));
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines[geom_shell_fin]));
		}

		// Tessellation shader rendering pipeline
		{
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayouts[tess]));
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines[tess]));
		}
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&shaderData.buffer, sizeof(shaderData.values)));
		// Map persistent
		VK_CHECK_RESULT(shaderData.buffer.map());

		// Uniform buffer for fur setting
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&furData.buffer, sizeof(furData.values)));
		// VK_CHECK_RESULT(furData.buffer.map());
	}

	void updateUniformBuffers()
	{
		shaderData.values.projection = camera.matrices.perspective;
		shaderData.values.model = camera.matrices.view;
		shaderData.values.viewPos = camera.viewPos;
		memcpy(shaderData.buffer.mapped, &shaderData.values, sizeof(shaderData.values));
		
		// memcpy(furData.buffer.mapped, &furData.values, sizeof(furData.values));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		updateUniformBuffers();
		renderFrame();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings"))
		{
			if (overlay->comboBox("Fur render method", &furRenderMethod,
				{"Multiple drawing shell", "Geometry shader shell", "Geometry shader shell and fin", "Tessellation shader"}))
			{
				buildCommandBuffers();
			}
			if (overlay->comboBox("Fur model", &furModel,
				{"Plane", "Sphere", "suzanne"}))
			{
				buildCommandBuffers();
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()
