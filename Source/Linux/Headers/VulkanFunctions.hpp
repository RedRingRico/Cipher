#ifndef __CIPHER_VULKANFUNCTIONS_HPP__
#define __CIPHER_VULKANFUNCTIONS_HPP__

#include <vulkan/vulkan.h>

namespace Cipher
{
#define VK_EXPORTED_FUNCTION( p_Function ) extern PFN_##p_Function p_Function;
#define VK_GLOBAL_LEVEL_FUNCTION( p_Function ) \
	extern PFN_##p_Function p_Function;
#define VK_INSTANCE_LEVEL_FUNCTION( p_Function ) \
	extern PFN_##p_Function p_Function;
#define VK_DEVICE_LEVEL_FUNCTION( p_Function ) \
	extern PFN_##p_Function p_Function;

#include <VulkanFunctions.inl>
}

#endif // __CIPHER_VULKANFUNCITONS_HPP__

