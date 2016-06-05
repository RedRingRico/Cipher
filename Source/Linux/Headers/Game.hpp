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
		xcb_intern_atom_cookie_t GetCookieFromAtom(
			const std::string &p_StateName );
		xcb_atom_t GetReplyAtomFromCookie( xcb_intern_atom_cookie_t p_Cookie );
		int InitialiseXCBConnection( );
		int InitialiseVulkan( );
		bool CheckPhysicalDeviceProperties( VkPhysicalDevice p_PhysicalDevice,
			uint32_t &p_SelectedGraphicsQueueFamilyIndex,
			uint32_t &p_SelectedPresentQueueFamilyIndex );
		bool CheckExtensionAvailability( const char *p_pExtension,
			const std::vector< VkExtensionProperties > &p_Extensions );
		void PrintExtensionNames(
			const std::vector< VkExtensionProperties > &p_Extensions );
		VkSurfaceFormatKHR GetSwapChainFormat(
			std::vector< VkSurfaceFormatKHR > &p_SurfaceFormats );
		VkExtent2D GetSwapChainExtent(
			VkSurfaceCapabilitiesKHR &p_SurfaceCapabilities );
		VkImageUsageFlags GetSwapChainUsageFlags(
			VkSurfaceCapabilitiesKHR &p_SurfaceCapabilities );
		VkPresentModeKHR GetSwapChainPresentMode(
			std::vector< VkPresentModeKHR > &p_PresentModes );
		uint32_t OnWindowSizeChanged( );
		uint32_t CreateSwapChain( );
		uint32_t CreateCommandBuffers( );
		uint32_t RecordCommandBuffers( );
		void ClearCmd( );
		uint32_t Render( );
		void ToggleFullscreen( );

		void	*m_pVulkanLibraryHandle;

		// XCB
		xcb_connection_t		*m_pXCBConnection;
		xcb_screen_t			*m_pScreen;
		xcb_window_t			m_Window;
		xcb_intern_atom_reply_t	*m_pDeleteWindowAtom;

		// Vulkan
		VkInstance						m_VulkanInstance;
		VkSurfaceKHR					m_VulkanPresentationSurface;
		VkSwapchainKHR					m_VulkanSwapChain;
		VkPhysicalDevice				m_VulkanPhysicalDevice;
		VkDevice						m_VulkanDevice;
		VkQueue							m_VulkanGraphicsQueue;
		VkQueue							m_VulkanPresentQueue;
		std::vector< VkCommandBuffer >	m_VulkanPresentQueueCmdBuffers;
		VkCommandPool					m_VulkanPresentQueueCmdPool;
		uint32_t						m_VulkanGraphicsQueueFamilyIndex;
		uint32_t						m_VulkanPresentQueueFamilyIndex;
		uint32_t						m_VulkanQueueCount;
		VkSemaphore						m_VulkanImageAvailableSemaphore;
		VkSemaphore						m_VulkanRenderingFinishedSemaphore;
		VkQueueFamilyProperties			*m_pVulkanQueueProperties;
		std::vector< std::string >		m_ExtensionNames;
		bool							m_Fullscreen;
		char							m_Keys[ 256 ];
	};
}

#endif // __CIPHER_GAME_HPP__

