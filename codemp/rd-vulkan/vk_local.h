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

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif
//#define USE_REVERSED_DEPTH
//#define USE_BUFFER_CLEAR

#define NUM_COMMAND_BUFFERS				2
#define VK_NUM_BLOOM_PASSES				4

#define MAX_SWAPCHAIN_IMAGES			4
#define MIN_SWAPCHAIN_IMAGES_IMM		3
#define MIN_SWAPCHAIN_IMAGES_FIFO		3
#define MIN_SWAPCHAIN_IMAGES_MAILBOX	3

#define MAX_VK_SAMPLERS					32
#define MAX_VK_PIPELINES				( 1024 + 128 )
#define USE_DEDICATED_ALLOCATION
// depth + msaa + msaa-resolve + screenmap.msaa + screenmap.resolve + screenmap.depth + bloom_extract + blur pairs
#define MAX_ATTACHMENTS_IN_POOL			( 6 + 1 + VK_NUM_BLOOM_PASSES * 2 ) 

#define VK_SAMPLER_LAYOUT_BEGIN			2
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
#define MAX_IMAGE_CHUNKS				48

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

// extra math
#define DotProduct4( a , b )			((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2] + (a)[3]*(b)[3])
#define VectorScale4( a , b , c )		((c)[0]=(a)[0]*(b),(c)[1]=(a)[1]*(b),(c)[2]=(a)[2]*(b),(c)[3]=(a)[3]*(b))
#define LERP( a, b, w )					((a)*(1.0f-(w))+(b)*(w))
#define LUMA( r, g, b )					(0.2126f*(r)+0.7152f*(g)+0.0722f*(b))
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
	TYPE_FOG_ONLY,
	TYPE_DOT,

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
#ifndef NDEBUG
extern PFN_vkCreateDebugReportCallbackEXT				qvkCreateDebugReportCallbackEXT;
extern PFN_vkDestroyDebugReportCallbackEXT				qvkDestroyDebugReportCallbackEXT;
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
	RENDER_PASS_POST_BLOOM,
	RENDER_PASS_COUNT
} renderPass_t;

typedef struct {
	uint32_t				state_bits; // GLS_XXX flags
	cullType_t				face_culling;// cullType_t

	VkBool32				polygon_offset;
	VkBool32				mirror;

	Vk_Shader_Type			shader_type;	
	Vk_Shadow_Phase			shadow_phase;
	Vk_Primitive_Topology	primitives;

	int fog_stage; // off, fog-in / fog-out
	int line_width;
	int abs_light;
	int allow_discard;
} Vk_Pipeline_Def;

typedef struct VK_Pipeline {
	Vk_Pipeline_Def def;
	VkPipeline		handle[RENDER_PASS_COUNT];
} VK_Pipeline_t;

// this structure must be in sync with shader uniforms!
typedef struct vkUniform_s {
	// vertex shader reference
	vec4_t eyePos;
	vec4_t lightPos;

	// vertex - fog parameters
	vec4_t fogDistanceVector;
	vec4_t fogDepthVector;
	vec4_t fogEyeT;

	// fragment shader reference
	vec4_t lightColor; // rgb + 1/(r*r)
	vec4_t fogColor;

	// fragment - linear dynamic light
	vec4_t lightVector;
} vkUniform_t;

typedef struct {
	VkSamplerAddressMode address_mode; // clamp/repeat texture addressing mode

	int gl_mag_filter; // GL_XXX mag filter
	int gl_min_filter; // GL_XXX min filter

	qboolean max_lod_1_0; // fixed 1.0 lod
	qboolean noAnisotropy;

	VkBool32 mipmap;
} Vk_Sampler_Def;

struct ImageChunk_t {
	VkDeviceMemory memory;
	uint32_t used;
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
	//VkBool32 dirty_depth_attachment;
	int dirty_depth_attachment;

	// MVP
	float modelview_transform[16]  QALIGN(16);
} Vk_World;

typedef struct vk_tess_s {
	VkCommandBuffer		command_buffer;

	VkSemaphore			image_acquired;
	VkSemaphore			rendering_finished;
	VkFence				rendering_finished_fence;
	qboolean			waitForFence;
	
	VkBuffer			vertex_buffer;
	byte				*vertex_buffer_ptr; // pointer to mapped vertex buffer
	VkDeviceSize		vertex_buffer_offset;

	VkDescriptorSet		uniform_descriptor;
	uint32_t			uniform_read_offset;
	VkDeviceSize		buf_offset[8];
	VkDeviceSize		vbo_offset[8];

	VkBuffer			curr_index_buffer;
	uint32_t			curr_index_offset;

	struct {
		uint32_t		start, end;
		VkDescriptorSet	current[7];	// 0:storage, 1:uniform, 2:color0, 3:color1, 4:color2, 5:fog
		uint32_t		offset[2];	// 0 (uniform) and 5 (storage)
	} descriptor_set;
	
	uint32_t			num_indexes; // value from most recent vk_bind_index() call
	VkPipeline			last_pipeline;
	Vk_Depth_Range		depth_range;
	VkRect2D			scissor_rect;
} vk_tess_t;

// Vk_Instance contains engine-specific vulkan resources that persist entire renderer lifetime.
// This structure is initialized/deinitialized by vk_initialize/vk_shutdown functions correspondingly.
typedef struct {
	VkInstance				instance;
	VkPhysicalDevice		physical_device;
	VkSurfaceKHR			surface;
	VkSurfaceFormatKHR		base_format;
	VkSurfaceFormatKHR		present_format;

#ifndef NDEBUG
	VkDebugReportCallbackEXT debug_callback;
#endif

	uint32_t		queue_family_index;
	VkDevice		device;
	VkQueue			queue;

	VkPhysicalDeviceMemoryProperties devMemProperties;

	VkSwapchainKHR	swapchain;
	uint32_t		swapchain_image_count;
	uint32_t		swapchain_image_index;
	VkImage			swapchain_images[MAX_SWAPCHAIN_IMAGES];
	VkImageView		swapchain_image_views[MAX_SWAPCHAIN_IMAGES];

	VkDeviceMemory	image_memory[MAX_ATTACHMENTS_IN_POOL];
	uint32_t		image_memory_count;

	VkCommandPool	command_pool;

	VkDescriptorSet	color_descriptor;
	VkDescriptorSet bloom_image_descriptor[1 + VK_NUM_BLOOM_PASSES * 2];
	
	VkImage			depth_image;
	VkImageView		depth_image_view;

	VkImage			msaa_image;
	VkImageView		msaa_image_view;

	VkImage			color_image;
	VkImageView		color_image_view;
	
	VkImage			bloom_image[1 + VK_NUM_BLOOM_PASSES * 2];
	VkImageView		bloom_image_view[1 + VK_NUM_BLOOM_PASSES * 2];

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

	vk_tess_t tess[NUM_COMMAND_BUFFERS], *cmd;
	int cmd_index;

	// render passes
	struct {
		VkRenderPass main;
		VkRenderPass gamma;
		VkRenderPass screenmap;
		VkRenderPass blur[VK_NUM_BLOOM_PASSES * 2]; // horizontal-vertical pairs
		VkRenderPass bloom_extract;
		VkRenderPass post_bloom;
		VkRenderPass capture;
	} render_pass;

	struct {
		VkImage image;
		VkImageView image_view;
	} capture;

	// framebuffers
	struct {
		VkFramebuffer main[MAX_SWAPCHAIN_IMAGES];
		VkFramebuffer gamma[MAX_SWAPCHAIN_IMAGES];
		VkFramebuffer screenmap;
		VkFramebuffer blur[VK_NUM_BLOOM_PASSES * 2];
		VkFramebuffer bloom_extract;
		VkFramebuffer capture;
	} framebuffers;

	struct {
		VkBuffer		vertex_buffer;
		VkDeviceMemory	buffer_memory;
	} vbo;

	// host visible memory that holds vertex, index and uniform data
	VkDeviceMemory		geometry_buffer_memory;
	VkDeviceSize		geometry_buffer_size;
	VkDeviceSize		geometry_buffer_size_new;

	VkDescriptorPool		descriptor_pool;
	VkDescriptorSetLayout	set_layout_sampler;
	VkDescriptorSetLayout	set_layout_uniform;
	VkDescriptorSetLayout	set_layout;
	VkDescriptorSetLayout	set_layout_storage;

	// pipeline(s)
	VkPipelineLayout pipeline_layout;
	VkPipelineLayout pipeline_layout_storage;
	VkPipelineLayout pipeline_layout_post_process;	// post-processing
	VkPipelineLayout pipeline_layout_blend;			// post-processing

	VkPipeline gamma_pipeline;
	VkPipeline bloom_extract_pipeline;
	VkPipeline blur_pipeline[VK_NUM_BLOOM_PASSES * 2]; // horizontal & vertical pairs
	VkPipeline bloom_blend_pipeline;
	VkPipeline capture_pipeline;

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
		uint32_t fog_pipelines[2][3][2];

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
	uint32_t uniform_item_size;
	uint32_t uniform_alignment;

	// shader modules.
	struct {
		struct {
			VkShaderModule gen[3][2][2][2]; // tx[0,1,2], cl[0,1] env0[0,1] fog[0,1]
			VkShaderModule light[2]; // fog[0,1]
			VkShaderModule gen0_ident;
		}	vert;

		struct {
			VkShaderModule gen0_ident;
			VkShaderModule gen0_df;
			VkShaderModule gen[3][2][2]; // tx[0,1,2] cl[0,1] fog[0,1]
			VkShaderModule light[2][2]; // linear[0,1] fog[0,1]
		}	frag;

		VkShaderModule dot_fs;
		VkShaderModule dot_vs;

		VkShaderModule gamma_fs;
		VkShaderModule gamma_vs;

		VkShaderModule fog_vs;
		VkShaderModule fog_fs;

		VkShaderModule color_vs;
		VkShaderModule color_fs;

		VkShaderModule bloom_fs;
		VkShaderModule blur_fs;
		VkShaderModule blend_fs;
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

	float maxAnisotropy;
	float maxLod;

	VkFormat depth_format;
	VkFormat color_format;
	VkFormat bloom_format;
	VkFormat capture_format;

	VkImageLayout initSwapchainLayout;

	VkBool32 isInitialized;
	VkBool32 msaaActive;

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
VkBool32	is_vk_initialized( void );
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
void		vk_create_bloom_pipelines( void );
void		vk_alloc_persistent_pipelines( void );
void		vk_create_blur_pipeline( uint32_t index, uint32_t width, uint32_t height, qboolean horizontal_pass );
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
void		vk_create_framebuffers( void );
void		vk_destroy_framebuffers( void );
void		vk_create_sync_primitives( void );
void		vk_destroy_sync_primitives( void );
void		vk_release_geometry_buffers( void );
void		vk_release_resources( void );
void		vk_read_pixels( byte *buffer, uint32_t width, uint32_t height );

// vbo
void		vk_release_vbo( void );
qboolean	vk_alloc_vbo( const byte *vbo_data, int vbo_size );
void		VBO_PrepareQueues( void );
void		VBO_RenderIBOItems( void );
void		VBO_ClearQueue( void );

// shader
void		vk_create_shader_modules( void );
void		vk_destroy_shader_modules( void );

// command
VkCommandBuffer vk_begin_command_buffer( void );
void		vk_end_command_buffer( VkCommandBuffer command_buffer );
void		vk_create_command_pool( VkCommandPool *pPool );
void		vk_create_command_buffer( VkCommandPool pool, VkCommandBuffer *pBuf );
void		vk_record_image_layout_transition( VkCommandBuffer command_buffer, VkImage image, 
	VkImageAspectFlags image_aspect_flags, VkAccessFlags src_access_flags, 
	VkImageLayout old_layout, VkAccessFlags dst_access_flags, VkImageLayout new_layout,
	uint32_t src_family_index, uint32_t dst_family_index, 
	VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask );

// memory
uint32_t	vk_find_memory_type( uint32_t memory_type_bits, VkMemoryPropertyFlags properties );
uint32_t	vk_find_memory_type_lazy( uint32_t memory_type_bits, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags *outprops );

// attachment
void		vk_create_attachments( void );
void		vk_destroy_attachments( void );
void		vk_update_attachment_descriptors( void );
void		vk_clear_color_attachments( const float *color );
void		vk_clear_depthstencil_attachments( qboolean clear_stencil );

// shade geometry
void		vk_set_2d( void );
void		vk_set_depthrange( const Vk_Depth_Range depthRange );
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
void		vk_draw_geometry( Vk_Depth_Range depRg, VkBool32 indexed );
void		vk_bind_geometry( uint32_t flags );
void		vk_bind_lighting( int stage, int bundle );
void		vk_reset_descriptor( int index);
void		vk_update_uniform_descriptor( VkDescriptorSet descriptor, VkBuffer buffer );
void		vk_create_storage_buffer( uint32_t size );
void		vk_update_descriptor_offset( int index, uint32_t offset );
void		vk_init_descriptors( void );
void		vk_create_vertex_buffer( VkDeviceSize size );
VkBuffer	vk_get_vertex_buffer( void );
void		vk_update_descriptor( int tmu, VkDescriptorSet curDesSet );
uint32_t	vk_find_pipeline_ext( uint32_t base, const Vk_Pipeline_Def *def, qboolean use );
VkPipeline	vk_gen_pipeline( uint32_t index );
void		vk_end_render_pass( void );
void		vk_begin_main_render_pass( void );
void		vk_get_pipeline_def( uint32_t pipeline, Vk_Pipeline_Def *def );

// image process
void		GetScaledDimension( const unsigned int width, const unsigned int height, 
	unsigned int * const outW, unsigned int * const outH, int isPicMip );
void		R_SetColorMappings( void );
void		R_LightScaleTexture( byte *in, int inwidth, int inheight, qboolean only_gamma );
void		ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight );
void		R_BlendOverTexture( unsigned char *data, const uint32_t pixelCount, const uint32_t l );
void		R_MipMapNormal( byte *out, byte *in, int width, int height, const qboolean swizzle );
void		R_MipMap( byte *out, byte *in, int width, int height );
void		R_MipMap2( unsigned* const out, unsigned* const in, int inWidth, int inHeight );

// image
VkSampler	vk_find_sampler( const Vk_Sampler_Def *def );
void		vk_delete_textures( void );
void		vk_upload_image_data( VkImage image, int x, int y, int width, int height, 
	qboolean mipmap, const uint8_t *pixels, int bytes_per_pixel );
void		vk_record_buffer_memory_barrier( VkCommandBuffer cb, VkBuffer buffer, 
	VkDeviceSize size, VkPipelineStageFlags src_stages, VkPipelineStageFlags dst_stages, 
	VkAccessFlags src_access, VkAccessFlags dst_access );

// bloom
void		vk_begin_blur_render_pass( uint32_t index );
void		vk_begin_post_bloom_render_pass( void );
void		vk_begin_bloom_extract_render_pass( void );
qboolean	vk_bloom( void );

// info
const char	*vk_format_string( VkFormat format );
const char	*vk_result_string( VkResult code );
const char	*vk_shadertype_string( Vk_Shader_Type code );
void		vk_get_vulkan_properties( VkPhysicalDeviceProperties *props );

// debug
void		vk_debug( const char *msg, ... );
void		vk_create_debug_callback( void );
void		R_DebugGraphics( void );

static void vk_set_object_name( uint64_t obj, const char *objName, VkDebugReportObjectTypeEXT objType )
{
	if (qvkDebugMarkerSetObjectNameEXT && obj)
	{
		VkDebugMarkerObjectNameInfoEXT info;
		info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		info.pNext = NULL;
		info.objectType = objType;
		info.object = obj;
		info.pObjectName = objName;
		qvkDebugMarkerSetObjectNameEXT(vk.device, &info);
	}
}
#define VK_SET_OBJECT_NAME( obj, objName, objType) vk_set_object_name( (uint64_t)( obj ), ( objName ), ( objType ) );

#define VK_CHECK( function_call ) { \
	VkResult result = function_call; \
	if ( result != VK_SUCCESS ) \
		vk_debug( "Vulkan: error %s returned by %s \n", vk_result_string( result ), #function_call ); \
}