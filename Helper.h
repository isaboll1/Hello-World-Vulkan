#pragma once
#include "Renderer.h"
#include <fstream>


static std::vector<char> ReadShaderFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t file_size = file.tellg();
	std::vector<char> buffer(file_size);

	file.seekg(0, std::ios::beg);
	file.read(buffer.data(),file_size);

	file.close();

	return buffer;
}

VkShaderModule CreateShaderModule(const std::vector<char>& code, VkDevice device) {

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module = {};
	if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
		throw std::runtime_error("We couldn't create a shader module");
	}

	return shader_module;
}