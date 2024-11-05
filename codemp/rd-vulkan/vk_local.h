/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// Sunny: known issues/notes that I havent really looked into yet 
/*
	known issues:
-	broken capturebuffer on stopvideo avi recording
-	no surfacesprites with vbo enabled

	notes:
-	optimizations for worldeffects
-	optimizations for surfacesprites
-	glow & distortion?
-	using USE_REVERSED_DEPTH results in missing decals
-	attachments[MAX_ATTACHMENTS_IN_POOL + 1]; // +1 for SSAA? Need to disable MSAA when SSAA enabled?
-	SSAA enabled: console font scaling. need to change clientgame files. which is not an option rn
-	look into empty descriptorset bindings: descriptorBindingPartiallyBound / VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
-	default tr.projectionShadowShader->sort results in artifacts with saber & shadow sort.

	sources:
-	https://github.com/eternalcodes/EternalJK
-	https://github.com/ec-/Quake3e
-	https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition
-	https://github.com/suijingfeng/vkQuake3
*/

#pragma once

//#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#if defined (_DEBUG)
#if defined (_WIN32)
#define USE_VK_VALIDATION
#define USE_DEBUG_REPORT
//#define USE_DEBUG_UTILS
#endif
#endif

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

//#define USE_REVERSED_DEPTH
#define USE_BUFFER_CLEAR
#define USE_VK_STATS

#define	REFRACTION_EXTRACT_SCALE		2
#define NUM_COMMAND_BUFFERS				2
#define VK_NUM_BLUR_PASSES				4

#define MAX_SWAPCHAIN_IMAGES			8
#define MIN_SWAPCHAIN_IMAGES_IMM		3
#define MIN_SWAPCHAIN_IMAGES_FIFO		3
#define MIN_SWAPCHAIN_IMAGES_FIFO_0		4
#define MIN_SWAPCHAIN_IMAGES_MAILBOX	3

#define MAX_VK_SAMPLERS					32
#define MAX_VK_PIPELINES				( 1024 + 128 )
#define USE_DEDICATED_ALLOCATION
// depth + msaa + msaa-resolve + screenmap.msaa + screenmap.resolve + screenmap.depth + (bloom_extract + blur pairs + dglow_extract + blur pairs) + dglow-msaa + gamma
#define MAX_ATTACHMENTS_IN_POOL			( 9 + ( ( 1 + VK_NUM_BLUR_PASSES * 2 ) * 2 ) + 1 + 1 + 1 ) // (6+3=9: cubemap.msaa + cubemap.resolve + cubemap.depth) + gamma + refraction_extract

#define VK_DESC_STORAGE					0
#define VK_DESC_UNIFORM					1
#define VK_DESC_TEXTURE0				2
#define VK_DESC_TEXTURE1				3
#define VK_DESC_TEXTURE2				4
#define VK_DESC_FOG_COLLAPSE			5
#ifdef USE_VK_PBR
#define VK_DESC_COUNT					10	// use 11 for irradiance testing
#else
#define VK_DESC_COUNT					6
#endif

#define VK_DESC_TEXTURE_BASE			VK_DESC_TEXTURE0
#define VK_DESC_FOG_ONLY				VK_DESC_TEXTURE1
#define VK_DESC_FOG_DLIGHT				VK_DESC_TEXTURE1

//#define MIN_IMAGE_ALIGN				( 128 * 1024 )

#define VERTEX_BUFFER_SIZE				( 4 * 1024 * 1024 )
#define VERTEX_CHUNK_SIZE				( 768 * 1024)

#define XYZ_SIZE						( 4 * VERTEX_CHUNK_SIZE )
#define COLOR_SIZE						( 1 * VERTEX_CHUNK_SIZE )
#define ST0_SIZE						( 2 * VERTEX_CHUNK_SIZE )
#define ST1_SIZE						( 2 * VERTEX_CHUNK_SIZE )

#define XYZ_OFFSET						0
#define COLOR_OFFSET					( XYZ_OFFSET + XYZ_SIZE )
#define ST0_OFFSET						( COLOR_OFFSET + COLOR_SIZE )
#define ST1_OFFSET						( ST0_OFFSET + ST0_SIZE )

#define IMAGE_CHUNK_SIZE				( 32 * 1024 * 1024 )
#define MAX_IMAGE_CHUNKS				256

#define TESS_XYZ						( 1 )
#define TESS_RGBA0 						( 2 )
#define TESS_RGBA1 						( 4 )
#define TESS_RGBA2 						( 8 )
#define TESS_ST0   						( 16 )
#define TESS_ST1   						( 32 )
#define TESS_ST2   						( 64 )
#define TESS_NNN   						( 128 )
#define TESS_VPOS  						( 256 )	// uniform with eyePos
#define TESS_ENV   						( 512 )	// mark shader stage with environment mapping

#ifdef USE_VK_PBR
#define TESS_QTANGENT   				( 1024 ) // PBR shader variant, qtangent vertex attribute and eyePos uniform
#define TESS_LIGHTDIR   				( 1024 ) // PBR shader variant, qtangent vertex attribute and eyePos uniform

#define PBR_HAS_NORMALMAP				( 1 )
#define PBR_HAS_PHYSICALMAP				( 2 )
#define PBR_HAS_SPECULARMAP				( 4 )

#define PHYS_NONE						( 1 )
#define PHYS_RMO						( 2 )
#define PHYS_RMOS   					( 4 )
#define PHYS_MOXR   					( 8 )
#define PHYS_MOSR   					( 16 )
#define PHYS_ORM  						( 32 )	
#define PHYS_ORMS   					( 64 )	
#define PHYS_NORMAL   					( 128 )	
#define PHYS_NORMALHEIGHT				( 256 )	
#define PHYS_SPECGLOSS					( 512 )	

#define LIGHTDEF_USE_LIGHTMAP			0x0001
#define LIGHTDEF_USE_LIGHT_VECTOR		0x0002
#define LIGHTDEF_USE_LIGHT_VERTEX		0x0003
#define LIGHTDEF_LIGHTTYPE_MASK			LIGHTDEF_USE_LIGHTMAP | LIGHTDEF_USE_LIGHT_VECTOR | LIGHTDEF_USE_LIGHT_VERTEX

#define BUFFER_OFFSET(i)				((char *)NULL + (i))

#define ByteToFloat(a)					((float)(a) * 1.0f/255.0f)
#define FloatToByte(a)					(byte)((a) * 255.0f)

#define RGBtosRGB(a)					(((a) < 0.0031308f) ? (12.92f * (a)) : (1.055f * pow((a), 0.41666f) - 0.055f))
#define sRGBtoRGB(a)					(((a) <= 0.04045f)  ? ((a) / 12.92f) : (pow((((a) + 0.055f) / 1.055f), 2.4)) )
#endif

typedef struct textureMapType_s {
	uint32_t			type;
	const char			*suffix;
	VkComponentMapping	swizzle;
} textureMapType_t;

const textureMapType_t textureMapTypes[] = {
	{ NULL,					"",			{ VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, } },
#ifdef USE_VK_PBR
	{ PHYS_RMO,				"_rmo",		{ VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ONE,	} },
	{ PHYS_RMOS,			"_rmos",	{ VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_A, } },
	{ PHYS_MOXR,			"_moxr",	{ VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE } },
	{ PHYS_MOSR,			"_mosr",	{ VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_B } },
	{ PHYS_ORM,				"_orm",		{ VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY } },
	{ PHYS_ORMS,			"_orms",	{ VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY } },
	{ PHYS_NORMAL,			"_n",		{ VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_R } },
	{ PHYS_NORMALHEIGHT,	"_nh",		{ VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_R } },
#endif
};

// extra math
#define DotProduct4( a , b )			((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2] + (a)[3]*(b)[3])
#define VectorScale4( a , b , c )		((c)[0]=(a)[0]*(b),(c)[1]=(a)[1]*(b),(c)[2]=(a)[2]*(b),(c)[3]=(a)[3]*(b))
#define Vector4Set( v, x, y, z, w )		((v)[0]=(x),(v)[1]=(y),(v)[2]=(z),v[3]=(w))
#define Vector4Copy( a, b )				((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define LERP( a, b, w )					((a)*(1.0f-(w))+(b)*(w))
#define LUMA( r, g, b )					(0.2126f*(r)+0.7152f*(g)+0.0722f*(b))
#define EPSILON 1e-6f
#ifndef SGN
#define SGN( x )						(((x) >= 0) ? !!(x) : -1)
#endif

// shaders
#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define	GLS_SRCBLEND_BITS		    			0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define	GLS_DSTBLEND_BITS					    0x000000f0

#define GLS_BLEND_BITS							( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS )

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00001000

#define GLS_DEPTHTEST_DISABLE					0x00010000
#define GLS_DEPTHFUNC_EQUAL						0x00020000

#define GLS_ATEST_GT_0							0x10000000
#define GLS_ATEST_LT_80							0x20000000
#define GLS_ATEST_GE_80							0x40000000
#define GLS_ATEST_GE_C0							0x80000000
#define	GLS_ATEST_BITS					    	0xF0000000

#define GLS_DEFAULT								GLS_DEPTHMASK_TRUE

#ifndef GL_REPEAT
#define GL_REPEAT				0x2901
#endif
#ifndef GL_CLAMP
#define GL_CLAMP				0x2900
#endif

typedef enum {
	TYPE_COLOR_WHITE,
	TYPE_COLOR_GREEN,
	TYPE_COLOR_RED,
	TYPE_COLOR_ORANGE,
	TYPE_FOG_ONLY,
	TYPE_DOT,
	TYPE_REFRACTION,

	TYPE_SINGLE_TEXTURE_LIGHTING,
	TYPE_SINGLE_TEXTURE_LIGHTING_LINEAR,

	TYPE_SINGLE_TEXTURE_DF,
	TYPE_SINGLE_TEXTURE_IDENTITY,

	TYPE_GENERIC_BEGIN,
	TYPE_SINGLE_TEXTURE = TYPE_GENERIC_BEGIN,
	TYPE_SINGLE_TEXTURE_ENV,

	TYPE_MULTI_TEXTURE_MUL2,
	TYPE_MULTI_TEXTURE_MUL2_ENV,
	TYPE_MULTI_TEXTURE_ADD2_IDENTITY,
	TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV,
	TYPE_MULTI_TEXTURE_ADD2,
	TYPE_MULTI_TEXTURE_ADD2_ENV,

	TYPE_MULTI_TEXTURE_MUL3,
	TYPE_MULTI_TEXTURE_MUL3_ENV,
	TYPE_MULTI_TEXTURE_ADD3_IDENTITY,
	TYPE_MULTI_TEXTURE_ADD3_IDENTITY_ENV,
	TYPE_MULTI_TEXTURE_ADD3,
	TYPE_MULTI_TEXTURE_ADD3_ENV,

	TYPE_BLEND2_ADD,
	TYPE_BLEND2_ADD_ENV,
	TYPE_BLEND2_MUL,
	TYPE_BLEND2_MUL_ENV,
	TYPE_BLEND2_ALPHA,
	TYPE_BLEND2_ALPHA_ENV,
	TYPE_BLEND2_ONE_MINUS_ALPHA,
	TYPE_BLEND2_ONE_MINUS_ALPHA_ENV,
	TYPE_BLEND2_MIX_ALPHA,
	TYPE_BLEND2_MIX_ALPHA_ENV,
	TYPE_BLEND2_MIX_ONE_MINUS_ALPHA,
	TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV,

	TYPE_BLEND2_DST_COLOR_SRC_ALPHA,
	TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV,

	TYPE_BLEND3_ADD,
	TYPE_BLEND3_ADD_ENV,
	TYPE_BLEND3_MUL,
	TYPE_BLEND3_MUL_ENV,
	TYPE_BLEND3_ALPHA,
	TYPE_BLEND3_ALPHA_ENV,
	TYPE_BLEND3_ONE_MINUS_ALPHA,
	TYPE_BLEND3_ONE_MINUS_ALPHA_ENV,
	TYPE_BLEND3_MIX_ALPHA,
	TYPE_BLEND3_MIX_ALPHA_ENV,
	TYPE_BLEND3_MIX_ONE_MINUS_ALPHA,
	TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV,

	TYPE_BLEND3_DST_COLOR_SRC_ALPHA,
	TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV,

	TYPE_GENERIC_END = TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV

} Vk_Shader_Type;

// used with cg_shadows == 2
typedef enum {
	SHADOW_DISABLED,
	SHADOW_EDGES,
	SHADOW_FS_QUAD,
} Vk_Shadow_Phase;

typedef enum {
	TRIANGLE_LIST = 0,
	TRIANGLE_STRIP,
	LINE_LIST,
	POINT_LIST
} Vk_Primitive_Topology;

// Instance
extern PFN_vkGetInstanceProcAddr						qvkGetInstanceProcAddr;
extern PFN_vkCreateInstance							    qvkCreateInstance;
extern PFN_vkEnumerateInstanceExtensionProperties		qvkEnumerateInstanceExtensionProperties;
extern PFN_vkCreateDevice								qvkCreateDevice;
extern PFN_vkDestroyInstance							qvkDestroyInstance;
extern PFN_vkEnumerateDeviceExtensionProperties	    	qvkEnumerateDeviceExtensionProperties;
extern PFN_vkEnumeratePhysicalDevices					qvkEnumeratePhysicalDevices;
extern PFN_vkGetDeviceProcAddr							qvkGetDeviceProcAddr;
extern PFN_vkGetPhysicalDeviceFeatures					qvkGetPhysicalDeviceFeatures;
extern PFN_vkGetPhysicalDeviceFormatProperties			qvkGetPhysicalDeviceFormatProperties;
extern PFN_vkGetPhysicalDeviceMemoryProperties			qvkGetPhysicalDeviceMemoryProperties;
extern PFN_vkGetPhysicalDeviceProperties				qvkGetPhysicalDeviceProperties;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties	    qvkGetPhysicalDeviceQueueFamilyProperties;
extern PFN_vkDestroySurfaceKHR							qvkDestroySurfaceKHR;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR	qvkGetPhysicalDeviceSurfaceCapabilitiesKHR;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR		    qvkGetPhysicalDeviceSurfaceFormatsKHR;
extern PFN_vkGetPhysicalDeviceSurfacePresentModesKHR	qvkGetPhysicalDeviceSurfacePresentModesKHR;
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR		    qvkGetPhysicalDeviceSurfaceSupportKHR;
#ifdef USE_VK_VALIDATION
	#ifdef USE_DEBUG_REPORT
		extern PFN_vkCreateDebugReportCallbackEXT				qvkCreateDebugReportCallbackEXT;
		extern PFN_vkDestroyDebugReportCallbackEXT				qvkDestroyDebugReportCallbackEXT;
	#endif
	#ifdef USE_DEBUG_UTILS
		extern PFN_vkCreateDebugUtilsMessengerEXT				qvkCreateDebugUtilsMessengerEXT;
		extern PFN_vkDestroyDebugUtilsMessengerEXT				qvkDestroyDebugUtilsMessengerEXT;
	#endif
#endif
extern PFN_vkAllocateCommandBuffers					    qvkAllocateCommandBuffers;
extern PFN_vkAllocateDescriptorSets					    qvkAllocateDescriptorSets;
extern PFN_vkAllocateMemory							    qvkAllocateMemory;
extern PFN_vkBeginCommandBuffer					    	qvkBeginCommandBuffer;
extern PFN_vkBindBufferMemory							qvkBindBufferMemory;
extern PFN_vkBindImageMemory							qvkBindImageMemory;
extern PFN_vkCmdBeginRenderPass					    	qvkCmdBeginRenderPass;
extern PFN_vkCmdBindDescriptorSets						qvkCmdBindDescriptorSets;
extern PFN_vkCmdBindIndexBuffer					    	qvkCmdBindIndexBuffer;
extern PFN_vkCmdBindPipeline							qvkCmdBindPipeline;
extern PFN_vkCmdBindVertexBuffers						qvkCmdBindVertexBuffers;
extern PFN_vkCmdBlitImage								qvkCmdBlitImage;
extern PFN_vkCmdClearAttachments						qvkCmdClearAttachments;
extern PFN_vkCmdCopyBuffer								qvkCmdCopyBuffer;
extern PFN_vkCmdCopyBufferToImage						qvkCmdCopyBufferToImage;
extern PFN_vkCmdCopyImage								qvkCmdCopyImage;
extern PFN_vkCmdCopyImageToBuffer                       qvkCmdCopyImageToBuffer;
extern PFN_vkCmdDraw									qvkCmdDraw;
extern PFN_vkCmdDrawIndexed						    	qvkCmdDrawIndexed;
extern PFN_vkCmdEndRenderPass							qvkCmdEndRenderPass;
extern PFN_vkCmdPipelineBarrier					    	qvkCmdPipelineBarrier;
extern PFN_vkCmdPushConstants							qvkCmdPushConstants;
extern PFN_vkCmdSetDepthBias							qvkCmdSetDepthBias;
extern PFN_vkCmdSetScissor								qvkCmdSetScissor;
extern PFN_vkCmdSetViewport						    	qvkCmdSetViewport;
extern PFN_vkCreateBuffer								qvkCreateBuffer;
extern PFN_vkCreateCommandPool							qvkCreateCommandPool;
extern PFN_vkCreateDescriptorPool						qvkCreateDescriptorPool;
extern PFN_vkCreateDescriptorSetLayout					qvkCreateDescriptorSetLayout;
extern PFN_vkCreateFence								qvkCreateFence;
extern PFN_vkCreateFramebuffer							qvkCreateFramebuffer;
extern PFN_vkCreateGraphicsPipelines					qvkCreateGraphicsPipelines;
extern PFN_vkCreateImage								qvkCreateImage;
extern PFN_vkCreateImageView							qvkCreateImageView;
extern PFN_vkCreatePipelineLayout						qvkCreatePipelineLayout;
extern PFN_vkCreatePipelineCache						qvkCreatePipelineCache;
extern PFN_vkCreateRenderPass							qvkCreateRenderPass;
extern PFN_vkCreateSampler								qvkCreateSampler;
extern PFN_vkCreateSemaphore							qvkCreateSemaphore;
extern PFN_vkCreateShaderModule				    		qvkCreateShaderModule;
extern PFN_vkDestroyBuffer								qvkDestroyBuffer;
extern PFN_vkDestroyCommandPool					    	qvkDestroyCommandPool;
extern PFN_vkDestroyDescriptorPool						qvkDestroyDescriptorPool;
extern PFN_vkDestroyDescriptorSetLayout			    	qvkDestroyDescriptorSetLayout;
extern PFN_vkDestroyPipelineCache						qvkDestroyPipelineCache;
extern PFN_vkDestroyDevice								qvkDestroyDevice;
extern PFN_vkDestroyFence								qvkDestroyFence;
extern PFN_vkDestroyFramebuffer					    	qvkDestroyFramebuffer;
extern PFN_vkDestroyImage								qvkDestroyImage;
extern PFN_vkDestroyImageView							qvkDestroyImageView;
extern PFN_vkDestroyPipeline							qvkDestroyPipeline;
extern PFN_vkDestroyPipelineLayout						qvkDestroyPipelineLayout;
extern PFN_vkDestroyRenderPass							qvkDestroyRenderPass;
extern PFN_vkDestroySampler						    	qvkDestroySampler;
extern PFN_vkDestroySemaphore							qvkDestroySemaphore;
extern PFN_vkDestroyShaderModule						qvkDestroyShaderModule;
extern PFN_vkDeviceWaitIdle						    	qvkDeviceWaitIdle;
extern PFN_vkEndCommandBuffer							qvkEndCommandBuffer;
extern PFN_vkResetCommandBuffer							qvkResetCommandBuffer;
extern PFN_vkFreeCommandBuffers					    	qvkFreeCommandBuffers;
extern PFN_vkFreeDescriptorSets					    	qvkFreeDescriptorSets;
extern PFN_vkFreeMemory							    	qvkFreeMemory;
extern PFN_vkGetBufferMemoryRequirements				qvkGetBufferMemoryRequirements;
extern PFN_vkGetDeviceQueue						    	qvkGetDeviceQueue;
extern PFN_vkGetImageMemoryRequirements			    	qvkGetImageMemoryRequirements;
extern PFN_vkGetImageSubresourceLayout					qvkGetImageSubresourceLayout;
extern PFN_vkInvalidateMappedMemoryRanges				qvkInvalidateMappedMemoryRanges;
extern PFN_vkMapMemory									qvkMapMemory;
extern PFN_vkUnmapMemory                                qvkUnmapMemory;
extern PFN_vkQueueSubmit								qvkQueueSubmit;
extern PFN_vkQueueWaitIdle								qvkQueueWaitIdle;
extern PFN_vkResetDescriptorPool						qvkResetDescriptorPool;
extern PFN_vkResetFences								qvkResetFences;
extern PFN_vkUpdateDescriptorSets						qvkUpdateDescriptorSets;
extern PFN_vkWaitForFences								qvkWaitForFences;
extern PFN_vkAcquireNextImageKHR						qvkAcquireNextImageKHR;
extern PFN_vkCreateSwapchainKHR				    		qvkCreateSwapchainKHR;
extern PFN_vkDestroySwapchainKHR						qvkDestroySwapchainKHR;
extern PFN_vkGetSwapchainImagesKHR						qvkGetSwapchainImagesKHR;
extern PFN_vkQueuePresentKHR							qvkQueuePresentKHR;

extern PFN_vkGetBufferMemoryRequirements2KHR			qvkGetBufferMemoryRequirements2KHR;
extern PFN_vkGetImageMemoryRequirements2KHR				qvkGetImageMemoryRequirements2KHR;

extern PFN_vkDebugMarkerSetObjectNameEXT				qvkDebugMarkerSetObjectNameEXT;

extern PFN_vkCmdClearColorImage							qvkCmdClearColorImage;

#ifdef USE_RTX
extern PFN_vkGetPhysicalDeviceProperties2					qvkGetPhysicalDeviceProperties2;

extern PFN_vkCreateAccelerationStructureKHR					qvkCreateAccelerationStructureKHR;
extern PFN_vkDestroyAccelerationStructureKHR				qvkDestroyAccelerationStructureKHR;
extern PFN_vkCmdBuildAccelerationStructuresKHR				qvkCmdBuildAccelerationStructuresKHR;
extern PFN_vkCmdCopyAccelerationStructureKHR				qvkCmdCopyAccelerationStructureKHR;
extern PFN_vkGetAccelerationStructureDeviceAddressKHR		qvkGetAccelerationStructureDeviceAddressKHR;
extern PFN_vkCmdWriteAccelerationStructuresPropertiesKHR	qvkCmdWriteAccelerationStructuresPropertiesKHR;
extern PFN_vkGetAccelerationStructureBuildSizesKHR			qvkGetAccelerationStructureBuildSizesKHR;
extern PFN_vkGetBufferDeviceAddress							qvkGetBufferDeviceAddress;
extern PFN_vkCreateRayTracingPipelinesKHR					qvkCreateRayTracingPipelinesKHR;
extern PFN_vkCmdTraceRaysKHR								qvkCmdTraceRaysKHR;
extern PFN_vkGetRayTracingShaderGroupHandlesKHR				qvkGetRayTracingShaderGroupHandlesKHR;

extern PFN_vkCmdDispatch									qvkCmdDispatch;
extern PFN_vkCreateComputePipelines							qvkCreateComputePipelines;
extern PFN_vkCmdFillBuffer									qvkCmdFillBuffer;

extern PFN_vkCmdBeginDebugUtilsLabelEXT						qvkCmdBeginDebugUtilsLabelEXT;
extern PFN_vkCmdEndDebugUtilsLabelEXT						qvkCmdEndDebugUtilsLabelEXT;
#endif

typedef float mat4_t[16];
typedef float mat3x4_t[12];

void Matrix16Identity( mat4_t out );
void Matrix16Copy( const mat4_t in, mat4_t out );

#ifdef USE_VK_IMGUI
extern PFN_vkFlushMappedMemoryRanges					qvkFlushMappedMemoryRanges;
extern PFN_vkResetCommandPool							qvkResetCommandPool;
#endif

extern PFN_vkCmdDrawIndexedIndirect						qvkCmdDrawIndexedIndirect;

typedef union floatint_u
{
	int32_t		i;
	uint32_t	u;
	float		f;
	byte		b[4];
} floatint_t;

typedef enum {
	DEPTH_RANGE_NORMAL, // [0..1]
	DEPTH_RANGE_ZERO, // [0..0]
	DEPTH_RANGE_ONE, // [1..1]
	DEPTH_RANGE_WEAPON, // [0..0.3]
	DEPTH_RANGE_COUNT
} Vk_Depth_Range;

typedef enum {
	RENDER_PASS_SCREENMAP = 0,
	RENDER_PASS_MAIN,
	RENDER_PASS_POST_BLEND,
	RENDER_PASS_DGLOW,
	RENDER_PASS_CUBEMAP,
	RENDER_PASS_INSPECTOR,
	RENDER_PASS_REFRACTION,
	RENDER_PASS_COUNT
} renderPass_t;

typedef struct {
	uint32_t				state_bits; // GLS_XXX flags
	cullType_t				face_culling;// cullType_t

	qboolean				polygon_offset;
	qboolean				mirror;
	qboolean				vbo_ghoul2;
	qboolean				vbo_mdv;

	Vk_Shader_Type			shader_type;	
	Vk_Shadow_Phase			shadow_phase;
	Vk_Primitive_Topology	primitives;

	int fog_stage; // off, fog-in / fog-out
	int line_width;
	int abs_light;
	int allow_discard;

	int						vk_light_flags;

#ifdef USE_VK_PBR
	uint32_t				vk_pbr_flags;
#endif
} Vk_Pipeline_Def;

typedef struct VK_Pipeline {
	Vk_Pipeline_Def def;
	VkPipeline		handle[RENDER_PASS_COUNT];
} VK_Pipeline_t;

typedef struct vktcMod_s {
	vec4_t	matrix;
	vec4_t	offTurb;
} vktcMod_t;

typedef struct vktcGen_s {
	vec3_t	vector0;
	int32_t	pad0;
	vec3_t	vector1;
	int32_t	type;
} vktcGen_t;

typedef struct vkBundle_s {
	vec4_t		baseColor;
	vec4_t		vertColor;
	vktcMod_t	tcMod;
	vktcGen_t	tcGen;
	int32_t		rgbGen;
	int32_t		alphaGen;
	int32_t		numTexMods;	// make this to a specialization constant
	int32_t		pad0;
} vkBundle_t;

typedef struct vkDisintegration_s {
	vec3_t	origin;
	float	threshold;
} vkDisintegration_t;

typedef struct vkDeform_s {
	float	base;
	float	amplitude;
	float	phase;
	float	frequency;

	vec3_t	vector;
	float	time;

	int32_t	type;
	int32_t	func;
	vec2_t	pad0;
} vkDeform_t;

// this structure must be in sync with shader uniforms!
typedef struct {
	float	mvp[16];
	float	renderMode;
} pushConst;

typedef struct vkUniform_s {
	// light/env/material parameters:
	vec4_t eyePos;
	vec4_t lightPos;
	vec4_t lightColor; // rgb + 1/(r*r)
	vec4_t lightVector;

	// fog parameters:
	union {
		struct {
			vec4_t fogDistanceVector;
			vec4_t fogDepthVector;
			vec4_t fogEyeT;
			vec4_t fogColor;
		} fog;

		struct {
			vktcMod_t tcMod;
			vktcGen_t tcGen;
		} refraction;
	};
} vkUniform_t;

typedef struct vkUniformCamera_s {
	vec4_t viewOrigin;
} vkUniformCamera_t;

typedef struct vkUniformLight_s {
	vec4_t item;
} vkUniformLight_t;


typedef struct vkUniformEntity_s {
	vec4_t ambientLight;
	vec4_t directedLight;
	vec4_t lightOrigin;
	vec4_t localViewOrigin;
	mat4_t modelMatrix;
} vkUniformEntity_t;

#ifdef USE_VBO_GHOUL2
typedef struct vkUniformBones_s {
	mat3x4_t boneMatrices[72];
} vkUniformBones_t;
#endif

typedef struct vkUniformGlobal_s {
	vkBundle_t			bundle[3];
	vkDisintegration_t	disintegration;
	vkDeform_t			deform;
	float				portalRange;
	vec3_t				pad0;
	vec4_t				specularScale;	
	vec4_t				normalScale;	
} vkUniformGlobal_t;

typedef struct {
	VkSamplerAddressMode address_mode; // clamp/repeat texture addressing mode

	int gl_mag_filter; // GL_XXX mag filter
	int gl_min_filter; // GL_XXX min filter

	qboolean max_lod_1_0; // fixed 1.0 lod
	qboolean noAnisotropy;
} Vk_Sampler_Def;

struct ImageChunk_t {
	VkDeviceMemory memory;
	uint32_t used;
	uint32_t size;
	uint32_t items;
};

struct Image_Upload_Data  {
	byte *buffer;
	int buffer_size;
	int mip_levels;
	int base_level_width;
	int base_level_height;
};

extern int vkSamples;
extern int vkMaxSamples;

extern unsigned char s_intensitytable[256];
extern unsigned char s_gammatable[256];
extern unsigned char s_gammatable_linear[256];

// Vk_World contains vulkan resources/state requested by the game code.
// It is reinitialized on a map change.
typedef struct {
	// resources.
	int				num_samplers;
	VkSampler		samplers[MAX_VK_SAMPLERS];
	Vk_Sampler_Def	sampler_defs[MAX_VK_SAMPLERS];

	// memory allocations.
	int				num_image_chunks;
	ImageChunk_t	image_chunks[MAX_IMAGE_CHUNKS];

	// host visible memory used to copy image data to device local memory.
	VkBuffer		staging_buffer;
	VkDeviceMemory	staging_buffer_memory;
	VkDeviceSize	staging_buffer_size;
	byte			*staging_buffer_ptr; // pointer to mapped staging buffer

	// This flag is used to decide whether framebuffer's depth attachment should be cleared
	// with vmCmdClearAttachment (dirty_depth_attachment != 0), or it have just been
	// cleared by render pass instance clear op (dirty_depth_attachment == 0).
	int dirty_depth_attachment;

	// MVP
	float modelview_transform[16]  QALIGN(16);
} Vk_World;

#ifdef USE_RTX
typedef struct semaphore_group_s {
	VkSemaphore		transfer_finished;
	VkSemaphore		trace_finished;
	qboolean		trace_signaled;
	qboolean		prev_trace_signaled;
} semaphore_group_t;
#endif

typedef struct vk_tess_s {
	VkCommandBuffer		command_buffer;
	VkCommandBuffer		command_buffer_trace;
	VkCommandBuffer		command_buffer_trace2;
	VkCommandBuffer		command_buffer_transfer;

	VkSemaphore			image_acquired;
	VkSemaphore			rendering_finished;

#ifdef USE_RTX
	semaphore_group_t	semaphores;
#endif

	VkFence				rendering_finished_fence;
	qboolean			waitForFence;
	
	VkBuffer			vertex_buffer;
	byte				*vertex_buffer_ptr; // pointer to mapped vertex buffer
	VkDeviceSize		vertex_buffer_offset;

	VkBuffer			indirect_buffer;
	byte				*indirect_buffer_ptr; // pointer to mapped indirect buffer
	VkDeviceSize		indirect_buffer_offset;

	VkDescriptorSet		uniform_descriptor;

	VkDeviceSize		buf_offset[12];	// 10 is ok, bones & weights are ghoul2 vbo only anyway
	VkDeviceSize		vbo_offset[12];

	VkBuffer			curr_index_buffer;
	uint32_t			curr_index_offset;

	struct {
		uint32_t		start, end;
		VkDescriptorSet	current[VK_DESC_COUNT];	// 0:storage, 1:uniform, 2:color0, 3:color1, 4:color2, 5:fog, 6:brdf lut, 7:normal, 8:physical, 9:prefilterd envmap, !10:irradiance envmap
		uint32_t		offset[7]; // 0:storage, 1:data, 2: camera, 3:light 4: entity, 5:ghoul2, 6:global
	} descriptor_set;
	
	uint32_t			num_indexes; // value from most recent vk_bind_index() call
	VkPipeline			last_pipeline;
	Vk_Depth_Range		depth_range;
	VkRect2D			scissor_rect;

	uint32_t			camera_ubo_offset;
	uint32_t			light_ubo_offset;
	uint32_t			entity_ubo_offset[REFENTITYNUM_WORLD + 1];
	uint32_t			bones_ubo_offset;
} vk_tess_t;


#ifdef USE_RTX
#include "rtx/vk_rtx.h"

typedef struct shader_s shader_t;
typedef struct msurface_s msurface_t;

typedef enum {
	PIPELINE_PRIMARY_RAYS,
	PIPELINE_REFLECT_REFRACT_1,
	PIPELINE_REFLECT_REFRACT_2,
    PIPELINE_DIRECT_LIGHTING,
    PIPELINE_DIRECT_LIGHTING_CAUSTICS,
    PIPELINE_INDIRECT_LIGHTING_FIRST,
    PIPELINE_INDIRECT_LIGHTING_SECOND,

	PIPELINE_COUNT
} pipeline_index_t;
#endif

// Vk_Instance contains engine-specific vulkan resources that persist entire renderer lifetime.
// This structure is initialized/deinitialized by vk_initialize/vk_shutdown functions correspondingly.
typedef struct {
	VkInstance			instance;
	VkPhysicalDevice	physical_device;
	VkSurfaceKHR		surface;
	VkSurfaceFormatKHR	base_format;
	VkSurfaceFormatKHR	present_format;

	// to prevent changes to rd-common, move these here
	char			renderer_string[MAX_STRING_CHARS];
	char			vendor_string[MAX_STRING_CHARS];
	char			version_string[MAX_STRING_CHARS];
	char			device_extensions_string[MAX_STRING_CHARS];
	char			instance_extensions_string[MAX_STRING_CHARS];

#ifdef USE_VK_VALIDATION
	#ifdef USE_DEBUG_REPORT
		VkDebugReportCallbackEXT debug_callback;
	#endif
	#ifdef USE_DEBUG_UTILS
		VkDebugUtilsMessengerEXT debug_utils_messenger;
	#endif
#endif

	uint32_t		queue_family_index;
	VkDevice		device;
	VkQueue			queue;
	VkQueue			queue_new;

	VkPhysicalDeviceMemoryProperties devMemProperties;

#ifdef USE_RTX
	uint64_t		frame_counter;
	vkgeometry_t	geometry;

	qboolean		rtxSupport;	// device supports raytracing
	qboolean		rtxActive;	// raytracing on
	qboolean		rtx_surf_is_hdr;

	VkPhysicalDeviceProperties							props;
	VkPhysicalDeviceProperties2							props2;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR		ray_pipeline_properties;
	VkPhysicalDeviceAccelerationStructurePropertiesKHR	accel_struct_properties;

	VkExtent2D		extent_screen_images;
	VkExtent2D		extent_render;
	VkExtent2D		extent_render_prev;
	VkExtent2D		extent_unscaled;
	VkExtent2D		extent_taa_images;
	VkExtent2D		extent_taa_output;

	uint32_t		shaderGroupHandleSize;
	uint32_t		shaderGroupBaseAlignment;
	uint32_t		minAccelerationStructureScratchOffsetAlignment;
	uint32_t		device_count;
	uint32_t		gpu_slice_width;
	uint32_t		gpu_slice_width_prev;

	vec2_t			taa_samples[NUM_TAA_SAMPLES];

	bool			supports_fp16;
	int				effective_aa_mode;

	cluster_t		*clusterList;
	int				numFixedCluster;
	int				numClusters;
	int				clusterBytes;
	int				numMaxClusters;
	const byte		*vis;

	char			cluster_debug_mask[VIS_MAX_BYTES];
	int				cluster_debug_index;

	qboolean		worldASInit;

	vkbuffer_t		buf_shader_binding_table;

	VkPipelineLayout rt_pipeline_layout;
	VkPipeline		rt_pipelines[PIPELINE_COUNT];
	VkShaderModule  shader_modules[6];

	vkpipeline_t	asvgf_pipeline[ASVGF_NUM_PIPELINES];
	vkshader_t		*asvgf_shader[NUM_ASVGF_SHADER_MODULES];

	vkpipeline_t	tonemap_pipeline[TM_NUM_PIPELINES];
	vkshader_t		*tonemap_shader[TM_NUM_SHADERS];

	vkpipeline_t	physical_sky_pipeline[PHYSICAL_SKY_NUM_PIPELINES];
	vkshader_t		*physical_sky_shader[NUM_PHYSICAL_SKY_SHADER_MODULES];

	VkPipeline		pipeline_final_blit;
	vkshader_t		*final_blit_shader_frag;
	
	vkimage_t		img_envmap;
	vkimage_t		img_blue_noise;
	vkimage_t		img_physical_sky[NUM_PHYSICAL_SKY_IMAGES];
	vkimage_t		img_rtx[NUM_RTX_IMAGES];

	VkDescriptorPool desc_pool_textures;

	VkDescriptorSetLayout       desc_set_layout_textures;
	VkDescriptorSet             desc_set_textures_even;
	VkDescriptorSet             desc_set_textures_odd;

	VkSampler       tex_sampler, 
                    tex_sampler_nearest,
                    tex_sampler_nearest_mipmap_aniso,
                    tex_sampler_linear_clamp;

	VkDescriptorSetLayout       desc_set_layout_ubo;
	VkDescriptorSet             desc_set_ubo;

	vkdescriptor_t	rtxDescriptor		[VK_MAX_SWAPCHAIN_SIZE];
	vkdescriptor_t	computeDescriptor	[VK_MAX_SWAPCHAIN_SIZE];
	vkdescriptor_t	imageDescriptor;

	struct {
		VkDescriptorPool		pool;
		VkDescriptorSetLayout	layout;
		vkbuffer_t				buffer_vertex;

		struct {
			VkDescriptorSet		vbos;
#ifdef USE_RTX_GLOBAL_MODEL_VBO
			VkDescriptorSet		ibos;
#endif
		} descriptor;

		struct {
			vk_blas_t	dynamic[VK_MAX_SWAPCHAIN_SIZE];
			vk_blas_t	transparent_models[VK_MAX_SWAPCHAIN_SIZE];
			vk_blas_t	viewer_models[VK_MAX_SWAPCHAIN_SIZE];
			vk_blas_t	viewer_weapon[VK_MAX_SWAPCHAIN_SIZE];
			vk_blas_t	explosions[VK_MAX_SWAPCHAIN_SIZE];
		} blas;
	} model_instance;

	Shadowmap		precomputed_shadowmapdata;

	struct {
		// world static (world geometry that does not requiere updates)
		vk_blas_t	world;
		vk_blas_t	world_transparent;
	} blas_static;

	struct {
		// world dynamic data (world geometry that requieres texture/color or uv updates)
		vk_blas_t	data_world;
		vk_blas_t	data_world_transparent;

		// world dynamic AS (world geometry that requieres AS updates)
		vk_blas_t	as_world[VK_MAX_SWAPCHAIN_SIZE];
		vk_blas_t	as_world_transparent[VK_MAX_SWAPCHAIN_SIZE];
	} blas_dynamic;

	struct {
		uint32_t	index_offset;
		uint32_t	vertex_offset;
		uint32_t	num_vertices;
		shader_t	*shader;
		msurface_t	*surf;
		int			cluster;
		int			fogindex;
	} updateDataOffsetXYZ[3500];
	uint32_t		updateDataOffsetXYZCount;

	struct {
		uint32_t	index_offset;
		uint32_t	vertex_offset;
		uint32_t	countXYZ;
		uint32_t	num_vertices;
		shader_t	*shader;
		msurface_t	*surf;
		int			cluster;
	} updateASOffsetXYZ[3500];
	uint32_t		updateASOffsetXYZCount;

	// Top AS (Buffers) for each swapchain image
	vk_tlas_t		tlas_geometry[VK_MAX_SWAPCHAIN_SIZE];

	// stores offset and stuff for in shader lookup
	vkbuffer_t		buf_instances[VK_MAX_SWAPCHAIN_SIZE];
	
	vkbuffer_t		buf_readback;
	vkbuffer_t		buf_readback_staging[VK_MAX_SWAPCHAIN_SIZE];

	vkbuffer_t		buf_light;
	vkbuffer_t		buf_light_staging[VK_MAX_SWAPCHAIN_SIZE];
	vkbuffer_t		buf_light_stats[NUM_LIGHT_STATS_BUFFERS];
	vkbuffer_t		buf_light_counts_history[LIGHT_COUNT_HISTORY];

	vkbuffer_t		buf_tonemap;

	vkbuffer_t		buf_sun_color;

	vkbuffer_t		buf_accel_scratch;
	size_t			scratch_buf_ptr;

	vkUniformRTX_t	uniform_buffer;
	InstanceBuffer	uniform_instance_buffer;
#endif

	VkSwapchainKHR	swapchain;
	uint32_t		swapchain_image_count;
	uint32_t		swapchain_image_index;
	VkImage			swapchain_images[MAX_SWAPCHAIN_IMAGES];
	VkImageView		swapchain_image_views[MAX_SWAPCHAIN_IMAGES];

	VkDeviceMemory	image_memory[MAX_ATTACHMENTS_IN_POOL];
	uint32_t		image_memory_count;

	VkCommandPool	command_pool;

	VkDescriptorSet	color_descriptor;
	VkDescriptorSet bloom_image_descriptor[1 + VK_NUM_BLUR_PASSES * 2];
	VkDescriptorSet dglow_image_descriptor[1 + VK_NUM_BLUR_PASSES * 2];
	
	VkImage			depth_image;
	VkImageView		depth_image_view;

	VkImage			msaa_image;
	VkImageView		msaa_image_view;

	VkImage			color_image;
	VkImageView		color_image_view;
	
	VkImage			refraction_extract_image;
	VkImageView		refraction_extract_image_view;
	VkDescriptorSet	refraction_extract_descriptor;

	VkImage			bloom_image[1 + VK_NUM_BLUR_PASSES * 2];
	VkImageView		bloom_image_view[1 + VK_NUM_BLUR_PASSES * 2];

	VkImage			dglow_image[1 + VK_NUM_BLUR_PASSES * 2];
	VkImageView		dglow_image_view[1 + VK_NUM_BLUR_PASSES * 2];
	VkImage			dglow_msaa_image;
	VkImageView		dglow_msaa_image_view;

	VkImage			gamma_image;
	VkImageView		gamma_image_view;


#ifdef VK_PBR_BRDFLUT
	VkImage			brdflut_image;
	VkImageView		brdflut_image_view;
	VkDescriptorSet brdflut_image_descriptor;
#endif

	// screenmap
	struct {
		VkImage			depth_image;
		VkImageView		depth_image_view;

		VkImage			color_image_msaa;
		VkImageView		color_image_view_msaa;

		VkDescriptorSet color_descriptor;
		VkImage			color_image;
		VkImageView		color_image_view;
	} screenMap;

	// cubemap
	struct {
		VkImage			depth_image;
		VkImageView		depth_image_view;

		VkDescriptorSet color_descriptor;
		VkImage			color_image;
		VkImageView		color_image_view[7];
	} cubeMap;

	vk_tess_t tess[NUM_COMMAND_BUFFERS], *cmd;
	int cmd_index;

	// render passes
	struct {
		VkRenderPass main;
		VkRenderPass gamma;
		VkRenderPass inspector;
		VkRenderPass screenmap;
		VkRenderPass capture;
#ifdef VK_PBR_BRDFLUT
		VkRenderPass brdflut;
#endif
		VkRenderPass cubemap;

		struct {
			VkRenderPass extract;
		} refraction;

		struct {
			VkRenderPass blur[VK_NUM_BLUR_PASSES * 2];
			VkRenderPass extract;
			VkRenderPass blend;
		} bloom;

		struct {
			VkRenderPass blur[VK_NUM_BLUR_PASSES * 2];
			VkRenderPass extract;
			VkRenderPass blend;
		} dglow;
#ifdef USE_RTX
		struct {
			VkRenderPass blit;
			VkRenderPass blend;
		} rtx_final_blit;
#endif
	} render_pass;

	struct {
		VkImage		image;
		VkImageView image_view;
	} capture;


	// framebuffers
	struct {
		VkFramebuffer main[MAX_SWAPCHAIN_IMAGES];
		VkFramebuffer inspector[MAX_SWAPCHAIN_IMAGES];
		VkFramebuffer gamma;
		VkFramebuffer screenmap;
		VkFramebuffer capture;
#ifdef VK_PBR_BRDFLUT
		VkFramebuffer brdflut;
#endif
#ifdef USE_RTX
		VkFramebuffer rtx_final_blit;
#endif
		VkFramebuffer cubemap[6];

		struct {
			VkFramebuffer extract;
		} refraction;

		struct {
			VkFramebuffer blur[VK_NUM_BLUR_PASSES * 2];
			VkFramebuffer extract;
		} bloom;

		struct {
			VkFramebuffer blur[VK_NUM_BLUR_PASSES * 2];
			VkFramebuffer extract;
		} dglow;
	} framebuffers;

	struct {
		VkBuffer		vertex_buffer;
		VkDeviceMemory	buffer_memory;
	} vbo;

	// statistics
	struct {
		VkDeviceSize	vertex_buffer_max;
		uint32_t		push_size;
		uint32_t		push_size_max;
	} stats;

	// host visible memory that holds vertex, index and uniform data
	VkDeviceMemory		geometry_buffer_memory;
	VkDeviceSize		geometry_buffer_size;
	VkDeviceSize		geometry_buffer_size_new;

	// host visible memory that holds indirect drawdata
	VkDeviceMemory		indirect_buffer_memory;
	VkDeviceSize		indirect_buffer_size;
	VkDeviceSize		indirect_buffer_size_new;

	VkDescriptorPool		descriptor_pool;
	VkDescriptorSetLayout	set_layout_sampler;
	VkDescriptorSetLayout	set_layout_uniform;
	VkDescriptorSetLayout	set_layout_storage;

	// pipeline(s)
	VkPipelineLayout pipeline_layout;
	VkPipelineLayout pipeline_layout_storage;
	VkPipelineLayout pipeline_layout_post_process;	// post-processing
	VkPipelineLayout pipeline_layout_blend;			// post-processing
#ifdef VK_PBR_BRDFLUT
	VkPipelineLayout pipeline_layout_brdflut;
#endif

	VkPipeline gamma_pipeline;
	VkPipeline bloom_extract_pipeline;
	VkPipeline bloom_blur_pipeline[VK_NUM_BLUR_PASSES * 2]; // horizontal & vertical pairs
	VkPipeline bloom_blend_pipeline;
	VkPipeline capture_pipeline;
	VkPipeline dglow_blur_pipeline[VK_NUM_BLUR_PASSES * 2]; // horizontal & vertical pairs
	VkPipeline dglow_blend_pipeline;
#ifdef VK_PBR_BRDFLUT
	VkPipeline brdflut_pipeline;
#endif

	VkPipeline refraction_capture_pipeline;

	// Standard pipeline(s)
	struct  {
		uint32_t skybox_pipeline;
		uint32_t worldeffect_pipeline[2];

		// dim 0: 0 - front side, 1 - back size
		// dim 1: 0 - normal view, 1 - mirror view
		uint32_t shadow_volume_pipelines[2][2];
		uint32_t shadow_finish_pipeline;

		// dim 0 is based on fogPass_t: 0 - corresponds to FP_EQUAL, 1 - corresponds to FP_LE.
		// dim 1 is directly a cullType_t enum value.
		// dim 2 is a polygon offset value (0 - off, 1 - on).
		uint32_t fog_pipelines[3][2][3][2];

#ifdef USE_PMLIGHT
		// cullType[3], polygonOffset[2], fogStage[2], absLight[2]
		uint32_t dlight_pipelines_x[3][2][2][2];
		uint32_t dlight1_pipelines_x[3][2][2][2];
#endif

		// debug-visualization pipelines
		uint32_t tris_debug_pipeline;
		uint32_t tris_mirror_debug_pipeline;
		uint32_t tris_debug_green_pipeline;
		uint32_t tris_mirror_debug_green_pipeline;
		uint32_t tris_debug_red_pipeline;
		uint32_t tris_mirror_debug_red_pipeline;

		uint32_t normals_debug_pipeline;
		uint32_t surface_debug_pipeline_solid;
		uint32_t surface_debug_pipeline_outline;
		uint32_t images_debug_pipeline;
		uint32_t surface_beam_pipeline;
		uint32_t surface_axis_pipeline;
		uint32_t dot_pipeline;

#ifdef USE_VK_IMGUI
		uint32_t inspector_object_debug_pipeline[2];
#endif
	} std_pipeline;

	VK_Pipeline_t	pipelines[MAX_VK_PIPELINES];
	VkPipelineCache pipelineCache;

	uint32_t	pipelines_count;
	uint32_t	pipelines_world_base;
	int32_t		pipeline_create_count;
	
	struct {
		VkBuffer		buffer;
		byte			*buffer_ptr;
		VkDeviceMemory	memory;
		VkDescriptorSet	descriptor;
	} storage;

	uint32_t storage_alignment;
	uint32_t uniform_alignment;

	uint32_t uniform_item_size;
	uint32_t uniform_camera_item_size;
	uint32_t uniform_global_item_size;
	uint32_t uniform_light_item_size;
	uint32_t uniform_entity_item_size;
	uint32_t uniform_bones_item_size;

	uint32_t ghoul2_vbo_stride;
	uint32_t mdv_vbo_stride;

	// shader modules.
	struct {
		// vbo 0: cpu or vbo world, 1: vbo ghoul2, 2: vbo mdv

		struct {	
			VkShaderModule gen[3][2][4][3][2][2][2]; // vbo[0,1], pbr[0,1], tx[0,1,2], cl[0,1] env0[0,1] fog[0,1]
			VkShaderModule light[2]; // fog[0,1]
			VkShaderModule gen0_ident;
		}	vert;

		struct {
			VkShaderModule gen0_ident;
			VkShaderModule gen0_df;
			VkShaderModule gen[2][4][3][2][2]; // pbr[0,1], tx[0,1,2] cl[0,1] fog[0,1]
			VkShaderModule light[2][2]; // linear[0,1] fog[0,1]
		}	frag;

		VkShaderModule dot_fs;
		VkShaderModule dot_vs;

		VkShaderModule gamma_fs;
		VkShaderModule gamma_vs;

		VkShaderModule fog_vs[2];
		VkShaderModule fog_fs;

		VkShaderModule color_vs[2];
		VkShaderModule color_fs;

		VkShaderModule bloom_fs;
		VkShaderModule blur_fs;
		VkShaderModule blend_fs;
#ifdef VK_PBR_BRDFLUT
		VkShaderModule brdflut_fs;
#endif
		VkShaderModule filtercube_vs;
		VkShaderModule filtercube_gm;
		VkShaderModule irradiancecube_fs;
		VkShaderModule prefilterenvmap_fs;
		VkShaderModule refraction_vs[3];
		VkShaderModule refraction_fs;
	} shaders;

	uint32_t frame_count;
	qboolean shaderStorageImageMultisample;
	qboolean samplerAnisotropy;
	qboolean fragmentStores;
	qboolean dedicatedAllocation;
	qboolean debugMarkers;
	qboolean wideLines;
	qboolean fastSky;		// requires VK_IMAGE_USAGE_TRANSFER_DST_BIT
	qboolean fboActive;
	qboolean blitEnabled;

	qboolean vboWorldActive;
	qboolean vboGhoul2Active;
	qboolean vboMdvActive;

	qboolean normalMappingActive;
	qboolean specularMappingActive;

	qboolean useFastLight;

	float maxAnisotropy;
	float maxLod;

	VkFormat depth_format;
	VkFormat color_format;
	VkFormat bloom_format;
	VkFormat capture_format;
	VkFormat compressed_format;

	VkImageLayout initSwapchainLayout;

	qboolean active;
	qboolean msaaActive;
	qboolean bloomActive;
	qboolean dglowActive;
#ifdef VK_CUBEMAP
	qboolean cubemapActive;
#endif
	qboolean refractionActive;

	qboolean	offscreenRender;
	qboolean	windowAdjusted;
	int			blitX0;
	int			blitY0;
	int			blitFilter;

	uint32_t screenMapWidth;
	uint32_t screenMapHeight;
	uint32_t screenMapSamples;

	int		ctmu;	// current texture index

	uint32_t renderWidth;
	uint32_t renderHeight;

	float	renderScaleX;
	float	renderScaleY;
	float	yscale2D;
	float	xscale2D;

	renderPass_t renderPassIndex;

	uint32_t image_chunk_size;
	uint32_t maxBoundDescriptorSets;
	
	struct {
		VkDescriptorSet *descriptor;
		uint32_t descriptor_size;
	} debug;

} Vk_Instance;

extern Vk_Instance	vk;				// shouldn't be cleared during ref re-init
extern Vk_World		vk_world;		// this data is cleared during ref re-init

// ...
qboolean	vk_surface_format_color_depth( VkFormat format, int* r, int* g, int* b );
void		vk_set_fastsky_color( void );
void		vk_create_window( void );
void		vk_initialize( void );
void		vk_shutdown( void );
void		vk_init_library( void );
void		vk_deinit_library( void );
void		get_viewport( VkViewport *viewport, Vk_Depth_Range depth_range );
void		get_viewport_rect( VkRect2D *r );
void		get_scissor_rect( VkRect2D *r );
void		myGlMultMatrix( const float *a, const float *b, float *out );
qboolean	vk_select_surface_format( VkPhysicalDevice physical_device, VkSurfaceKHR surface );
void		vk_setup_surface_formats( VkPhysicalDevice physical_device );
qboolean	R_CanMinimize( void );

// pipeline
void		vk_create_pipelines(void);
void		vk_alloc_persistent_pipelines( void );
void		vk_create_descriptor_layout( void );
void		vk_create_pipeline_layout( void );
void		vk_destroy_pipelines( qboolean reset );
void		vk_update_post_process_pipelines( void );

// swapchain
void		vk_restart_swapchain( const char *funcname );
void		vk_destroy_swapchain( void );
void		vk_create_swapchain( VkPhysicalDevice physical_device, VkDevice device,
	VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format, VkSwapchainKHR *swapchain);

// frame
void		vk_begin_frame( void );
void		vk_end_frame( void );
void		vk_present_frame( void );
void		vk_create_framebuffers( void );
void		vk_destroy_framebuffers( void );
void		vk_create_sync_primitives( void );
void		vk_destroy_sync_primitives( void );
void		vk_release_geometry_buffers( void );
void		vk_release_resources( void );
void		vk_read_pixels( byte *buffer, uint32_t width, uint32_t height );

// vbo
void		vk_release_vbo( void );
void		vk_release_model_vbo( void );
qboolean	vk_alloc_vbo( const char *name, const byte *vbo_data, int vbo_size );
void		VBO_PrepareQueues( void );
void		VBO_RenderIBOItems( void );
void		VBO_ClearQueue( void );

int			get_mdv_stride( void );
int			get_mdxm_stride( void );

// shader
void		vk_create_shader_modules( void );
void		vk_destroy_shader_modules( void );

// command
VkCommandBuffer vk_begin_command_buffer( void );
void		vk_end_command_buffer( VkCommandBuffer command_buffer );
void		vk_create_command_pool( void );
void		vk_create_command_buffer( void );
void vk_record_image_layout_transition( VkCommandBuffer cmdBuf, VkImage image, 
	VkImageAspectFlags image_aspect_flags, 
	VkImageLayout old_layout, VkImageLayout new_layout );


// memory
uint32_t	vk_find_memory_type( uint32_t memory_type_bits, VkMemoryPropertyFlags properties );
uint32_t	vk_find_memory_type_lazy( uint32_t memory_type_bits, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags *outprops );

// attachment
void		vk_create_attachments( void );
void		vk_destroy_attachments( void );
void		vk_update_attachment_descriptors( void );
void		vk_clear_color_attachments( const vec4_t color );
void		vk_clear_depthstencil_attachments( qboolean clear_stencil );

// shade geometry
void		vk_set_2d( void );
void		vk_set_depthrange( const Vk_Depth_Range depthRange );


pushConst	*vk_get_push_constant();

void		vk_update_mvp( const float *m );
void		vk_wait_idle( void );
void		vk_create_render_passes( void );
void		vk_destroy_render_passes( void );
void		vk_select_texture( const int index );
uint32_t	vk_tess_index( uint32_t numIndexes, const void *src );
void		vk_bind_index_buffer( VkBuffer buffer, uint32_t offset );
void		vk_bind_index( void );
void		vk_bind_index_ext( const int numIndexes, const uint32_t *indexes );
void		vk_bind_pipeline( uint32_t pipeline );
void		vk_draw_geometry( Vk_Depth_Range depRg, qboolean indexed );
void		vk_bind_geometry( uint32_t flags );
void		vk_bind_geometry_buffer( void );
void		vk_bind_lighting( int stage, int bundle );
void		vk_reset_descriptor( int index);
void		vk_update_uniform_descriptor( VkDescriptorSet descriptor, VkBuffer buffer );
void		vk_create_storage_buffer( uint32_t size );
void		vk_update_descriptor_offset( int index, uint32_t offset );
void		vk_init_descriptors( void );
void		vk_create_vertex_buffer( VkDeviceSize size );
void		vk_create_indirect_buffer( VkDeviceSize size );
VkBuffer	vk_get_vertex_buffer( void );
void		vk_update_descriptor( int tmu, VkDescriptorSet curDesSet );
uint32_t	vk_find_pipeline_ext( uint32_t base, const Vk_Pipeline_Def *def, qboolean use );
VkPipeline	vk_gen_pipeline( uint32_t index );
void		vk_end_render_pass( void );
void		vk_begin_main_render_pass( void );
void		vk_get_pipeline_def( uint32_t pipeline, Vk_Pipeline_Def *def );
uint32_t	vk_append_uniform( const void *uniform, size_t size, uint32_t min_offset );

// image process
void		R_SetColorMappings( void );
void		R_LightScaleTexture( byte *in, int inwidth, int inheight, qboolean only_gamma );
void		ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight );
void		R_BlendOverTexture( unsigned char *data, const uint32_t pixelCount, const uint32_t l );
void		R_MipMap( byte *out, byte *in, int width, int height );
void		R_MipMap2( unsigned* const out, unsigned* const in, int inWidth, int inHeight );

// image
void		vk_texture_mode( const char *string, const qboolean init );
VkSampler	vk_find_sampler( const Vk_Sampler_Def *def );
void		vk_delete_textures( void );
void		vk_record_buffer_memory_barrier( VkCommandBuffer cb, VkBuffer buffer, 
	VkDeviceSize size, VkPipelineStageFlags src_stages, VkPipelineStageFlags dst_stages, 
	VkAccessFlags src_access, VkAccessFlags dst_access );

// post-processing
void		vk_begin_post_blend_render_pass( VkRenderPass renderpass, qboolean clearValues );

// bloom
void		vk_begin_bloom_extract_render_pass( void );
void		vk_begin_bloom_blur_render_pass( uint32_t index );
qboolean	vk_bloom( void );

// refraction
void		vk_refraction_extract( void );
void		vk_begin_post_refraction_extract_render_pass( void );

// dynamic glow
void		vk_begin_dglow_extract_render_pass( void );
void		vk_begin_dglow_blur_render_pass( uint32_t index );
qboolean	vk_begin_dglow_blur( void );

// cubemap
#ifdef VK_CUBEMAP
void		vk_begin_cubemap_render_pass( void );
void		vk_create_cubemap_prefilter( void );
void		vk_destroy_cubemap_prefilter( void );
#endif

#ifdef VK_PBR_BRDFLUT
void		vk_create_brfdlut( void );
#endif

// info
const char	*vk_format_string( VkFormat format );
const char	*vk_result_string( VkResult code );
const char	*vk_shadertype_string( Vk_Shader_Type code );
const char	*renderer_name( const VkPhysicalDeviceProperties *props );
void		vk_get_vulkan_properties( VkPhysicalDeviceProperties *props );
void		vk_info_f( void );
void		GfxInfo_f( void );

// debug
void		vk_set_object_name( uint64_t obj, const char *objName, VkDebugReportObjectTypeEXT objType );
#define		VK_SET_OBJECT_NAME( obj, objName, objType) vk_set_object_name( (uint64_t)( obj ), ( objName ), ( objType ) );

#define VK_CHECK( function_call ) { \
	VkResult result = function_call; \
	if ( result < 0 ) \
		vk_debug( "Vulkan: error %s returned by %s \n", vk_result_string( result ), #function_call ); \
}

void		vk_debug( const char *msg, ... );
void		R_DebugGraphics( void );


#ifdef USE_VK_IMGUI
struct ImGuiGlobal {
	qboolean	skip; // skip rendering on map loading
	qboolean	input_state;
	int			profiler_index;
};

extern ImGuiGlobal	imguiGlobal;

#ifdef USE_RTX
VkResult	vkpt_final_blit_filtered(VkCommandBuffer cmd_buf);
void		vk_rtx_create_final_blit_pipeline( void );
void		vk_imgui_bind_rtx_cvars( void );
#endif

void		vk_imgui_initialize( void );
void		vk_imgui_shutdown( void );
void		vk_imgui_begin_frame( void );
void		vk_imgui_draw( void );
int			vk_imgui_get_render_mode( void );

void		vk_imgui_clear_inspector( qboolean reset );
void		vk_imgui_swapchain_restarted();

void		vk_imgui_reload_shader_editor( qboolean close );
int			vk_imgui_get_shader_editor_index( void );

void		R_GetImGuiContext( void **context );
uint64_t	R_GetImGuiTexture( qhandle_t hShader );

// profiler
#define		RGBA_LE(col) (((col & 0xff000000) >> (3 * 8)) + ((col & 0x00ff0000) >> (1 * 8)) + ((col & 0x0000ff00) << (1 * 8)) + ((col & 0x000000ff) << (3 * 8)))

struct profilerTask_s
{
	double		startTime;
	double		endTime;
	char		name[MAX_QPATH];
	uint32_t	color;
};

double	vk_imgui_profiler_get_time_secconds();
void	vk_imgui_profiler_begin_frame( void );
void	vk_imgui_profiler_end_frame( void );
size_t	vk_imgui_profiler_start_task( const char *name, uint32_t color );
void	vk_imgui_profiler_end_task( uint32_t task_id );
size_t	vk_imgui_profiler_insert_task( const char *name, uint32_t color, double startTime, double endTime );
#endif

#ifdef USE_VK_VALIDATION
	void	vk_create_debug_callback( void );

#ifdef USE_DEBUG_UTILS
	void	vk_create_debug_utils( VkDebugUtilsMessengerCreateInfoEXT &desc );
#endif
#endif