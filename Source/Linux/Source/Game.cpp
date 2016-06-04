#include <Game.hpp>
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <dlfcn.h>
#include <VulkanFunctions.hpp>
#include <GitVersion.hpp>

namespace Cipher
{
	Game::Game( ) :
		m_pVulkanLibraryHandle( nullptr ),
		m_pXCBConnection( nullptr ),
		m_pScreen( nullptr ),
		m_pDeleteWindowAtom( nullptr ),
		m_VulkanInstance( VK_NULL_HANDLE ),
		m_VulkanDevice( VK_NULL_HANDLE )
	{
	}

	Game::~Game( )
	{
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

		// Get the extensions Vulkan supports
		uint32_t ExtensionCount = 0UL;

		if( ( vkEnumerateInstanceExtensionProperties( nullptr, &ExtensionCount,
			nullptr ) != VK_SUCCESS ) || ( ExtensionCount == 0UL ) )
		{
			std::cout << "[Cipher::Game::InitialiseVulkan] <ERROR> "
				"Failed to enumerate Vulkan extenions" << std::endl;

			return 1;
		}

		std::vector< VkExtensionProperties > AvailableExtensions(
			ExtensionCount );

		if( vkEnumerateInstanceExtensionProperties( nullptr, &ExtensionCount,
			&AvailableExtensions[ 0 ] ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::InitialiseVulkan] <ERROR> "
				"Failed to acquire Vulkan extensions" << std::endl;

			return 1;
		}

		std::vector< const char * > VulkanExtensions =
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_XCB_SURFACE_EXTENSION_NAME
		};

		std::cout << "Vulkan instance supports " << ExtensionCount <<
			" extensions" << std::endl;

		PrintExtensionNames( AvailableExtensions );

		for( size_t Index = 0; Index < VulkanExtensions.size( ); ++Index )
		{
			if( CheckExtensionAvailability( VulkanExtensions[ Index ],
				AvailableExtensions ) == false )
			{
				std::cout << "[Cipher::Game::InitialiseVulkan] <ERROR> "
					"Unable to locate extension: " <<
					VulkanExtensions[ Index ] << std::endl;

				return 1;
			}
		}
		
		VkApplicationInfo ApplicationInfo =
		{
			VK_STRUCTURE_TYPE_APPLICATION_INFO,
			nullptr,
			"Cipher",
			VK_MAKE_VERSION( GIT_MAJOR_BUILD_VERSION, GIT_MINOR_BUILD_VERSION,
				GIT_REVISION_BUILD_NUM ),
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
			static_cast< uint32_t >( VulkanExtensions.size( ) ),
			&VulkanExtensions[ 0 ]
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

		VkXcbSurfaceCreateInfoKHR SurfaceCreateInfo =
		{
			VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			m_pXCBConnection,
			m_Window
		};

		if( vkCreateXcbSurfaceKHR( m_VulkanInstance, &SurfaceCreateInfo,
			nullptr, &m_VulkanPresentationSurface ) != VK_SUCCESS )
		{
			return 1;
		}

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

		uint32_t SelectedGraphicsQueueFamilyIndex = UINT32_MAX;
		uint32_t SelectedPresentQueueFamilyIndex = UINT32_MAX;

		for( uint32_t Device = 0; Device < DeviceCount; ++Device )
		{
			if( CheckPhysicalDeviceProperties( PhysicalDevices[ Device ],
				SelectedGraphicsQueueFamilyIndex,
				SelectedPresentQueueFamilyIndex ) == true )
			{
				m_VulkanPhysicalDevice = PhysicalDevices[ Device ];
			}
		}

		if( m_VulkanPhysicalDevice == VK_NULL_HANDLE )
		{
			std::cout << "[Cipher::Game::Initialise] <ERROR> "
				"Could not select the physical device based on the chosen "
				"properties" << std::endl;

			return 1;
		}

		std::vector< VkDeviceQueueCreateInfo > QueueCreateInfo;
		std::vector< float > QueueProperties = { 1.0f };

		QueueCreateInfo.push_back(
			{
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr,
				0,
				SelectedGraphicsQueueFamilyIndex,
				static_cast< uint32_t >( QueueProperties.size( ) ),
				&QueueProperties[ 0 ]
			}
		);

		if( SelectedGraphicsQueueFamilyIndex !=
			SelectedPresentQueueFamilyIndex )
		{
			QueueCreateInfo.push_back(
				{
					VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					nullptr,
					0,
					SelectedPresentQueueFamilyIndex,
					static_cast< uint32_t >( QueueProperties.size( ) ),
					&QueueProperties[ 0 ]
				}
			);
		}

		std::vector< const char * > Extensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		VkDeviceCreateInfo DeviceCreateInfo = 
		{
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			nullptr,
			0,
			static_cast< uint32_t >( QueueCreateInfo.size( ) ),
			&QueueCreateInfo[ 0 ],
			0,
			nullptr,
			static_cast< uint32_t >( Extensions.size( ) ),
			&Extensions[ 0 ],
			nullptr
		};

		if( vkCreateDevice( m_VulkanPhysicalDevice, &DeviceCreateInfo,
			nullptr, &m_VulkanDevice ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::Initialise] <ERROR> "
				"Unable to create a Vulkan device" << std::endl;

			return 1;
		}

		m_VulkanGraphicsQueueFamilyIndex = SelectedGraphicsQueueFamilyIndex;
		m_VulkanPresentQueueFamilyIndex = SelectedPresentQueueFamilyIndex;

#define VK_DEVICE_LEVEL_FUNCTION( p_Function ) \
	if( !( p_Function = ( PFN_##p_Function )vkGetDeviceProcAddr( \
		m_VulkanDevice, #p_Function ) ) ) \
	{ \
		std::cout << "Could not load device-level Vulkan function: " << \
			#p_Function << std::endl; \
		return 1;\
	}
#include <VulkanFunctions.inl>
		
		vkGetDeviceQueue( m_VulkanDevice, m_VulkanGraphicsQueueFamilyIndex, 0,
			&m_VulkanGraphicsQueue );
		vkGetDeviceQueue( m_VulkanDevice, m_VulkanPresentQueueFamilyIndex, 0,
			&m_VulkanPresentQueue );

		return 0;
	}

	bool Game::CheckPhysicalDeviceProperties(
		VkPhysicalDevice p_PhysicalDevice,
		uint32_t &p_SelectedGraphicsQueueFamilyIndex,
		uint32_t &p_SelectedPresentQueueFamilyIndex )
	{
		uint32_t ExtensionCount = 0UL;

		if( ( vkEnumerateDeviceExtensionProperties( p_PhysicalDevice, nullptr,
			&ExtensionCount, nullptr ) != VK_SUCCESS ) ||
			( ExtensionCount == 0UL ) )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Failed to enumerate physical device extensions" <<
				std::endl;

			return false;
		}

		std::vector< VkExtensionProperties > AvailableExtensions(
			ExtensionCount );

		if( vkEnumerateDeviceExtensionProperties( p_PhysicalDevice, nullptr,
			&ExtensionCount, &AvailableExtensions[ 0 ] ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Failed to populate the extensions array" << std::endl;

			return false;
		}

		std::vector< const char * > DeviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		VkPhysicalDeviceProperties DeviceProperties;
		VkPhysicalDeviceFeatures DeviceFeatures;

		vkGetPhysicalDeviceProperties( p_PhysicalDevice, &DeviceProperties );
		vkGetPhysicalDeviceFeatures( p_PhysicalDevice, &DeviceFeatures );


		std::cout << "Device: \"" << DeviceProperties.deviceName <<
			"\" Supports " << AvailableExtensions.size( ) << " extensions" <<
			std::endl;

		PrintExtensionNames( AvailableExtensions );

		for( size_t Extension = 0; Extension < DeviceExtensions.size( );
			++Extension )
		{
			if( CheckExtensionAvailability( DeviceExtensions[ Extension ],
				AvailableExtensions ) == false )
			{
				std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
					"<ERROR> Device: \"" << p_PhysicalDevice << "\" does not "
					"support required extension: \"" <<
					DeviceExtensions[ Extension ] << "\"" << std::endl;

				return false;
			}
		}

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
		std::vector< VkBool32 > QueuePresentSupport( QueueFamiliesCount );

		vkGetPhysicalDeviceQueueFamilyProperties( p_PhysicalDevice,
			&QueueFamiliesCount, &QueueFamilyProperties[ 0 ] );

		uint32_t GraphicsQueueFamilyIndex = UINT32_MAX;
		uint32_t PresentQueueFamilyIndex = UINT32_MAX;

		for( uint32_t Index = 0UL; Index < QueueFamiliesCount; ++Index )
		{
			vkGetPhysicalDeviceSurfaceSupportKHR( p_PhysicalDevice,
				Index, m_VulkanPresentationSurface,
				&QueuePresentSupport[ Index ] );

			if( ( QueueFamilyProperties[ Index ].queueCount > 0 ) &&
				( QueueFamilyProperties[ Index ].queueFlags &
					VK_QUEUE_GRAPHICS_BIT ) )
			{
				// Use the first available queue that supports graphics
				if( GraphicsQueueFamilyIndex == UINT32_MAX )
				{
					p_SelectedGraphicsQueueFamilyIndex = Index;
				}

				// If a queue supports both graphics and present, use it
				if( QueuePresentSupport[ Index ] == VK_TRUE )
				{
					p_SelectedGraphicsQueueFamilyIndex = Index;
					p_SelectedPresentQueueFamilyIndex = Index;

					return true;
				}
			}
		}

		// The queue does not support both graphica and present, use a
		// separate queue for each
		for( uint32_t Index = 0; Index < QueueFamiliesCount; ++Index )
		{
			if( QueuePresentSupport[ Index ] == true )
			{
				PresentQueueFamilyIndex = Index;
				break;
			}
		}

		// No suitable queue support for both graphics and/or present
		if( ( GraphicsQueueFamilyIndex == UINT32_MAX ) ||
			( PresentQueueFamilyIndex == UINT32_MAX ) )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Could not find a queue family with the required "
				"properties on the physical device: \"" << p_PhysicalDevice <<
				"\"" << std::endl;

			return false;
		}

		p_SelectedGraphicsQueueFamilyIndex = GraphicsQueueFamilyIndex;
		p_SelectedPresentQueueFamilyIndex = PresentQueueFamilyIndex;

		return true;
	}

	bool Game::CheckExtensionAvailability( const char *p_pExtension,
		const std::vector< VkExtensionProperties > &p_Extensions )
	{
		for( size_t Index = 0; Index < p_Extensions.size( ); ++Index )
		{
			if( strcmp( p_Extensions[ Index ].extensionName,
				p_pExtension ) == 0 )
			{
				return true;
			}
		}

		return false;
	}
	void Game::PrintExtensionNames(
		const std::vector< VkExtensionProperties > &p_Extensions )
	{
		size_t LongestName = 0;

		for( auto Extension : p_Extensions )
		{
			size_t ExtensionLength = strlen( Extension.extensionName );

			if( ExtensionLength > LongestName )
			{
				LongestName = ExtensionLength;
			}
		}

		std::cout << "\tName";
		for( size_t Char = strlen( "Name" ); Char < LongestName; ++Char )
		{
			std::cout << " ";
		}
		std::cout << " Version" << std::endl;

		for( auto Extension : p_Extensions )
		{
			std::cout << "\t" << Extension.extensionName;
			for( size_t Char = strlen( Extension.extensionName );
				Char < LongestName; ++Char )
			{
				std::cout << " ";
			}
			std::cout << " " <<
				VK_VERSION_MAJOR( Extension.specVersion ) << "." <<
				VK_VERSION_MINOR( Extension.specVersion ) << "." <<
				VK_VERSION_PATCH( Extension.specVersion ) << std::endl;
		}
	}
}

