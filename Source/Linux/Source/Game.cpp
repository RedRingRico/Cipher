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
		m_VulkanPresentationSurface( VK_NULL_HANDLE ),
		m_VulkanSwapChain( VK_NULL_HANDLE ),
		m_VulkanPhysicalDevice( VK_NULL_HANDLE ),
		m_VulkanDevice( VK_NULL_HANDLE ),
		m_VulkanGraphicsQueue( VK_NULL_HANDLE ),
		m_VulkanPresentQueue( VK_NULL_HANDLE ),
		m_VulkanPresentQueueCmdBuffers( 0 ),
		m_VulkanPresentQueueCmdPool( VK_NULL_HANDLE ),
		m_VulkanGraphicsQueueFamilyIndex( 0UL ),
		m_VulkanPresentQueueFamilyIndex( 0UL ),
		m_Fullscreen( false )
	{
		memset( m_Keys, 0, sizeof( m_Keys ) );
	}

	Game::~Game( )
	{
		if( m_VulkanDevice != VK_NULL_HANDLE )
		{
			vkDeviceWaitIdle( m_VulkanDevice );

			if( m_VulkanImageAvailableSemaphore != VK_NULL_HANDLE )
			{
				vkDestroySemaphore( m_VulkanDevice,
					m_VulkanImageAvailableSemaphore, nullptr );
			}

			if( m_VulkanRenderingFinishedSemaphore != VK_NULL_HANDLE )
			{
				vkDestroySemaphore( m_VulkanDevice,
					m_VulkanRenderingFinishedSemaphore, nullptr );
			}

			if( m_VulkanSwapChain != VK_NULL_HANDLE )
			{
				vkDestroySwapchainKHR( m_VulkanDevice, m_VulkanSwapChain,
					nullptr );
			}

			vkDestroyDevice( m_VulkanDevice, nullptr );
		}

		if( m_VulkanPresentationSurface != VK_NULL_HANDLE )
		{
			vkDestroySurfaceKHR( m_VulkanInstance, m_VulkanPresentationSurface,
				nullptr );
		}

		if( m_VulkanInstance != VK_NULL_HANDLE )
		{
			vkDestroyInstance( m_VulkanInstance, nullptr );
		}

		if( m_pVulkanLibraryHandle )
		{
			dlclose( m_pVulkanLibraryHandle );
		}

		xcb_destroy_window( m_pXCBConnection, m_Window );
		xcb_disconnect( m_pXCBConnection );
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
		xcb_intern_atom_cookie_t ProtocolsCookie = xcb_intern_atom(
			m_pXCBConnection, 1, 12, "WM_PROTOCOLS" );
		xcb_intern_atom_reply_t *pProtocolsReply = xcb_intern_atom_reply(
			m_pXCBConnection, ProtocolsCookie, 0 );
		xcb_intern_atom_cookie_t DeleteCookie = xcb_intern_atom(
			m_pXCBConnection, 0, 16, "WM_DELETE_WINDOW" );
		xcb_intern_atom_reply_t *pDeleteReply = xcb_intern_atom_reply(
			m_pXCBConnection, DeleteCookie, 0 );
		xcb_change_property( m_pXCBConnection, XCB_PROP_MODE_REPLACE, m_Window,
			( *pProtocolsReply ).atom, 4, 32, 1, &( *pDeleteReply ).atom );
		free( pProtocolsReply );

		xcb_map_window( m_pXCBConnection, m_Window );
		xcb_flush( m_pXCBConnection );

		xcb_generic_event_t *pEvent;
		bool Run = true;
		bool Resize = true;

		while( Run == true )
		{
			pEvent = xcb_poll_for_event( m_pXCBConnection );

			if( pEvent != nullptr )
			{
				switch( pEvent->response_type & 0x7F )
				{
					case XCB_CONFIGURE_NOTIFY:
					{
						xcb_configure_notify_event_t *pConfigureEvent =
							( xcb_configure_notify_event_t * )pEvent;
						static uint16_t Width = pConfigureEvent->width;
						static uint16_t Height = pConfigureEvent->height;

						if( ( ( pConfigureEvent->width > 0 ) &&
								( Width != pConfigureEvent->width ) ) ||
							( ( pConfigureEvent->height > 0 ) &&
								( Height != pConfigureEvent->height ) ) )
						{
							Resize = true;
							Width = pConfigureEvent->width;
							Height = pConfigureEvent->height;
						}
						break;
					}
					case XCB_CLIENT_MESSAGE:
					{
						if( ( *( xcb_client_message_event_t * )pEvent ).data.
							data32[ 0 ] == ( *pDeleteReply ).atom )
						{
							Run = false;
						}
						break;
					}
					case XCB_KEY_PRESS:
					{
						xcb_key_press_event_t *pKeyPress =
							( xcb_key_press_event_t * )pEvent;
						xcb_keycode_t Key = pKeyPress->detail;

						if( Key == 9 )
						{
							Run = false;
						}

						if( Key == 95 )
						{
							ToggleFullscreen( );
						}

						break;
					}
				}

				free( pEvent );
			}
			else
			{
				if( Resize )
				{
					std::cout << "Resize" << std::endl;
					Resize = false;
					if( OnWindowSizeChanged( ) != 0 )
					{
						Run = false;
					}
				}

				if( Render( ) != 0 )
				{
					Run = false;
				}
			}
		}

		free( pDeleteReply );

		return 0;
	}

	xcb_intern_atom_cookie_t Game::GetCookieFromAtom(
		const std::string &p_StateName )
	{
		return xcb_intern_atom( m_pXCBConnection, 0, p_StateName.size( ),
			p_StateName.c_str( ) );
	}

	xcb_atom_t Game::GetReplyAtomFromCookie(
		xcb_intern_atom_cookie_t p_Cookie )
	{
		xcb_generic_error_t *pError;

		xcb_intern_atom_reply_t *pReply = xcb_intern_atom_reply(
			m_pXCBConnection, p_Cookie, &pError );

		if( pError )
		{
			std::cout << "[Cipher::Game::GetReplyAtomFromCookie] <ERROR> "
				"Cant get the reply atom from cookie: \"" <<
				pError->error_code << "\""<< std::endl;
		}

		return pReply->atom;
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
		m_Window = xcb_generate_id( m_pXCBConnection );

		uint32_t WindowValues[ ] =
		{
			m_pScreen->black_pixel,
			XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS |
				XCB_EVENT_MASK_STRUCTURE_NOTIFY
		};

		xcb_create_window( m_pXCBConnection, XCB_COPY_FROM_PARENT, m_Window,
			m_pScreen->root, 20, 20, 500, 500, 0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT, m_pScreen->root_visual,
			XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, WindowValues );

		xcb_change_property( m_pXCBConnection, XCB_PROP_MODE_REPLACE, m_Window,
			XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen( "CIPHER" ),
			"CIPHER" );

		xcb_map_window( m_pXCBConnection, m_Window );
		xcb_flush( m_pXCBConnection );

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

		VkSemaphoreCreateInfo SemaphoreCreateInfo =
		{
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			nullptr,
			0
		};

		if( ( vkCreateSemaphore( m_VulkanDevice, &SemaphoreCreateInfo,
			nullptr, &m_VulkanImageAvailableSemaphore ) != VK_SUCCESS ) ||
			( vkCreateSemaphore( m_VulkanDevice, &SemaphoreCreateInfo,
			nullptr, &m_VulkanRenderingFinishedSemaphore ) != VK_SUCCESS ) )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Unable to create semaphores" << std::endl;

			return false;
		}

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

	VkSurfaceFormatKHR Game::GetSwapChainFormat(
		std::vector< VkSurfaceFormatKHR > &p_SurfaceFormats )
	{
		if( ( p_SurfaceFormats.size( ) == 1 ) &&
			( p_SurfaceFormats[ 0 ].format == VK_FORMAT_UNDEFINED ) )
		{
			return {
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_COLORSPACE_SRGB_NONLINEAR_KHR
				};
		}

		for( VkSurfaceFormatKHR &SurfaceFormat : p_SurfaceFormats )
		{
			if( SurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM )
			{
				return SurfaceFormat;
			}
		}

		return p_SurfaceFormats[ 0 ];
	}

	VkExtent2D Game::GetSwapChainExtent(
		VkSurfaceCapabilitiesKHR &p_SurfaceCapabilities )
	{
		// Need to check the documentation, as width and height are unsigned
		// integers
		if( ( p_SurfaceCapabilities.currentExtent.width == -1 ) &&
			( p_SurfaceCapabilities.currentExtent.height == -1 ) )
		{
			VkExtent2D Extent = { 640, 480 };

			if( Extent.width < p_SurfaceCapabilities.minImageExtent.width )
			{
				Extent.width = p_SurfaceCapabilities.minImageExtent.width;
			}
			if( Extent.height < p_SurfaceCapabilities.minImageExtent.height )
			{
				Extent.height = p_SurfaceCapabilities.minImageExtent.height;
			}

			if( Extent.width > p_SurfaceCapabilities.maxImageExtent.width )
			{
				Extent.width = p_SurfaceCapabilities.maxImageExtent.width;
			}
			if( Extent.height > p_SurfaceCapabilities.maxImageExtent.height )
			{
				Extent.height = p_SurfaceCapabilities.maxImageExtent.height;
			}

			std::cout << "Extent: " << Extent.width << "x" << Extent.height <<
				std::endl;

			return Extent;
		}

		std::cout << "Extent: " << p_SurfaceCapabilities.currentExtent.width <<
			"x" << p_SurfaceCapabilities.currentExtent.height << std::endl;

		return p_SurfaceCapabilities.currentExtent;
	}

	VkImageUsageFlags Game::GetSwapChainUsageFlags(
		VkSurfaceCapabilitiesKHR &p_SurfaceCapabilities )
	{
		if( p_SurfaceCapabilities.supportedUsageFlags &
			VK_IMAGE_USAGE_TRANSFER_DST_BIT )
		{
			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		std::cout << "[Cipher::Game::GetSwapChainUsageFlags] <ERROR> "
			"VK_IMAGE_USAGE_TRANSFER_DST_BIT not supported" << std::endl;

		return static_cast< VkImageUsageFlags >( -1 );
	}

	VkPresentModeKHR Game::GetSwapChainPresentMode(
		std::vector< VkPresentModeKHR > &p_PresentModes )
	{
		for( VkPresentModeKHR &Mode : p_PresentModes )
		{
			if( Mode == VK_PRESENT_MODE_MAILBOX_KHR )
			{
				return Mode;
			}
		}

		for( VkPresentModeKHR &Mode : p_PresentModes )
		{
			if( Mode == VK_PRESENT_MODE_FIFO_KHR )
			{
				return Mode;
			}
		}

		std::cout << "[Cipher::Game::GetSwapChainPresentMode] <ERROR> "
			"MAILBOX or FIFO present mode not supported" << std::endl;

		return static_cast< VkPresentModeKHR >( -1 );
	}

	uint32_t Game::OnWindowSizeChanged( )
	{
		ClearCmd( );

		if( CreateSwapChain( ) != 0 )
		{
			return 1;
		}

		if( CreateCommandBuffers( ) != 0 )
		{
			return 1;
		}

		return 0;
	}

	void Game::ClearCmd( )
	{
		if( m_VulkanDevice != VK_NULL_HANDLE )
		{
			vkDeviceWaitIdle( m_VulkanDevice );

			if( ( m_VulkanPresentQueueCmdBuffers.size( ) > 0 ) &&
				( m_VulkanPresentQueueCmdBuffers[ 0 ] != VK_NULL_HANDLE ) )
			{
				vkFreeCommandBuffers( m_VulkanDevice,
					m_VulkanPresentQueueCmdPool, static_cast< uint32_t >(
						m_VulkanPresentQueueCmdBuffers.size( ) ),
					&m_VulkanPresentQueueCmdBuffers[ 0 ] );
				m_VulkanPresentQueueCmdBuffers.clear( );
			}

			if( m_VulkanPresentQueueCmdPool != VK_NULL_HANDLE )
			{
				vkDestroyCommandPool( m_VulkanDevice,
					m_VulkanPresentQueueCmdPool, nullptr );
				m_VulkanPresentQueueCmdPool = VK_NULL_HANDLE;
			}
		}
	}

	uint32_t Game::CreateSwapChain( )
	{
		if( m_VulkanDevice != VK_NULL_HANDLE )
		{
			vkDeviceWaitIdle( m_VulkanDevice );
		}

		VkSurfaceCapabilitiesKHR SurfaceCapabilities;

		if( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_VulkanPhysicalDevice,
			m_VulkanPresentationSurface, &SurfaceCapabilities ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Could not check presentation surface capabilities" <<
				std::endl;

			return 1;
		}
		
		uint32_t FormatsCount;

		if( ( vkGetPhysicalDeviceSurfaceFormatsKHR( m_VulkanPhysicalDevice,
			m_VulkanPresentationSurface, &FormatsCount, nullptr ) !=
			VK_SUCCESS ) || ( FormatsCount == 0UL ) )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Could not enumerate presentation surface formats" <<
				std::endl;

			return 1;
		}

		std::vector< VkSurfaceFormatKHR > SurfaceFormats( FormatsCount );

		if( vkGetPhysicalDeviceSurfaceFormatsKHR( m_VulkanPhysicalDevice,
			m_VulkanPresentationSurface, &FormatsCount,
			&SurfaceFormats[ 0 ] ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Could not acquire presentation surface formats" <<
				std::endl;

			return 1;
		}

		uint32_t PresentModesCount;
		if( ( vkGetPhysicalDeviceSurfacePresentModesKHR(
			m_VulkanPhysicalDevice, m_VulkanPresentationSurface,
			&PresentModesCount, nullptr ) != VK_SUCCESS ) ||
			( PresentModesCount == 0UL ) )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Could not enumerate presentation surface present "
				"modes" << std::endl;

			return 1;
		}

		std::vector< VkPresentModeKHR > PresentModes( PresentModesCount );

		if( vkGetPhysicalDeviceSurfacePresentModesKHR( m_VulkanPhysicalDevice,
			m_VulkanPresentationSurface, &PresentModesCount,
			&PresentModes[ 0 ] ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] "
				"<ERROR> Could not acquire presentation surface present "
				"modes" << std::endl;

			return 1;
		}

		uint32_t SwapChainImageCount = SurfaceCapabilities.minImageCount + 1;

		if( ( SurfaceCapabilities.maxImageCount > 0 ) &&
			( SwapChainImageCount > SurfaceCapabilities.maxImageCount ) )
		{
			SwapChainImageCount = SurfaceCapabilities.maxImageCount;
		}

		std::cout << "Swap chain image count: " << SwapChainImageCount <<
			std::endl;

		VkSurfaceFormatKHR SwapChainFormat = GetSwapChainFormat(
			SurfaceFormats );
		VkExtent2D SwapChainExtent = GetSwapChainExtent( SurfaceCapabilities );
		VkImageUsageFlags SwapChainUsage = GetSwapChainUsageFlags(
			SurfaceCapabilities );
		VkSurfaceTransformFlagBitsKHR SwapChainTransform =
			VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		VkPresentModeKHR SwapChainPresentMode = GetSwapChainPresentMode(
			PresentModes );
		VkSwapchainKHR OldSwapChain = m_VulkanSwapChain;

		if( static_cast< int >( SwapChainUsage ) == -1 )
		{
			return 1;
		}

		if( static_cast< int >( SwapChainPresentMode ) == -1 )
		{
			return 1;
		}

		VkSwapchainCreateInfoKHR SwapChainCreateInfo =
		{
			VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			nullptr,
			0,
			m_VulkanPresentationSurface,
			SwapChainImageCount,
			SwapChainFormat.format,
			SwapChainFormat.colorSpace,
			SwapChainExtent,
			1,
			SwapChainUsage,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			SwapChainTransform,
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			SwapChainPresentMode,
			VK_TRUE,
			OldSwapChain
		};

		if( vkCreateSwapchainKHR( m_VulkanDevice, &SwapChainCreateInfo,
			nullptr, &m_VulkanSwapChain ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::CheckPhysicalDeviceProperties] " <<
				"<ERROR> Unable to create swap chain" << std::endl;

			return 1;
		}

		if( OldSwapChain != VK_NULL_HANDLE )
		{
			vkDestroySwapchainKHR( m_VulkanDevice, OldSwapChain, nullptr );
		}
		return 0;
	}

	uint32_t Game::CreateCommandBuffers( )
	{
		VkCommandPoolCreateInfo CommandPoolCreateInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			0,
			m_VulkanPresentQueueFamilyIndex
		};

		if( vkCreateCommandPool( m_VulkanDevice, &CommandPoolCreateInfo,
			nullptr, &m_VulkanPresentQueueCmdPool ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::CreateCommandBuffers] <ERROR> "
				"Failed to create a command pool" << std::endl;

			return 1;
		}

		uint32_t ImageCount = 0UL;

		if( ( vkGetSwapchainImagesKHR( m_VulkanDevice, m_VulkanSwapChain,
			&ImageCount, nullptr ) != VK_SUCCESS ) || ( ImageCount == 0UL ) )
		{
			std::cout << "[Cipher::Game::CreateCommandBuffers] <ERROR> "
				"Failed to acquire the number of swap chain images" <<
				std::endl;

			return 1;
		}

		m_VulkanPresentQueueCmdBuffers.resize( ImageCount );

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			m_VulkanPresentQueueCmdPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			ImageCount
		};

		if( vkAllocateCommandBuffers( m_VulkanDevice,
			&CommandBufferAllocateInfo,
			&m_VulkanPresentQueueCmdBuffers[ 0 ] ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::CreateCommandBuffers] <ERROR> "
				"Unable to allocate command buffers" << std::endl;

			return 1;
		}

		if( RecordCommandBuffers( ) != 0 )
		{
			std::cout << "[Cipher::Game::CreateCommandBuffers] <ERROR> "
				"Could not record command buffers" << std::endl;

			return 1;
		}

		return 0;
	}

	uint32_t Game::RecordCommandBuffers( )
	{
		uint32_t ImageCount =
			static_cast< uint32_t >( m_VulkanPresentQueueCmdBuffers.size( ) );

		std::vector< VkImage > SwapChainImages( ImageCount );

		if( vkGetSwapchainImagesKHR( m_VulkanDevice, m_VulkanSwapChain,
			&ImageCount, &SwapChainImages[ 0 ] ) != VK_SUCCESS )
		{
			std::cout << "[Cipher::Game::RecordCommandBuffers] <ERROR> "
				"Could not get the swap chain images" << std::endl;

			return 1;
		}

		VkCommandBufferBeginInfo CommandBufferBeginInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
			nullptr
		};

		VkClearColorValue ClearColour =
		{
			{ 0.0f, 17.0f / 255.0f, 43.0f / 255.0f, 1.0f }
		};

		VkImageSubresourceRange ImageSubresourceRange =
		{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0,
			1,
			0,
			1
		};

		for( uint32_t Image = 0; Image < ImageCount; ++Image )
		{
			VkImageMemoryBarrier PresentToClearBarrier =
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_MEMORY_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				SwapChainImages[ Image ],
				ImageSubresourceRange
			};

			VkImageMemoryBarrier ClearToPresentBarrier =
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				SwapChainImages[ Image ],
				ImageSubresourceRange
			};

			vkBeginCommandBuffer( m_VulkanPresentQueueCmdBuffers[ Image ],
				&CommandBufferBeginInfo );
			vkCmdPipelineBarrier( m_VulkanPresentQueueCmdBuffers[ Image ],
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
				&PresentToClearBarrier );

			vkCmdClearColorImage( m_VulkanPresentQueueCmdBuffers[ Image ],
				SwapChainImages[ Image ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				&ClearColour, 1, &ImageSubresourceRange );

			vkCmdPipelineBarrier( m_VulkanPresentQueueCmdBuffers[ Image ],
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
				nullptr, 1, &ClearToPresentBarrier );

			if( vkEndCommandBuffer(
				m_VulkanPresentQueueCmdBuffers[ Image ] ) != VK_SUCCESS )
			{
				std::cout << "[Cipher::Game::RecordComamndBuffers] "
					"<ERROR> Failed to record the command buffer" <<
					std::endl;

				return 1;
			}

		}

		return 0;
	}

	uint32_t Game::Render( )
	{
		uint32_t ImageIndex = 0UL;
		VkResult Result = vkAcquireNextImageKHR( m_VulkanDevice,
			m_VulkanSwapChain, UINT64_MAX, m_VulkanImageAvailableSemaphore,
			VK_NULL_HANDLE, &ImageIndex );

		switch( Result )
		{
			case VK_SUCCESS:
			case VK_SUBOPTIMAL_KHR:
			{
				break;
			}
			case VK_ERROR_OUT_OF_DATE_KHR:
			{
				return OnWindowSizeChanged( );
			}
			default:
			{
				std::cout << "[Cipher::Game::Render] <ERROR> "
					"Failed to acquire a swap chain image" << std::endl;

				return 1;
			}
		}

		VkPipelineStageFlags WaitStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo SubmitInfo =
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			1,
			&m_VulkanImageAvailableSemaphore,
			&WaitStageMask,
			1,
			&m_VulkanPresentQueueCmdBuffers[ ImageIndex ],
			1,
			&m_VulkanRenderingFinishedSemaphore
		};

		if( vkQueueSubmit( m_VulkanPresentQueue, 1, &SubmitInfo,
			VK_NULL_HANDLE ) != VK_SUCCESS )
		{
			return 1;
		}

		VkPresentInfoKHR PresentInfo = 
		{
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			nullptr,
			1,
			&m_VulkanRenderingFinishedSemaphore,
			1,
			&m_VulkanSwapChain,
			&ImageIndex,
			nullptr
		};
		
		Result = vkQueuePresentKHR( m_VulkanPresentQueue, &PresentInfo );

		switch( Result )
		{
			case VK_SUCCESS:
			{
				break;
			}
			case VK_ERROR_OUT_OF_DATE_KHR:
			case VK_SUBOPTIMAL_KHR:
			{
				return OnWindowSizeChanged( );
			}
			default:
			{
				std::cout << "[Cipher::Game::Render] <ERROR> "
					"Failed to present the image" << std::endl;

				return 1;
			}
		}

		return 0;
	}

	void Game::ToggleFullscreen( )
	{
		m_Fullscreen = !m_Fullscreen;

		xcb_intern_atom_cookie_t WM_State =
			GetCookieFromAtom( "_NET_WM_STATE" );
		xcb_intern_atom_cookie_t WM_State_Fullscreen =
			GetCookieFromAtom( "_NET_WM_STATE_FULLSCREEN" );

		xcb_client_message_event_t Event;

		Event.response_type = XCB_CLIENT_MESSAGE;
		Event.type = GetReplyAtomFromCookie( WM_State );
		Event.window = m_Window;
		Event.format = 32;
		Event.data.data32[ 0 ] = m_Fullscreen ? 1 : 0;
		Event.data.data32[ 1 ] = GetReplyAtomFromCookie( WM_State_Fullscreen );
		Event.data.data32[ 2 ] = XCB_ATOM_NONE;
		Event.data.data32[ 3 ] = 0;
		Event.data.data32[ 4 ] = 0;

		xcb_send_event( m_pXCBConnection, 1, m_Window,
			XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
			XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, ( const char * )( &Event ) );
	}
}

