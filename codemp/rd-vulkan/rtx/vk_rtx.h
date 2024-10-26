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

#ifndef VK_RTX_H
#define VK_RTX_H

#include "dds.h"
#include "shaders/glsl/rtx/sky.h"
#include "shaders/glsl/rtx/global_ubo.h"		// contains constants.h
#include "shaders/glsl/rtx/global_textures.h"	// contains constants.h, ignored
#include "shaders/glsl/rtx/inspector.h"
#include "vk_rtx_asvgf.h"
#include "vk_rtx_tonemap.h"
#include "vk_rtx_precomputed_sky.h"
#include "vk_rtx_physical_sky.h"

// math
#define frand()     ((rand() & 32767) * (1.0 / 32767))

#define VectorVectorScale(in,scale,out) \
        ((out)[0]=(in)[0]*(scale)[0], \
         (out)[1]=(in)[1]*(scale)[1], \
         (out)[2]=(in)[2]*(scale)[2])

// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define DVIS_PVS    0
#define DVIS_PHS    1
#define DVIS_PVS2   16 // Q2RTX : 2nd order PVS

#define MAX_RIMAGES 2048

// maximum size of a PVS row, in bytes
#define VIS_MAX_BYTES   ( MAX_MAP_LEAFS >> 3 )

#define NUM_TAA_SAMPLES						128

#define VK_MAX_SWAPCHAIN_SIZE				2

#define VK_AS_MEMORY_ALLIGNMENT_SIZE		65536

#define VK_MAX_BOTTOM_AS					768
#define VK_MAX_STATIC_BOTTOM_AS_INSTANCES	16
#define VK_MAX_DYNAMIC_BOTTOM_AS_INSTANCES	384
#define VK_MAX_BOTTOM_AS_INSTANCES (VK_MAX_STATIC_BOTTOM_AS_INSTANCES + VK_MAX_DYNAMIC_BOTTOM_AS_INSTANCES)

#define RTX_BOTTOM_AS_FLAG (VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR)
#define VK_GLOBAL_IMAGEARRAY_SHADER_STAGE_FLAGS (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT)
#define AS_BUFFER_FLAGS (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)

#define RTX_WORLD_STATIC_XYZ_SIZE			16384 * 64 
#define RTX_WORLD_STATIC_IDX_SIZE			16384 * 128
#define RTX_WORLD_DYNAMIC_DATA_XYZ_SIZE		16384 * 4
#define RTX_WORLD_DYNAMIC_DATA_IDX_SIZE		16384 * 8
#define RTX_WORLD_DYNAMIC_AS_XYZ_SIZE		16384 * 8
#define RTX_WORLD_DYNAMIC_AS_IDX_SIZE		16384 * 16
#define RTX_ENTITY_STATIC_XYZ_SIZE			16384 * 128
#define RTX_ENTITY_STATIC_INDEX_SIZE		16384 * 256
#define RTX_ENTITY_DYNAMIC_XYZ_SIZE			16384 * 16
#define RTX_ENTITY_DYNAMIC_IDX_SIZE			16384 * 32

#define ANIMATE_TEXTURE		( tess.shader->stages[0]->bundle[0].numImageAnimations > 0 )
#define UV_CHANGES			( tess.shader->stages[0] != NULL ? ((tess.shader->stages[0]->bundle[0].tcGen != TCGEN_BAD)  && tess.shader->stages[0]->bundle[0].numTexMods > 0 /*&& tess.shader->stages[0]->bundle[0].texMods[0].type != TMOD_NONE*/) : qfalse )
#define UV_CHANGES_S(s)		( tess.shader->stages[s] != NULL ? ((tess.shader->stages[s]->bundle[0].tcGen != TCGEN_BAD)  && tess.shader->stages[s]->bundle[0].numTexMods > 0 /*&& tess.shader->stages[0]->bundle[0].texMods[0].type != TMOD_NONE*/) : qfalse )
#define MODEL_DEFORM		( tess.shader->numDeforms > 0 )
#define ANIMATE_MODEL		( backEnd.currentEntity->e.frame > 0 || backEnd.currentEntity->e.oldframe > 0 )
#define RTX_DYNAMIC_AS		( MODEL_DEFORM || ANIMATE_MODEL )
#define RTX_DYNAMIC_AS_DATA	( RTX_DYNAMIC_AS || UV_CHANGES || ANIMATE_TEXTURE )
#define RTX_DYNAMIC_DATA	( UV_CHANGES || ANIMATE_TEXTURE )

#define MEM_BARRIER_BUILD_ACCEL( cmd_buf, ... ) \
	do { \
		VkMemoryBarrier barrier;  \
		Com_Memset( &barrier, 0, sizeof(VkMemoryBarrier) ); \
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;  \
		barrier.pNext = NULL;  \
		barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR \
							  | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR; \
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR; \
	 \
		qvkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, \
				VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, \
				&barrier, 0, 0, 0, 0); \
	} while(0)

#define BUFFER_BARRIER( cmd_buf, _src_access_mask, _dst_access_mask, _buffer, _offset, _size ) \
	do { \
		VkBufferMemoryBarrier barrier;\
		Com_Memset( &barrier, 0, sizeof(VkBufferMemoryBarrier) ); \
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;  \
		barrier.pNext = NULL; \
		barrier.srcAccessMask = _src_access_mask; \
		barrier.dstAccessMask = _dst_access_mask; \
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; \
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; \
		barrier.buffer = _buffer; \
		barrier.offset = _offset; \
		barrier.size = _size; \
	\
		qvkCmdPipelineBarrier( cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, \
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 1, &barrier, \
				0, NULL); \
	} while(0)

#define IMAGE_BARRIER( cmd_buf, _image, _range, _src_access_mask, _dst_access_mask, _old_layout, _new_layout ) \
	do { \
		VkImageMemoryBarrier barrier; \
		Com_Memset( &barrier, 0, sizeof(VkImageMemoryBarrier) ); \
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, \
		barrier.pNext = NULL, \
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; \
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; \
		barrier.image = _image; \
		barrier.subresourceRange = _range; \
		barrier.srcAccessMask = _src_access_mask; \
		barrier.dstAccessMask = _dst_access_mask; \
		barrier.oldLayout = _old_layout; \
		barrier.newLayout = _new_layout; \
		\
		qvkCmdPipelineBarrier( cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, \
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, \
				1, &barrier ); \
	} while(0)

// passes information back from the RTX renderer to the engine for various development maros
typedef struct ref_feedback_s {
	int         viewcluster;
	int         lookatcluster;
	int         num_light_polys;
	int         resolution_scale;

	char        view_material[MAX_QPATH];
	char        view_material_override[MAX_QPATH];

	float		sun_luminance;
	float		adapted_luminance;

	vec3_t      hdr_color;
} ref_feedback_t;

typedef struct sun_light_s {
	vec3_t		direction;
	vec3_t		direction_envmap;
	vec3_t		color;
	float		angular_size_rad;
	qboolean	use_physical_sky;
	qboolean	visible;
} sun_light_t;

typedef struct EntityUploadInfo
{
	uint32_t num_instances;
	uint32_t num_vertices;
	uint32_t dynamic_vertex_num;
	uint32_t transparent_model_vertex_offset;
	uint32_t transparent_model_vertex_num;
	uint32_t viewer_model_vertex_offset;
	uint32_t viewer_model_vertex_num;
	uint32_t viewer_weapon_vertex_offset;
	uint32_t viewer_weapon_vertex_num;
	uint32_t explosions_vertex_offset;
	uint32_t explosions_vertex_num;
	qboolean weapon_left_handed;
} EntityUploadInfo;

typedef struct {
	vec3_t		mins, maxs;
	int			idx;
} cluster_t;

typedef struct {
    VkDeviceSize	allocSize;
    VkBool32		onGpu;
    byte			*p;
    VkBuffer		buffer;
    VkDeviceMemory	memory;
    VkDeviceAddress address;
	int				is_mapped;
} vkbuffer_t;

typedef struct {
	vkbuffer_t buffer;
	vkbuffer_t staging_buffer;
	int registration_sequence;
} model_vbo_t;

typedef struct {
	float			transform[12];
	uint32_t		instance_id			: 24;
	uint32_t		mask				: 8;
	uint32_t		instance_offset		: 24;
	uint32_t		flags				: 8;
	uint64_t		acceleration_structure;
} vk_geometry_instance_t; // needed for top AS

typedef struct {
	union {
		VkDescriptorImageInfo							*image;
		VkDescriptorBufferInfo							*buffer;
		VkWriteDescriptorSetAccelerationStructureKHR	as;
	};

	uint32_t updateSize;
} vkdescriptorData_t;

typedef struct {
    size_t							size;
    VkDescriptorSetLayoutBinding	*bindings;	// length = size
    vkdescriptorData_t				*data;		// length = size
    VkDescriptorSet					set; 
    VkDescriptorSetLayout			layout;
    VkDescriptorPool				pool;

	// ext
	qboolean						lastBindingVariableSizeExt;
	qboolean						needsUpdate;
} vkdescriptor_t;

typedef struct {
    size_t									size;
    VkShaderModule							*modules;
    VkShaderStageFlagBits					*flags;
    VkPipelineShaderStageCreateInfo			*shader_stages;
	size_t									group_count;
	VkRayTracingShaderGroupCreateInfoKHR	*shader_groups;
} vkshader_t;

typedef struct {
	VkPipelineCache			cache;
	VkPipelineLayout		layout;
	VkPipeline				handle;

	vkshader_t				*shader;

	VkDescriptorSetLayout	*set_layouts;
	uint32_t				set_layouts_count;

	struct {
		size_t				size;
		VkPushConstantRange	*p;
	} pushConstantRange;
} vkpipeline_t;

typedef struct {
	VkExtent3D		extent;
	uint32_t		mipLevels;
	uint32_t		arrayLayers;

	VkImage			handle;
	VkImageView		view;
	VkDeviceMemory	memory;
} vkimage_t;

typedef struct {
	uint32_t	idx_world_static_offset;
	uint32_t	xyz_world_static_offset;
	uint32_t	cluster_world_static_offset;
	vkbuffer_t	idx_world_static;
	vkbuffer_t	xyz_world_static;
	vkbuffer_t	cluster_world_static;

	uint32_t	idx_world_dynamic_data_offset;
	uint32_t	xyz_world_dynamic_data_offset;
	uint32_t	cluster_world_dynamic_data_offset;
	vkbuffer_t	idx_world_dynamic_data[VK_MAX_SWAPCHAIN_SIZE];
	vkbuffer_t	xyz_world_dynamic_data[VK_MAX_SWAPCHAIN_SIZE];
	vkbuffer_t	cluster_world_dynamic_data;

	uint32_t	idx_world_dynamic_as_offset[VK_MAX_SWAPCHAIN_SIZE];
	uint32_t	xyz_world_dynamic_as_offset[VK_MAX_SWAPCHAIN_SIZE];
	uint32_t	cluster_world_dynamic_as_offset;
	vkbuffer_t	idx_world_dynamic_as[VK_MAX_SWAPCHAIN_SIZE];
	vkbuffer_t	xyz_world_dynamic_as[VK_MAX_SWAPCHAIN_SIZE];
	vkbuffer_t	cluster_world_dynamic_as;
} vkgeometry_t;

typedef struct {
	VkAccelerationStructureKHR	accel_khr;
	uint64_t					handle;
	VkDeviceSize				offset;
} vk_tlas_t;

typedef struct accel_bottom_match_info_s {
	int fast_build;
	uint32_t vertex_count;
	uint32_t index_count;
} vk_blas_match_info_t;

typedef struct {
	VkAccelerationStructureKHR			accel_khr;
	vk_blas_match_info_t				match;
	vkbuffer_t							mem;
	uint64_t							handle;
	VkAccelerationStructureGeometryKHR	geometries;

	ASInstanceData						data;
	VkDeviceSize						offset;			// offset in static or dynamic buffer
	qboolean							isWorldSurface;	// just for entity bas which are from world surfaces

	// revisit this
	uint32_t	create_num_vertices;
	uint32_t	create_num_indices;
} vk_blas_t;

typedef struct {
	uint32_t	index;
	uint32_t	remappedIndex;
	qboolean	active;
	qboolean	uploaded[VK_MAX_SWAPCHAIN_SIZE];
	uint32_t	albedo;
	uint32_t	emissive;	// glow
	uint32_t	normals;
	uint32_t	phyiscal;
	vec4_t		specular_scale;
} rtx_material_t;

typedef struct {
	int gpu_index;
	int bounce;
} pt_push_constants_t;
#endif // VK_RTX_H

#ifdef VK_RTX_LINKER
void		vk_rtx_cvar_handler( void );
void		vk_rtx_begin_registration( void );
void		vk_rtx_initialize( void );
void		vk_rtx_shutdown( void );
void		vk_rtx_begin_frame( void );
void		vk_rtx_show_pvs_f( void );

VkDescriptorSet vk_rtx_get_current_desc_set_textures( void);
void			vk_rtx_get_descriptor_index( uint32_t &idx, uint32_t &prev_idx);

void		R_PreparePT( world_t &worldData ) ;

// matrix
void		mult_matrix_matrix( float *p, const float *a, const float *b );
void		create_entity_matrix( float matrix[16], trRefEntity_t *e, qboolean enable_left_hand );
void		create_projection_matrix( float matrix[16], float znear, float zfar, float fov_x, float fov_y );
void		create_orthographic_matrix( float matrix[16], float xmin, float xmax,
										float ymin, float ymax, float znear, float zfar );
void		create_view_matrix( float matrix[16], trRefdef_t *fd );
void		inverse( const float *m, float *inv );
void		mult_matrix_vector( float *p, const float *a, const float *b );

// shade 
void		temporal_cvar_changed( void );
void		VK_BeginRenderClear( void );

void		vk_rtx_update_descriptor( vkdescriptor_t *descriptor );
void		vk_rtx_add_descriptor_sampler( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, uint32_t count, VkImageLayout layout );
void		vk_rtx_add_descriptor_image( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage );
void		vk_rtx_add_descriptor_as( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage );
void		vk_rtx_bind_descriptor_as( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkAccelerationStructureKHR *as );
uint32_t	vk_rtx_add_descriptor_buffer( vkdescriptor_t *descriptor, uint32_t count, uint32_t binding, VkShaderStageFlagBits stage, VkDescriptorType type );
void		vk_rtx_bind_descriptor_buffer( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkBuffer buffer );
void		vk_rtx_bind_descriptor_image_sampler( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkSampler sampler, VkImageView view, uint32_t index );
void		vk_rtx_create_descriptor( vkdescriptor_t *descriptor );
void		vk_rtx_set_descriptor_update_size( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, uint32_t size );
void		vk_rtx_destroy_descriptor( vkdescriptor_t *descriptor );

// pipeline
void		vk_rtx_bind_pipeline_shader( vkpipeline_t *pipeline, vkshader_t *shader );
void		vk_rtx_bind_pipeline_desc_set_layouts( vkpipeline_t *pipeline, VkDescriptorSetLayout *set_layouts, uint32_t count );
void		vk_rtx_create_compute_pipeline( vkpipeline_t *pipeline, VkSpecializationInfo *spec, uint32_t push_size );
void		vk_rtx_create_raytracing_pipelines( void );
void		vk_rtx_create_compute_pipelines( void );
void		vk_rtx_destroy_pipeline( vkpipeline_t *pipeline );

// uniform
VkResult	vkpt_uniform_buffer_create( void );
VkResult	vkpt_uniform_buffer_destroy( void );
VkResult	vkpt_uniform_buffer_update( VkCommandBuffer command_buffer, uint32_t idx );

// buffer
VkResult	allocate_gpu_memory( VkMemoryRequirements mem_req, VkDeviceMemory *pMemory );
VkResult	vk_rtx_buffer_create( vkbuffer_t *buf, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_properties );
VkResult	vk_rtx_buffer_destroy( vkbuffer_t *buf );
void		*buffer_map( vkbuffer_t *buf );
void		buffer_unmap( vkbuffer_t *buf );

void		VK_CreateImageMemory( VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *bufferMemory );
void		VK_CreateAttributeBuffer( vkbuffer_t *buffer, VkDeviceSize allocSize, VkBufferUsageFlagBits usage );
void		vk_rtx_upload_buffer_data_offset( vkbuffer_t *buffer, VkDeviceSize offset, VkDeviceSize size, const byte *data );
void		vk_rtx_upload_buffer_data( vkbuffer_t *buffer, const byte *data );
VkResult	vkpt_light_buffer_upload_to_staging( const uint32_t idx, qboolean render_world, 
												 world_t *world, int num_model_lights, const float* sky_radiance );
VkResult	vk_rtx_readback( ReadbackBuffer *dst );
VkResult	vkpt_light_buffers_create( world_t &world );
VkResult	vkpt_light_buffers_destroy( void );
void		VK_DestroyBuffer( vkbuffer_t *buffer );
void		vk_rtx_create_buffers( void );
void		vk_rtx_destroy_buffers( void );
void		vk_rtx_upload_indices( vkbuffer_t *buffer, uint32_t offsetIDX, uint32_t offsetXYZ );
void		vk_rtx_upload_vertices( vkbuffer_t *buffer, uint32_t offsetXYZ, int cluster );

// acceleration structure
qboolean	RB_ASDataDynamic( shader_t *shader );
qboolean	RB_ASDynamic( shader_t *shader );
void		vk_rtx_reset_accel_offsets( void );
//void		vk_rtx_append_blas( vk_blas_t *blas, int instance_id, uint32_t type, uint32_t offset, uint32_t flags );
void		vk_rtx_create_blas( VkCommandBuffer cmd_buf,  
								 vkbuffer_t *vertex_buffer, VkDeviceAddress vertex_offset,
								 vkbuffer_t *index_buffer, VkDeviceAddress index_offset,
								 uint32_t num_vertices, uint32_t num_indices,
								 vk_blas_t *blas, boolean is_dynamic, qboolean fast_build, 
								 qboolean allow_update, qboolean instanced );
void		vk_rtx_update_blas( VkCommandBuffer cmd_buf, 
								vk_blas_t* oldBas, vk_blas_t* newBas, 
								VkDeviceSize* offset, VkBuildAccelerationStructureFlagsKHR flag);
void		vk_rtx_create_tlas	( VkCommandBuffer cmd_buf, int idx, uint32_t num_instances,
								  VkBuildAccelerationStructureFlagsKHR flag );
void		vk_rtx_destroy_accel_all( void );
void		vk_rtx_destroy_tlas( int idx );
void		vk_rtx_destroy_blas( vk_blas_t *blas );


// bsp
mnode_t		*BSP_PointLeaf( mnode_t *node, vec3_t p );
int			R_FindClusterForPos( world_t &worldData, const vec3_t p );
int			R_FindClusterForPos2( world_t &worldData, const vec3_t p );
int			R_FindClusterForPos3( world_t &worldData, const vec3_t p );
qboolean	RB_ClusterVisIdent(byte* aVis, byte* bVis);
int			RB_CheckClusterExist(byte* cVis);
int			RB_TryMergeCluster(int cluster[3], int defaultC);
int			RB_GetCluster( void );
void		get_triangle_norm( const float* positions, float* normal );
qboolean	get_triangle_off_center(const float* positions, float* center, float* anti_center);
void		build_pvs2( world_t *world );
int			R_GetClusterFromSurface( world_t &worldData, surfaceType_t *surf );

// material
void			vk_rtx_clear_material_list( void ) ;
void			vk_rtx_clear_material( uint32_t index );
rtx_material_t	*vk_rtx_get_material( uint32_t index );

uint32_t	vk_rtx_find_emissive_texture( const shader_t *shader );
qboolean	RB_StageNeedsColor( int stage );
uint32_t	RB_GetMaterial( shader_t *shader );
uint32_t	RB_GetNextTex( shader_t *shader, int stage );
uint32_t	RB_GetNextTexEncoded( shader_t *shader, int stage );
qboolean	RB_IsLight( shader_t *shader );
qboolean	RB_SkipObject( shader_t *shader );
qboolean	RB_IsTransparent( shader_t *shader );
void		vk_rtx_update_shader_material( shader_t *shader, shader_t *updatedShader );
VkResult	vk_rtx_shader_to_material( shader_t *shader, uint32_t &index, uint32_t &flags );
VkResult	vk_rtx_upload_materials( const uint32_t idx, LightBuffer *lbo );

// image
void		vk_rtx_create_image( const char *name, vkimage_t *image, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels );
void		vk_rtx_create_cubemap( vkimage_t *image, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels );
void		vk_rtx_upload_image_data( vkimage_t *image, uint32_t width, uint32_t height, const uint8_t *pixels, uint32_t bytes_per_pixel, uint32_t mipLevel, uint32_t arrayLayer );
void		vk_rtx_create_images( void );
void		vk_rtx_initialize_images( void );
void		vk_rtx_destroy_image( vkimage_t *image );
void		vk_rtx_create_blue_noise( void );

// shader
void		vk_load_final_blit_shader( void );

vkshader_t	*vk_rtx_create_global_shader( void );
void		vk_rtx_destroy_shaders( void );

void		vk_rtx_begin_scene( trRefdef_t *refdef, drawSurf_t *drawSurfs, int numDrawSurfs );
void		vk_rtx_begin_blit( void );

// phyiscal sky
void		vk_rtx_init_sky_scatter( void );
void		vk_rtx_prepare_envmap( world_t &worldData );
void		vk_rtx_set_envmap_descriptor_binding( void );
void		prepare_sky_matrix( float time, vec3_t sky_matrix[3] );
void		vk_rtx_evaluate_sun_light( sun_light_t *light, const vec3_t sky_matrix[3], float time );
VkResult	vk_rtx_physical_sky_update_ubo( vkUniformRTX_t *ubo, const sun_light_t *light, qboolean render_world );

// shadow map
VkResult	vk_rtx_shadow_map_initialize( void );
VkResult	vk_rtx_shadow_map_destroy( void );
VkResult	vk_rtx_shadow_map_create_pipelines( void );
VkResult	vk_rtx_shadow_map_destroy_pipelines( void );
VkResult	vk_rtx_shadow_map_render( VkCommandBuffer cmd_buf, float *view_projection_matrix, int num_static_indices, int num_dynamic_verts, int transparent_offset, int num_transparent_verts );
VkImageView	vk_rtx_shadow_map_get_view( void );
void		vk_rtx_shadow_map_setup( const sun_light_t *light, const float *bbox_min, const float *bbox_max, float *VP, float *depth_scale, qboolean random_sampling );

// god rays
qboolean	vk_rtx_god_rays_enabled( const sun_light_t* sun_light );
VkResult	vk_rtx_god_rays_initialize( void );
VkResult	vk_rtx_god_rays_destroy( void );
void		vk_rtx_god_rays_prepare_ubo( vkUniformRTX_t * ubo, /*const aabb_t* world_aabb,*/ const float* proj, 
									   const float* view, const float* shadowmap_viewproj, float shadowmap_depth_scale );
VkResult	vk_rtx_god_rays_create_pipelines( void );
VkResult	vk_rtx_god_rays_destroy_pipelines( void );
VkResult	vk_rtx_god_rays_update_images( void );
VkResult	vk_rtx_god_rays_noop( void );
void		vk_rtx_record_god_rays_trace_command_buffer( VkCommandBuffer command_buffer, int pass );
void		vk_rtx_record_god_rays_filter_command_buffer( VkCommandBuffer command_buffer );
void		vk_rtx_get_god_rays_shadowmap( VkImageView &view, VkSampler &sampler );

// models
void		vk_rtx_write_model_descriptor( int index, VkDescriptorSet descriptor, VkBuffer buffer, VkDeviceSize size );
void		vk_rtx_create_model_vbo_ibo_descriptor( void );
VkResult	vk_rtx_model_vbo_create_pipelines( void );
void		vkpt_instance_geometry( VkCommandBuffer cmd_buf, uint32_t num_instances, qboolean update_world_animations );
#ifdef USE_RTX_GLOBAL_MODEL_VBO
void		vk_rtx_bind_model( int index );
#else
void		vk_rtx_build_mdxm_vbo( model_t *mod, mdxmHeader_t *mdxm );
#endif

// ghoul2
void		vk_rtx_AddGhoulSurfaces( trRefEntity_t *ent, int entityNum );
void		vk_rtx_found_mdxm_vbo_mesh( mdxmVBOMesh_t *mesh, shader_t *shader );

// debug
#define PROFILER_LIST \
	PROFILER_DO(PROFILER_FRAME_TIME,                 0) \
	PROFILER_DO(PROFILER_INSTANCE_GEOMETRY,          1) \
	PROFILER_DO(PROFILER_BVH_UPDATE,                 1) \
	PROFILER_DO(PROFILER_PRIMARY_RAYS,               1) \
	PROFILER_DO(PROFILER_REFLECT_REFRACT_1,          1) \
	PROFILER_DO(PROFILER_REFLECT_REFRACT_2,          1) \
	PROFILER_DO(PROFILER_ASVGF_GRADIENT_REPROJECT,   1) \
	PROFILER_DO(PROFILER_DIRECT_LIGHTING,            1) \
	PROFILER_DO(PROFILER_INDIRECT_LIGHTING,          1) \
	PROFILER_DO(PROFILER_ASVGF_FULL,                 1) \
	PROFILER_DO(PROFILER_ASVGF_RECONSTRUCT_GRADIENT, 2) \
	PROFILER_DO(PROFILER_ASVGF_TEMPORAL,             2) \
	PROFILER_DO(PROFILER_ASVGF_ATROUS,               2) \
	PROFILER_DO(PROFILER_MGPU_TRANSFERS,             1) \
	PROFILER_DO(PROFILER_INTERLEAVE,                 1) \
	PROFILER_DO(PROFILER_ASVGF_TAA,                  2) \
	PROFILER_DO(PROFILER_BLOOM,                      1) \
	PROFILER_DO(PROFILER_TONE_MAPPING,               1) \
	PROFILER_DO(PROFILER_UPDATE_ENVIRONMENT,         1) \
	PROFILER_DO(PROFILER_GOD_RAYS,                   1) \
	PROFILER_DO(PROFILER_GOD_RAYS_REFLECT_REFRACT,   1) \
	PROFILER_DO(PROFILER_GOD_RAYS_FILTER,            1) \
	PROFILER_DO(PROFILER_SHADOW_MAP,                 1) \
	PROFILER_DO(PROFILER_COMPOSITING,                1) \

enum {
#define PROFILER_DO(a, ...) a,
	PROFILER_LIST
#undef PROFILER_DO
	NUM_PROFILER_ENTRIES,
};

static inline void begin_perf_marker( VkCommandBuffer cmd_buf, const char* name )
{
	VkDebugUtilsLabelEXT label;
	Com_Memset( &label, 0, sizeof(VkDebugUtilsLabelEXT) );
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pNext = NULL;
	label.pLabelName = name;

	if ( qvkCmdBeginDebugUtilsLabelEXT != NULL )
		qvkCmdBeginDebugUtilsLabelEXT( cmd_buf, &label );
}

static inline void end_perf_marker( VkCommandBuffer cmd_buf, int index )
{
	if (qvkCmdEndDebugUtilsLabelEXT != NULL)
		qvkCmdEndDebugUtilsLabelEXT(cmd_buf);
}

#define BEGIN_PERF_MARKER(cmd_buf, name)  begin_perf_marker(cmd_buf, #name)
#define END_PERF_MARKER(cmd_buf, index)    end_perf_marker(cmd_buf, index)
#endif