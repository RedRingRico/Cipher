#ifndef VK_EXPORTED_FUNCTION
#define VK_EXPORTED_FUNCTION( p_Function )
#endif // VK_EXPORTED_FUNCTION

		VK_EXPORTED_FUNCTION( vkGetInstanceProcAddr )

#undef VK_EXPORTED_FUNCTION

#ifndef VK_GLOBAL_LEVEL_FUNCTION
#define VK_GLOBAL_LEVEL_FUNCTION( p_Function )
#endif // VK_GLOBAL_LEVEL_FUNCTION

		VK_GLOBAL_LEVEL_FUNCTION( vkCreateInstance )
		VK_GLOBAL_LEVEL_FUNCTION( vkEnumerateInstanceExtensionProperties )

#undef VK_GLOBAL_LEVEL_FUNCTION

#ifndef VK_INSTANCE_LEVEL_FUNCTION
#define VK_INSTANCE_LEVEL_FUNCTION( p_Function )
#endif // VK_INSTANCE_LEVEL_FUCNTION

		VK_INSTANCE_LEVEL_FUNCTION( vkEnumeratePhysicalDevices )
		VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceProperties )
		VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceFeatures )
		VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceQueueFamilyProperties )
		VK_INSTANCE_LEVEL_FUNCTION( vkCreateDevice )
		VK_INSTANCE_LEVEL_FUNCTION( vkGetDeviceProcAddr )
		VK_INSTANCE_LEVEL_FUNCTION( vkDestroyInstance )
		VK_INSTANCE_LEVEL_FUNCTION( vkEnumerateDeviceExtensionProperties )
		VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceSurfaceSupportKHR )
		VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceSurfaceCapabilitiesKHR )
		VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceSurfaceFormatsKHR )
		VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceSurfacePresentModesKHR )
		VK_INSTANCE_LEVEL_FUNCTION( vkDestroySurfaceKHR )
		VK_INSTANCE_LEVEL_FUNCTION( vkCreateXcbSurfaceKHR )

#undef VK_INSTANCE_LEVEL_FUNCTION

#ifndef VK_DEVICE_LEVEL_FUNCTION
#define VK_DEVICE_LEVEL_FUNCTION( p_Function )
#endif // VK_DEVICE_LEVEL_FUNCTION
		
		VK_DEVICE_LEVEL_FUNCTION( vkGetDeviceQueue )
		VK_DEVICE_LEVEL_FUNCTION( vkDeviceWaitIdle )
		VK_DEVICE_LEVEL_FUNCTION( vkDestroyDevice )
		VK_DEVICE_LEVEL_FUNCTION( vkCreateSemaphore )
		VK_DEVICE_LEVEL_FUNCTION( vkCreateCommandPool )
		VK_DEVICE_LEVEL_FUNCTION( vkAllocateCommandBuffers )
		VK_DEVICE_LEVEL_FUNCTION( vkBeginCommandBuffer )
		VK_DEVICE_LEVEL_FUNCTION( vkCmdPipelineBarrier )
		VK_DEVICE_LEVEL_FUNCTION( vkCmdClearColorImage )
		VK_DEVICE_LEVEL_FUNCTION( vkEndCommandBuffer )
		VK_DEVICE_LEVEL_FUNCTION( vkQueueSubmit )
		VK_DEVICE_LEVEL_FUNCTION( vkFreeCommandBuffers )
		VK_DEVICE_LEVEL_FUNCTION( vkDestroyCommandPool )
		VK_DEVICE_LEVEL_FUNCTION( vkDestroySemaphore )
		VK_DEVICE_LEVEL_FUNCTION( vkCreateSwapchainKHR )
		VK_DEVICE_LEVEL_FUNCTION( vkGetSwapchainImagesKHR )
		VK_DEVICE_LEVEL_FUNCTION( vkAcquireNextImageKHR )
		VK_DEVICE_LEVEL_FUNCTION( vkQueuePresentKHR )
		VK_DEVICE_LEVEL_FUNCTION( vkDestroySwapchainKHR )

#undef VK_DEVICE_LEVEL_FUNCTION

