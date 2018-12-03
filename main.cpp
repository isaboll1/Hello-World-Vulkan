#include "Renderer.h"
#include "Helper.h"
#include <cmath>

constexpr double PI = 3.14159265358979323846;
constexpr double CIRCLE_RAD = PI * 2;
constexpr double CIRCLE_THIRD = CIRCLE_RAD / 3.0;
constexpr double CIRCLE_THIRD_1 = 0;
constexpr double CIRCLE_THIRD_2 = CIRCLE_THIRD;
constexpr double CIRCLE_THIRD_3 = CIRCLE_THIRD * 2;

int main(int argc, char** argv) // Equivalent to WinMain, this initializes SDL2
{
	
	SDL_Init(SDL_INIT_VIDEO);   // This activates the specified SDL2 subsystem;

	//Forward Declarations
	SDL_Window * window;    //  window handle
	SDL_Event event;		//   event handle
	int WIDTH = 640, HEIGHT = 480;


	window = SDL_CreateWindow("Hello Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		WIDTH, HEIGHT, SDL_WINDOW_VULKAN|SDL_WINDOW_ALLOW_HIGHDPI);


	VK_Renderer renderer = VK_Renderer(window, WIDTH, HEIGHT);    //  vulkan renderer handle

	//Variables
	float deltaTime;
	bool running = true;
	float rotator = 0.0f;

	//Rendering
	VkCommandPool command_pool = VK_NULL_HANDLE;
	VkCommandPoolCreateInfo pool_create_info{};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT|VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_create_info.queueFamilyIndex = renderer.family_i;
	vkCreateCommandPool(renderer.device_context, &pool_create_info, nullptr, &command_pool);

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo command_buffer_info;
	command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_info.pNext = nullptr;
	command_buffer_info.commandPool = command_pool;
	command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_info.commandBufferCount = 1;
	vkAllocateCommandBuffers(renderer.device_context, &command_buffer_info, &command_buffer);
	
	VkSemaphore semaphore = VK_NULL_HANDLE;
	
	VkSemaphoreCreateInfo semaphore_create_info {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(renderer.device_context, &semaphore_create_info, nullptr, &semaphore);

	//Triangle Creation
	{
		
		VkShaderModule vertex_shader = CreateShaderModule(ReadShaderFile("shaders/triangle.vert.spv"), renderer.device_context);
		VkShaderModule frag_shader = CreateShaderModule(ReadShaderFile("shaders/triangle.frag.spv"), renderer.device_context);

		VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vertex_shader;
		vert_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_shader;
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 0;
		vertex_input_info.pVertexBindingDescriptions = nullptr;
		vertex_input_info.vertexAttributeDescriptionCount = 0;
		vertex_input_info.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
		input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly.primitiveRestartEnable = VK_FALSE;

		//Viewport and Scissor
		renderer.viewport.x = 0.0f;
		renderer.viewport.y = 0.0f;
		renderer.viewport.width = (float)WIDTH;
		renderer.viewport.height = (float)HEIGHT;
		renderer.viewport.minDepth = 0.0f;
		renderer.viewport.maxDepth = 1.0f;

		renderer.scissor.offset = { 0,0 };
		renderer.scissor.extent.width = WIDTH;
		renderer.scissor.extent.height = HEIGHT;
		VkPipelineViewportStateCreateInfo viewport_state = {};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &renderer.viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &renderer.scissor;


		//Rasterizer
		renderer.rasterizer.depthClampEnable = VK_FALSE;
		renderer.rasterizer.rasterizerDiscardEnable = VK_FALSE;
		renderer.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		renderer.rasterizer.lineWidth = 1.0f;
		renderer.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		renderer.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		renderer.rasterizer.depthBiasEnable = VK_FALSE;
		renderer.rasterizer.depthBiasConstantFactor = 0.0f;
		renderer.rasterizer.depthBiasClamp = 0.0f;
		renderer.rasterizer.depthBiasSlopeFactor = 0.0f;

		//Multisampling
		renderer.multisampling.sampleShadingEnable = VK_FALSE;
		renderer.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		renderer.multisampling.minSampleShading = 1.0f;
		renderer.multisampling.pSampleMask = nullptr;
		renderer.multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		renderer.multisampling.alphaToOneEnable = VK_FALSE;

		//Depth and Stencil Tests
		VkPipelineDepthStencilStateCreateInfo depth_stencil_state = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		depth_stencil_state.depthTestEnable = VK_FALSE;
		depth_stencil_state.stencilTestEnable = VK_FALSE;

		//Color Blending - (The fragment shader's color needs to be combined with a color in a framebuffer)
		VkPipelineColorBlendAttachmentState color_attachment = {};
		color_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_attachment.blendEnable = VK_TRUE;
		color_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blending = {};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &color_attachment;
		color_blending.blendConstants[0] = 0.0f;
		color_blending.blendConstants[1] = 0.0f;
		color_blending.blendConstants[2] = 0.0f;
		color_blending.blendConstants[3] = 0.0f;

		//Dynamic State
		VkDynamicState dynamic_states[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH,
		};

		VkPipelineDynamicStateCreateInfo dynamic_state = {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = 2;
		dynamic_state.pDynamicStates = dynamic_states;

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 0;
		pipeline_layout_info.pSetLayouts = nullptr;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		vkCreatePipelineLayout(renderer.device_context, &pipeline_layout_info, nullptr, &renderer.pipeline_layout);

		VkGraphicsPipelineCreateInfo pipeline_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pDepthStencilState = &depth_stencil_state;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &renderer.rasterizer;
		pipeline_info.pMultisampleState = &renderer.multisampling;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = &dynamic_state;
		pipeline_info.layout = renderer.pipeline_layout;
		pipeline_info.renderPass = renderer.render_pass;
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_info.basePipelineIndex = -1;

		vkCreateGraphicsPipelines(renderer.device_context, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &renderer.pipeline);

		vkDestroyShaderModule(renderer.device_context, vertex_shader, nullptr);
		vkDestroyShaderModule(renderer.device_context, frag_shader, nullptr);
	}

	//FPS Defines
	double lastTime = SDL_GetTicks() / 1000;
	float lastFrame = SDL_GetTicks();
	int nmbf = 0;
	bool PrintFPS = true;

	while (running) {
		//Game Logic
		if (PrintFPS == true) {
			double currentTime = SDL_GetTicks() / 1000;
			nmbf++;
			if (currentTime - lastTime >= 1.0) {
				printf("\n Framerate: %f \n", 1.0 * (double)nmbf);
				nmbf = 0;
				lastTime += 1.0;
			}
		}

		// per frame calculations
		float currentFrame = SDL_GetTicks();
		deltaTime = (currentFrame - lastFrame) / 1000;
		lastFrame = currentFrame;

		//OS Events
		while (SDL_PollEvent(&event))
			if (event.type == SDL_QUIT) {
				running = false;
				break;
			}

		//Begin Rendering Stage
		renderer.BeginRendering(nullptr);

		//Record to Command Buffers
		VkCommandBufferBeginInfo command_buffer_begin_info{};
		command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

		//sending info to the render pass as to how it renders (clear and depth color, the framebuffer that the drawing commands are applied to)
		VkRect2D render_area{};
		render_area.offset = { 0,0 };
		render_area.extent.width = WIDTH;
		render_area.extent.height = HEIGHT;

		rotator += 0.001;

		array<VkClearValue, 2> color_values{};
		color_values[0].depthStencil = { 0.0f, 0 }; // {depth, stencil} indexed by attachments
		color_values[1].color.float32[0] = sin(rotator + CIRCLE_THIRD_1) * 0.5 + 0.5; //r
		color_values[1].color.float32[1] = sin(rotator + CIRCLE_THIRD_2)* 0.5 + 0.5; //g
		color_values[1].color.float32[2] = sin(rotator + CIRCLE_THIRD_3)* 0.5 + 0.5; //b
		color_values[1].color.float32[3] = 1.0f; //a

		VkRenderPassBeginInfo render_pass_begin_info{};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = renderer.render_pass;
		render_pass_begin_info.framebuffer = renderer.framebuffers[renderer.active_swapchain_id];
		render_pass_begin_info.renderArea = render_area;
		render_pass_begin_info.clearValueCount = color_values.size();
		render_pass_begin_info.pClearValues = color_values.data();

		vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		//at this point drawing commands can begin...

		vkCmdSetViewport(command_buffer, VK_NULL_HANDLE, 1, &renderer.viewport);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);
		vkCmdDraw(command_buffer, 3, 1, 0, 0);

		//...up until this point
		vkCmdEndRenderPass(command_buffer);
		vkEndCommandBuffer(command_buffer);

		//Submit Command Buffers
		VkSubmitInfo submit_info {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = nullptr;
		submit_info.pWaitDstStageMask = nullptr;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &semaphore;

		vkQueueSubmit(renderer.queue, 1, &submit_info, nullptr);
	
		//End Rendering Stage and Present Swapchain Buffers
		renderer.PresentStopRendering({ semaphore });

	}
	vkQueueWaitIdle(renderer.queue);
	vkDestroySemaphore(renderer.device_context, semaphore, nullptr);
	vkDestroyCommandPool(renderer.device_context, command_pool, nullptr);
	vkDestroyPipeline(renderer.device_context, renderer.pipeline, nullptr);
	vkDestroyPipelineLayout(renderer.device_context, renderer.pipeline_layout, nullptr);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
