#include "Renderer.h"
#ifdef _WIN32
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0'  processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

VK_Renderer::VK_Renderer(SDL_Window * window, int width, int height)
{
	instance_extensions;
	render_width = width;
	render_height = height;

	GetSDLWindowInfo(window);
	InitInstance();
	CreateDeviceContext();
	CreateSurface(window);
	CreateSwapchain();
	CreateSwapchainImages();
	CreateDepthStencilImage();
	CreateRenderPass();
	CreateFramebuffers();

	StartSynchronizations();

}

VK_Renderer::~VK_Renderer()
{
	vkQueueWaitIdle(queue);

	EndSynchronizations();

	DestroyFramebuffers();
	DestroyRenderPass();
	DestroyDepthStencilImage();
	DestroySwapchainImages();
	DestroySwapchain();
	DestroySurface();
	DestroyDeviceContext();
	DestroyInstance();

}

void VK_Renderer::GetSDLWindowInfo(SDL_Window * window) //Get the necessary information from the SDL2 window handle for Vulkan support.
{
	uint32_t extension_count = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &extension_count, NULL);
	vector<const char *> names;
	instance_extensions.resize(extension_count);
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, instance_extensions.data())){
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, SDL_GetError());
		return;
	}
	device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}


void VK_Renderer::InitInstance()
{
#ifdef VK_DEBUG
	SetupDebug();
#endif // VK_DEBUG add Extensions

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 14);
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "Apparatus Engine";

	VkInstanceCreateInfo InstanceInfo {};
	InstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstanceInfo.pApplicationInfo = &appInfo;
	InstanceInfo.enabledLayerCount = instance_layers.size();
	InstanceInfo.ppEnabledLayerNames = instance_layers.data();
	InstanceInfo.enabledExtensionCount = 2;
	InstanceInfo.ppEnabledExtensionNames = instance_extensions.data();
	

	result = vkCreateInstance(&InstanceInfo, nullptr, &instance);
	if (result != VK_SUCCESS) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "Create Instance Failed");
	}

#ifdef VK_DEBUG
	InitDebug();
#endif

}

void VK_Renderer::DestroyInstance()
{
#ifdef VK_DEBUG
	DestroyDebug();
#endif  //VK_DEBUG Destroy Debug First

	vkDestroyInstance(instance, nullptr);
	instance = nullptr;

}


void VK_Renderer::CreateDeviceContext()
{
	//Get the Physical GPU
	uint32_t gpu_number = 0;
	vkEnumeratePhysicalDevices(instance, &gpu_number, nullptr);
	vector<VkPhysicalDevice> devices(gpu_number);
	vkEnumeratePhysicalDevices(instance, &gpu_number, devices.data());
	device = devices[0];


	//Get The Physical GPU info
	SDL_memset(&device_properties, 0, sizeof device_properties);

	vkGetPhysicalDeviceProperties(device, &device_properties); //General GPU info
	vkGetPhysicalDeviceMemoryProperties(device, &device_memory_info); //GPU Memory Info

	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Driver Version: %i\n", device_properties.driverVersion);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Device Name: %s\n", device_properties.deviceName);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Device Type: %p\n", device_properties.deviceType);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Vulkan Version: %d. %d. %d\n",
		(device_properties.apiVersion >> 22) & 0x3FF,
		(device_properties.apiVersion >> 12) & 0x3FF,
		(device_properties.apiVersion & 0xFFF));
	printf("\n");


	//Get the Physical GPU Supported Operations
	uint32_t family_number = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_number, nullptr);
	vector<VkQueueFamilyProperties> familyProperties(family_number);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_number, familyProperties.data());

	vkGetPhysicalDeviceFeatures(device, &gpu_features);

	bool gr = false;

	for (int i = 0; i < family_number; i++) {
		if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			gr = true;
			family_i = i;
			break;
	}

	if (!gr) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "Graphics operations not supported");
	}


	//Create the Device Context
	float queuePriorities[] = { 1.0f };
	VkDeviceQueueCreateInfo deviceQueueInfo = {};
	deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueInfo.queueFamilyIndex = family_i;
	deviceQueueInfo.queueCount = 1;
	deviceQueueInfo.pQueuePriorities = queuePriorities;

	VkDeviceCreateInfo contextInfo = {};
	contextInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	contextInfo.queueCreateInfoCount = 1;
	contextInfo.enabledExtensionCount = device_extensions.size();
	contextInfo.ppEnabledExtensionNames = device_extensions.data();
	contextInfo.pQueueCreateInfos = &deviceQueueInfo;

	result = vkCreateDevice(device, &contextInfo, nullptr, &device_context);

	if (result != VK_SUCCESS) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "Could not create a Device Context!");
	}

	vkGetDeviceQueue(device_context, family_i, 0, &queue);
}

void VK_Renderer::DestroyDeviceContext()
{

	vkDestroyDevice(device_context, nullptr);
	device_context = nullptr;
}

void VK_Renderer::CreateSurface(SDL_Window * window) //Get the window surface (this process usually is platform specific, but with SDL2 we can have it be platform agnostic)
{
	SDL_Vulkan_CreateSurface(window, instance, &window_surface);

	VkBool32 WSI_supported = false;
	result = vkGetPhysicalDeviceSurfaceSupportKHR(device, family_i, window_surface, &WSI_supported);
	if (!WSI_supported) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "Could not not create window surface for Vulkan!");
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, window_surface, &surface_caps);

	if (surface_caps.currentExtent.width < UINT32_MAX) {
		render_width = surface_caps.currentExtent.width;
		render_height = surface_caps.currentExtent.height;
	}

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, window_surface, &format_count, nullptr);
	if (format_count == 0) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "Could not get surface informaion for Vulkan Swapchain!");
	}
	vector<VkSurfaceFormatKHR> formats(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, window_surface, &format_count, formats.data());
	if (formats[0].format == VK_FORMAT_UNDEFINED) {
		surface_format.format = VK_FORMAT_R8G8B8A8_UNORM;
		surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	} else { surface_format = formats[0]; }

}

void VK_Renderer::DestroySurface()
{
	vkDestroySurfaceKHR(instance, window_surface, nullptr);
}

void VK_Renderer::CreateSwapchain() //Initialize the Swapchain Object
{
	if (surface_caps.maxImageCount){
		if (swapchain_buffer_count > surface_caps.maxImageCount) {
			swapchain_buffer_count = surface_caps.maxImageCount;
		}
		else if (swapchain_buffer_count < surface_caps.minImageCount) {
			swapchain_buffer_count = surface_caps.minImageCount;
		}
	} else swapchain_buffer_count = surface_caps.minImageCount + 1;

	 //present mode is essentially the type of vertical syncronization mode.
	uint32_t present_mode_count = 0;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, window_surface, &present_mode_count, nullptr);
	vector<VkPresentModeKHR> swap_modes(present_mode_count);
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, window_surface, &present_mode_count, swap_modes.data());
	if (!present_mode_set) {
		present_mode = VK_PRESENT_MODE_FIFO_KHR;
		for (auto mode : swap_modes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) { present_mode = mode; }
		}
		present_mode_set = true;
	}

	VkSwapchainCreateInfoKHR swapchain_info {};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = window_surface;
	swapchain_info.minImageCount = swapchain_buffer_count;
	swapchain_info.imageFormat = surface_format.format;
	swapchain_info.imageColorSpace = surface_format.colorSpace;
	swapchain_info.imageExtent.width = render_width;
	swapchain_info.imageExtent.height = render_height;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.queueFamilyIndexCount = 0;
	swapchain_info.pQueueFamilyIndices = nullptr;
	swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = present_mode;
	swapchain_info.clipped = VK_TRUE;
	swapchain_info.oldSwapchain = old_swapchain;

	result = vkCreateSwapchainKHR(device_context, &swapchain_info, nullptr, &swapchain);

	result = vkGetSwapchainImagesKHR(device_context, swapchain, &swapchain_buffer_count, nullptr);
}

void VK_Renderer::DestroySwapchain()
{
	vkDestroySwapchainKHR(device_context, swapchain, nullptr);
}

void VK_Renderer::CreateSwapchainImages() //Create Buffers for the Swapchain
{
	swapchain_buffers.resize(swapchain_buffer_count);
	swapchain_buffer_view.resize(swapchain_buffer_count);

	result = vkGetSwapchainImagesKHR(device_context, swapchain, &swapchain_buffer_count, swapchain_buffers.data());

	for (uint32_t i = 0; i < swapchain_buffer_count; ++i) {
		VkImageViewCreateInfo swapchain_view_info {};
		swapchain_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		swapchain_view_info.image = swapchain_buffers[i];
		swapchain_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		swapchain_view_info.format = surface_format.format;
		swapchain_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		swapchain_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		swapchain_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		swapchain_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		swapchain_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		swapchain_view_info.subresourceRange.baseMipLevel = 0;
		swapchain_view_info.subresourceRange.levelCount = 1;
		swapchain_view_info.subresourceRange.baseArrayLayer = 0;
		swapchain_view_info.subresourceRange.layerCount = 1;

		result = vkCreateImageView(device_context, &swapchain_view_info, nullptr, &swapchain_buffer_view[i]);

	}
}

void VK_Renderer::DestroySwapchainImages() //Destroy Swapchain Buffers
{
	for (auto view : swapchain_buffer_view) {
		vkDestroyImageView(device_context, view, nullptr);
	}
}

void VK_Renderer::CreateDepthStencilImage() //Create the Depth and Stencil Buffer Attachments for Swapchain Rendering
{
	//Check for the format of the Depth/Stencil Buffer
	vector<VkFormat> depth_formats {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT,VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM};
	for (auto format : depth_formats) {
		VkFormatProperties format_properties{};
		vkGetPhysicalDeviceFormatProperties(device, format, &format_properties);
		if (format_properties.optimalTilingFeatures &VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depth_buffer_format = format;
			break;
		}
	}

	if (depth_buffer_format == VK_FORMAT_UNDEFINED) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "Couldn't get depth buffer format!");
	}

	//Check the Buffer Aspect
	if ((depth_buffer_format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
		(depth_buffer_format == VK_FORMAT_D24_UNORM_S8_UINT) ||
		(depth_buffer_format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(depth_buffer_format == VK_FORMAT_S8_UINT)) {
		stencil_support = true;
	}

	//Create Depth Buffer
	VkImageCreateInfo image_create_info {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.flags = 0;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = depth_buffer_format;
	image_create_info.extent.width = render_width;
	image_create_info.extent.height = render_height;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
	image_create_info.pQueueFamilyIndices = nullptr;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	vkCreateImage(device_context, &image_create_info, nullptr, &depth_stencil_buffer);

	VkMemoryRequirements buffer_memory_req; //Get the required information about memory for the buffer
	vkGetImageMemoryRequirements(device_context, depth_stencil_buffer, &buffer_memory_req);

	uint32_t memory_index = UINT_FAST32_MAX;  //Index of GPU memory location
	auto propeties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; //application controlled property of the memory used with the GPU
	for (uint32_t i = 0; i < device_memory_info.memoryTypeCount; i++) {
		if (buffer_memory_req.memoryTypeBits & (1 << i)) {
			if ((device_memory_info.memoryTypes[i].propertyFlags & propeties) == propeties) {
				memory_index = i;
				break;
			}
		}
	}
	SDL_assert(memory_index != UINT32_MAX);

	VkMemoryAllocateInfo memory_info{}; //info about the buffer's memory
	memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_info.allocationSize = buffer_memory_req.size;
	memory_info.memoryTypeIndex = memory_index;

	vkAllocateMemory(device_context, &memory_info, nullptr, &depth_stencil_buffer_memory); //allocate memory for the Depth/Stencil Buffer
	vkBindImageMemory(device_context, depth_stencil_buffer, depth_stencil_buffer_memory, 0); //Bind the Depth/Stencil Buffer

	VkImageViewCreateInfo image_view_info {};
	image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.image = depth_stencil_buffer;
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format = depth_buffer_format;
	image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (stencil_support ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	image_view_info.subresourceRange.baseMipLevel = 0;
	image_view_info.subresourceRange.levelCount = 1;
	image_view_info.subresourceRange.baseArrayLayer = 0;
	image_view_info.subresourceRange.layerCount = 1;

	vkCreateImageView(device_context, &image_view_info, nullptr, &depth_stencil_buffer_view);
}

void VK_Renderer::DestroyDepthStencilImage()
{
	vkDestroyImageView(device_context, depth_stencil_buffer_view, nullptr);
	vkFreeMemory(device_context, depth_stencil_buffer_memory, nullptr);
	vkDestroyImage(device_context, depth_stencil_buffer, nullptr);
}

void VK_Renderer::CreateRenderPass() //Create the Render Pass Object, which is essentially used for the order of rendering (i.e forward or deferred)
{
	array<VkAttachmentDescription, 2> attachments {};
	{
		//attachments
		attachments[0].flags = 0;
		attachments[0].format = depth_buffer_format;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		attachments[1].flags = 0;
		attachments[1].format = surface_format.format;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		//extra attachments
		//~extra attachments
	}

	VkAttachmentReference sub_pass_depth_attachment {};
	sub_pass_depth_attachment.attachment = 0;
	sub_pass_depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	array<VkAttachmentReference, 1>sub_pass_color_attachments {}; // synonymous with (in glsl): layout(location = 0) out vec4 FinalColor
	{
		sub_pass_color_attachments[0].attachment = 1;
		sub_pass_color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//extra color attachments
		//~extra color attachments
	}

	array<VkSubpassDescription, 1> sub_passes {};
	{
		sub_passes[0].flags = 0;
		sub_passes[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		sub_passes[0].inputAttachmentCount = 0;
		sub_passes[0].pInputAttachments = nullptr;
		sub_passes[0].colorAttachmentCount = sub_pass_color_attachments.size();
		sub_passes[0].pColorAttachments = sub_pass_color_attachments.data();
		sub_passes[0].pResolveAttachments = nullptr;
		sub_passes[0].pDepthStencilAttachment = &sub_pass_depth_attachment;
		sub_passes[0].preserveAttachmentCount = 0;
		sub_passes[0].pPreserveAttachments = nullptr;
		//extra subpasses
		//~extra subpasses
	}

	VkRenderPassCreateInfo render_pass_info {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = attachments.size();
	render_pass_info.pAttachments = attachments.data();
	render_pass_info.subpassCount = sub_passes.size();
	render_pass_info.pSubpasses = sub_passes.data();
	render_pass_info.dependencyCount = 0;
	render_pass_info.pDependencies = nullptr;

	result = vkCreateRenderPass(device_context, &render_pass_info, nullptr, &render_pass);
}

void VK_Renderer::DestroyRenderPass()
{
	vkDestroyRenderPass(device_context, render_pass, nullptr);
}

void VK_Renderer::CreateFramebuffers() //create the framebuffers (which bind the image attachments to the renderpass)
{
	framebuffers.resize(swapchain_buffer_count);
	for (unsigned int i = 0; i < swapchain_buffer_count; i++) {

		array<VkImageView, 2> attachments{};
		attachments[0] = depth_stencil_buffer_view;
		attachments[1] = swapchain_buffer_view[i];
		//extra attachments
		//~extra attachments
		VkFramebufferCreateInfo framebuffer_info {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass;
		framebuffer_info.attachmentCount = attachments.size();
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = render_width;
		framebuffer_info.height = render_height;
		framebuffer_info.layers = 1;

		result = vkCreateFramebuffer(device_context, &framebuffer_info, nullptr, &framebuffers[i]);
	}
}

void VK_Renderer::DestroyFramebuffers()
{
	for (auto buffer : framebuffers) {
		vkDestroyFramebuffer(device_context, buffer, nullptr);
	}
}

void VK_Renderer::StartSynchronizations()
{
	VkFenceCreateInfo fence_info {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(device_context, &fence_info, VK_NULL_HANDLE, &swapchain_available);
}

void VK_Renderer::EndSynchronizations()
{
	vkDestroyFence(device_context,swapchain_available,nullptr);
}

//Rendering Functions

void VK_Renderer::BeginRendering(VkSemaphore acquire_semaphore)
{
	vkAcquireNextImageKHR(device_context, swapchain, UINT64_MAX, acquire_semaphore, swapchain_available, &active_swapchain_id);
	vkWaitForFences(device_context, 1, &swapchain_available, VK_TRUE, UINT64_MAX);
	vkResetFences(device_context, 1, &swapchain_available);
	vkQueueWaitIdle(queue);
}

void VK_Renderer::PresentStopRendering(vector<VkSemaphore> wait_semaphores)
{
	VkResult present_result = VkResult::VK_RESULT_MAX_ENUM;

	VkPresentInfoKHR present_info {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = wait_semaphores.size();
	present_info.pWaitSemaphores = wait_semaphores.data();
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain;
	present_info.pImageIndices = &active_swapchain_id;
	present_info.pResults = &present_result;

	result = vkQueuePresentKHR(queue, &present_info);
}

//~Rendering Functions



//---------------------------------------------VK_VALIDATION_LAYER_CODE--------------------------------------------------------

#ifdef VK_DEBUG
PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCall(
	VkDebugReportFlagsEXT       flags,
	VkDebugReportObjectTypeEXT  obj_type,
	uint64_t                    src_object,
	size_t                      location,
	int32_t                     msg_code,
	const char*                 layer_prefix,
	const char*                 msg,
	void *                      user_data)
{
	string stream = msg;
	/*
	if (stream.length() > 100)
		stream.resize(120);
	*/

	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, msg );
		printf("\n");
		return false;
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, msg);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Vulkan Warning!", stream.c_str(), NULL);
		printf("\n");
		return false;
	}
	else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, msg);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Vulkan Error!", stream.c_str(), NULL);
		printf("\n");
		return true;
	}

	else {
		return false;
	}

}

void VK_Renderer::SetupDebug()
{

	debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debug_create_info.pfnCallback = VulkanDebugCall;
	debug_create_info.flags =
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		0;

	instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
}

void VK_Renderer::InitDebug()
{
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (fvkCreateDebugReportCallbackEXT == nullptr || fvkDestroyDebugReportCallbackEXT == nullptr) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "Could not get debug functions!");
	}

	result = fvkCreateDebugReportCallbackEXT(instance, &debug_create_info, nullptr, &debug_report);
}

void VK_Renderer::DestroyDebug()
{
	fvkDestroyDebugReportCallbackEXT(instance, debug_report, nullptr);
}

#endif
//-------------------------------------------------------------------------------------------------------------------------------
