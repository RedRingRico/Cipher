#include <Game.hpp>
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <dlfcn.h>
#include <VulkanFunctions.hpp>

PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupport;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
	vkGetPhysicalDeviceSurfaceCapabilities;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormats;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
	vkGetPhysicalDeviceSurfacePresentModes;
PFN_vkCreateSwapchainKHR vkCreateSwapchain;
PFN_vkDestroySwapchainKHR vkDestroySwapchain;
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImages;
PFN_vkAcquireNextImageKHR vkAcquireNextImage;
PFN_vkQueuePresentKHR vkQueuePresent;

#define GET_INSTANCE_PROC_ADDR( p_Instance, p_EntryPoint ) \
	{ \
		p_EntryPoint = ( PFN_##p_EntryPoint##KHR )vkGetInstanceProcAddr( \
			p_Instance, #p_EntryPoint "KHR" ); \
		if( p_EntryPoint == nullptr ) \
		{ \
			std::cout << "[GET_INSTANCE_PROC_ADDR] <ERROR> " \
				"Failed to acquire: " << #p_EntryPoint << "KHR" << std::endl; \
			return INITIALISE_ERROR_VULKAN_GET_PROC_ADDR; \
		} \
	}

namespace Cipher
{
	Game::Game( ) :
		m_pVulkanLibraryHandle( nullptr ),
		m_pXCBConnection( nullptr ),
		m_pScreen( nullptr ),
		m_pDeleteWindowAtom( nullptr ),
		m_VulkanInstance( VK_NULL_HANDLE ),
		m_VulkanDevice( VK_NULL_HANDLE ),
		m_ppExtensionNames( nullptr )
	{
	}

	Game::~Game( )
	{
		/*if( m_ppExtensionNames )
		{
			for( size_t Extension = 0; Extension < m_ExtensionNames.size( );
				++Extension )
			{
				delete [ ] m_ppExtensionNames[ Extension ];
			}

			delete [ ] m_ppExtensionNames;
		}*/

		if( m_VulkanDevice != VK_NULL_HANDLE )
		{
			vkDeviceWaitIdle( m_VulkanDevice );
			vkDestroyDevice( m_VulkanDevice, nullptr );
		}

		if( m_VulkanInstance != VK_NULL_HANDLE )
		{
			vkDestroyInstance( m_VulkanInstance, nullptr );
		}

		if( m_pVulkanLibraryHandle )
		{
			dlclose( m_pVulkanLibraryHandle );
		}
	}

	int Game::Initialise( )
	{
		if( this->InitialiseXCBConnection( ) != 0 )
		{
			return 1;
		}

		if( this->InitialiseVulkan( ) != 0 )
		{
			return 1;
		}

		return 0;
	}

	int Game::Execute( )
	{
		return 0;
	}

	int Game::InitialiseXCBConnection( )
	{
		const xcb_setup_t *pSetup;
		xcb_screen_iterator_t ScreenItr;
		int Screen;

		m_pXCBConnection = xcb_connect( nullptr, &Screen );

		if( m_pXCBConnection == nullptr )
		{
			std::cout << "[Cipher::Game::InitialiseXCBConnection] <ERROR> "
				"Failed to initialise the XCB Connection" << std::endl;

			return INITIALISE_ERROR_XCB;
		}

		pSetup = xcb_get_setup( m_pXCBConnection );
		ScreenItr = xcb_setup_roots_iterator( pSetup );

		while( Screen-- > 0 )
		{
			xcb_screen_next( &ScreenItr );
		}

		m_pScreen = ScreenItr.data;

		return 0;
	}

	int Game::InitialiseVulkan( )
	{
		m_pVulkanLibraryHandle = dlopen( "libvulkan.so", RTLD_NOW );

		if( m_pVulkanLibraryHandle == nullptr )
		{
			std::cout << "[Cipher::Game::InitialiseVulkan] <ERROR> "
				"Unable to open Vulkan library" << std::endl;

			return 1;
		}

#define VK_EXPORTED_FUNCTION( p_Function ) \
	if( !( p_Function = ( PFN_##p_Function )dlsym( m_pVulkanLibraryHandle,\
		#p_Function ) ) ) \
	{ \
		std::cout << "Could not load Vulkan function: " << #p_Function << \
			std::endl; \
		return 1; \
	}
#include <VulkanFunctions.inl>

#define VK_GLOBAL_LEVEL_FUNCTION( p_Function ) \
	if( !( p_Function = ( PFN_##p_Function )vkGetInstanceProcAddr( nullptr, \
			#p_Function ) ) ) \
	{ \
		std::cout << "Could not load global-level Vulkan function: " << \
			#p_Function << std::endl; \
		return 1; \
	}
#include <VulkanFunctions.inl>
		
		VkApplicationInfo ApplicationInfo =
		{
			VK_STRUCTURE_TYPE_APPLICATION_INFO,
			nullptr,
			"Cipher",
			VK_MAKE_VERSION( 1, 0, 0 ),
			"CipherEngine",
			VK_MAKE_VERSION( 1, 0, 0 ),
			VK_API_VERSION_1_0
		};

		VkInstanceCreateInfo InstanceCreateInfo =
		{
			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			nullptr,
			0,
			&ApplicationInfo,
			0,
			nullptr,
			0,
			nullptr
		};

		if( vkCreateInstance( &InstanceCreateInfo, nullptr,
			&m_VulkanInstance ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::InitialiseVulkan] <ERROR> "
				"Unable to create a Vulkan instance" << std::endl;

			return 1;
		}

#define VK_INSTANCE_LEVEL_FUNCTION( p_Function ) \
	if( !( p_Function = ( PFN_##p_Function )vkGetInstanceProcAddr( \
			m_VulkanInstance, #p_Function ) ) ) \
	{ \
		std::cout << "Could not load instance-level Vulkan function: " << \
			#p_Function << std::endl; \
		return 1; \
	}
#include <VulkanFunctions.inl>

		uint32_t DeviceCount = 0UL;

		if( ( vkEnumeratePhysicalDevices( m_VulkanInstance, &DeviceCount,
				nullptr ) != VK_SUCCESS ) || ( DeviceCount == 0UL ) )
		{
			std::cout << "[Cipher::Game::InitialiseVulkan] <ERROR> "
				"Failed to enumerate Vulkan devices" << std::endl;

			return 1;
		}

		std::vector< VkPhysicalDevice > PhysicalDevices( DeviceCount );

		if( vkEnumeratePhysicalDevices( m_VulkanInstance, &DeviceCount,
			&PhysicalDevices[ 0 ] ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::InitiliaseVulkan] <ERROR> "
				"Unable to enumerate physical Vulkan devices" << std::endl;

			return 1;
		}

		VkPhysicalDevice SelectedPhysicalDevice = VK_NULL_HANDLE;
		uint32_t SelectedQueueFamilyIndex = UINT32_MAX;

		for( uint32_t Index = 0UL; Index < DeviceCount; ++Index )
		{
			if( CheckPhysicalDeviceProperties( PhysicalDevices[ Index ],
				SelectedQueueFamilyIndex ) )
			{
				SelectedPhysicalDevice = PhysicalDevices[ Index ];
			}
		}

		if( SelectedPhysicalDevice == VK_NULL_HANDLE )
		{
			std::cout << "[Cipher::Game::Initialise] <ERROR> "
				"Could not select the physical device based on the chosen "
				"properties" << std::endl;

			return 1;
		}

		std::vector< float > QueueProperties = { 1.0f };

		VkDeviceQueueCreateInfo QueueCreateInfo =
		{
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			nullptr,
			0,
			SelectedQueueFamilyIndex,
			static_cast< uint32_t >( QueueProperties.size( ) ),
			&QueueProperties[ 0 ]
		};

		VkDeviceCreateInfo DeviceCreateInfo = 
		{
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			nullptr,
			0,
			1,
			&QueueCreateInfo,
			0,
			nullptr,
			0,
			nullptr,
			nullptr
		};

		if( vkCreateDevice( SelectedPhysicalDevice, &DeviceCreateInfo,
			nullptr, &m_VulkanDevice ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::Initialise] <ERROR> "
				"Unable to create a Vulkan device" << std::endl;

			return 1;
		}

		m_VulkanQueueFamilyIndex = SelectedQueueFamilyIndex;

#define VK_DEVICE_LEVEL_FUNCTION( p_Function ) \
	if( !( p_Function = ( PFN_##p_Function )vkGetDeviceProcAddr( \
		m_VulkanDevice, #p_Function ) ) ) \
	{ \
		std::cout << "Could not load device-level Vulkan funciton: " << \
			#p_Function << std::endl; \
		return 1;\
	}
#include <VulkanFunctions.inl>
		
		vkGetDeviceQueue( m_VulkanDevice, m_VulkanQueueFamilyIndex, 0,
			&m_VulkanQueue );

		return 0;
	}

	bool Game::CheckPhysicalDeviceProperties(
		VkPhysicalDevice p_PhysicalDevice, uint32_t &p_QueueFamilyIndex )
	{
		VkPhysicalDeviceProperties DeviceProperties;
		VkPhysicalDeviceFeatures DeviceFeatures;

		vkGetPhysicalDeviceProperties( p_PhysicalDevice, &DeviceProperties );
		vkGetPhysicalDeviceFeatures( p_PhysicalDevice, &DeviceFeatures );

		uint32_t Major = VK_VERSION_MAJOR( DeviceProperties.apiVersion );

		if( ( Major < 1 ) &&
			( DeviceProperties.limits.maxImageDimension2D < 4096 ) )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Physical device " << p_PhysicalDevice << " does not "
				"support the required parameters" << std::endl;

			return false;
		}

		uint32_t QueueFamiliesCount = 0UL;
		vkGetPhysicalDeviceQueueFamilyProperties( p_PhysicalDevice,
			&QueueFamiliesCount, nullptr );

		if( QueueFamiliesCount == 0UL )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Physical device " << p_PhysicalDevice << " does not "
				"have any queue families" << std::endl;

			return false;
		}

		std::vector< VkQueueFamilyProperties > QueueFamilyProperties(
			QueueFamiliesCount );

		vkGetPhysicalDeviceQueueFamilyProperties( p_PhysicalDevice,
			&QueueFamiliesCount, &QueueFamilyProperties[ 0 ] );

		for( uint32_t Index = 0UL; Index < QueueFamiliesCount; ++Index )
		{
			if( ( QueueFamilyProperties[ Index ].queueCount > 0 ) &&
				( QueueFamilyProperties[ Index ].queueFlags &
					VK_QUEUE_GRAPHICS_BIT ) )
			{
				p_QueueFamilyIndex = Index;

				return true;
			}
		}

		std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] <ERROR> "
			"Physical device " << p_PhysicalDevice << " does not support the "
			"requred properties" << std::endl;

		return false;
	}
}
