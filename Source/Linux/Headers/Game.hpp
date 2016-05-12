#ifndef __CIPHER_GAME_HPP__
#define __CIPHER_GAME_HPP__

#include <xcb/xcb.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

const int INITIALISE_ERROR_XCB = 0x00010000;

const int INITIALISE_ERROR_VULKAN_SURFACE_EXTENSION_MISSING =	0x00020000;
const int INITIALISE_ERROR_VULKAN_PLATFORM_EXTENSION_MISSING =	0x00020001;
const int INITIALISE_ERROR_VULKAN_INCOMPATIBLE_DRIVER =			0x00020002;
const int INITIALISE_ERROR_VULKAN_EXTENSION_NOT_PRESENT =		0x00020003;
const int INITIALISE_ERROR_VULKAN_NO_PHYSICAL_DEVICES =			0x00020003;
const int INITIALISE_ERROR_VULKAN_SWAPCHAIN_EXTENSION_MISSING =	0x00020004;
const int INITIALISE_ERROR_VULKAN_GET_PROC_ADDR =				0x00020005;
const int INITIALISE_ERROR_VULKAN_UNKNOWN =						0x0002FFFF;

namespace Cipher
{
	class Game
	{
	public:
		Game( );
		~Game( );

		int Initialise( );
		int Execute( );

	private:
		int InitialiseXCBConnection( );
		int InitialiseVulkan( );
		bool CheckPhysicalDeviceProperties( VkPhysicalDevice p_PhysicalDevice,
			uint32_t &p_QueueFamilyIndex );

		void	*m_pVulkanLibraryHandle;

		// XCB
		xcb_connection_t		*m_pXCBConnection;
		xcb_screen_t			*m_pScreen;
		xcb_window_t			m_Window;
		xcb_intern_atom_reply_t	*m_pDeleteWindowAtom;

		// Vulkan
		VkInstance					m_VulkanInstance;
		VkPhysicalDevice			m_VulkanGPU;
		VkPhysicalDeviceProperties	m_VulkanGPUProperties;
		VkDevice					m_VulkanDevice;
		VkQueue						m_VulkanQueue;
		uint32_t					m_VulkanQueueFamilyIndex;
		uint32_t					m_VulkanQueueCount;
		VkQueueFamilyProperties		*m_pVulkanQueueProperties;
		char **						m_ppExtensionNames;
		std::vector< std::string >	m_ExtensionNames;
	};
}

#endif // __CIPHER_GAME_HPP__

