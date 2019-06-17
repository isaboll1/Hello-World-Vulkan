#pragma once
//Vulkan Header
#include <vulkan/vulkan.h>
//SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <array>

using namespace std;

class VK_Renderer {
public:

	VkDevice device_context;
	VkQueue queue;
	uint32_t family_i;

	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	vector<VkImage> swapchain_buffers;
	vector<VkImageView> swapchain_buffer_view;
	uint32_t active_swapchain_id = UINT32_MAX;
	VkSemaphore present = VK_NULL_HANDLE;
	VkSemaphore render = VK_NULL_HANDLE;
	vector<VkFence> swapchain_available;
	VkImage depth_stencil_buffer = VK_NULL_HANDLE;
	VkDeviceMemory depth_stencil_buffer_memory = VK_NULL_HANDLE;
	VkImageView depth_stencil_buffer_view = VK_NULL_HANDLE;
	VkRenderPass render_pass = VK_NULL_HANDLE;
	vector<VkFramebuffer> framebuffers;
	VkViewport viewport = {};
	VkRect2D scissor = {};
	VkPhysicalDeviceFeatures gpu_features;
	VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	int render_width;
	int render_height;

	VK_Renderer(SDL_Window * window, int width, int height);
	~VK_Renderer();

	//Rendering Functions (you can do all this stuff manually in your game loop using the public variables available)
	void AcquireNextSwapchain();
	void BeginRenderPresent(vector<VkCommandBuffer> command_buffers);
	//~Rendering Functions

private:

	//Variables
	VkResult result;
	VkInstance instance;
	VkSurfaceKHR window_surface;
	VkSwapchainKHR old_swapchain = nullptr;
	uint32_t swapchain_buffer_count = 2;
	VkSurfaceCapabilitiesKHR surface_caps {};
	VkFormat depth_buffer_format = VK_FORMAT_UNDEFINED;
	bool stencil_support = false;
	bool present_mode_set = true;
	VkSurfaceFormatKHR surface_format {};
	VkPhysicalDevice device;
	VkPhysicalDeviceProperties device_properties {};
	VkPhysicalDeviceMemoryProperties device_memory_info {};
	vector<const char *> device_extensions {};
	vector<const char *> instance_layers {};
	vector<const char *> instance_extensions;
	VkDebugReportCallbackCreateInfoEXT debug_create_info = {};

	//Functions
	void GetSDLWindowInfo(SDL_Window * window);
	void InitInstance();
	void DestroyInstance();

	void CreateDeviceContext();
	void DestroyDeviceContext();

	void CreateSurface(SDL_Window * window);
	void DestroySurface();

	void CreateSwapchain();
	void DestroySwapchain();

	void CreateSwapchainImages();
	void DestroySwapchainImages();

	void CreateDepthStencilImage();
	void DestroyDepthStencilImage();

	void CreateRenderPass();
	void DestroyRenderPass();

	void CreateFramebuffers();
	void DestroyFramebuffers();

	void StartSynchronizations();
	void EndSynchronizations();


#ifdef VK_DEBUG
	VkDebugReportCallbackEXT debug_report;
	void SetupDebug();
	void InitDebug();
	void DestroyDebug();
#endif

};
