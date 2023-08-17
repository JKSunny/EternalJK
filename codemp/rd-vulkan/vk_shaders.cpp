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

#include "tr_local.h"
#include "shaders/spirv/shader_data.c"

// Vulkan has to be specified in a bytecode format which is called SPIR-V
// and is designed to be work with both Vulkan and OpenCL.
//
// The graphics pipeline is the sequence of the operations that take the
// vertices and textures of your meshes all way to the pixels in the
// render targets.
static VkShaderModule SHADER_MODULE( const uint8_t *bytes, const int count ) {
    VkShaderModuleCreateInfo desc;
    VkShaderModule module;

    if (count % 4 != 0) {
        ri.Error(ERR_FATAL, "Vulkan: SPIR-V binary buffer size is not a multiple of 4");
    }

    desc.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.codeSize = count;
    desc.pCode = (const uint32_t*)bytes;

    VK_CHECK(qvkCreateShaderModule(vk.device, &desc, NULL, &module));

    return module;
}
#define SHADER_MODULE( name ) SHADER_MODULE( name, sizeof( name ) )

#include "shaders/spirv/shader_binding.c"

void vk_create_shader_modules( void )
{
    vk_bind_generated_shaders();

#if 0
    int i, j, k, l, m, n, o;

    const char *vbo[] = { "cpu ", "gpu ghoul2", "gpu mdv" };
    const char *pbr[] = { "", "pbr " };
    const char *light[] = { "", "lightmap", "vector", "vertex" };
    const char *tx[]  = { "single", "double", "triple" };
    const char *cl[]  = { "", "+cl" };
    const char *env[] = { "", "+env" };
    const char *fog[] = { "", "+fog" };

    for ( i = 0; i < 3; i++ ) {
        for ( j = 0; j < 2; j++ ) {
            for ( k = 0; k < 4; k++ ) {
                for ( l = 0; l < 3; l++ ) {
                    for ( m = 0; m < 2; m++ ) {
                        for ( n = 0; n < 2; n++ ) {
                            for ( o = 0; o < 2; o++ ) 
                            {
                                const char *s = va( "%s texture %s%s%s%s%s%s vertex module", vbo[i], pbr[j], light[k], tx[l], cl[m], env[n], fog[o] );
                                VK_SET_OBJECT_NAME( vk.shaders.vert.gen[i][j][k][l][m][n][o], s, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
                            }
                        }
                    }
                }
            }
        }
    }
    for ( j = 0; j < 2; j++ ) {
        for ( k = 0; k < 4; k++ ) {
            for ( l = 0; l < 3; l++ ) {
                for ( m = 0; m < 2; m++ ) {
                    for ( n = 0; n < 2; n++ ) {
                        const char *s = va( "texture %s%s%s%s%s fragment module", pbr[j], light[k], tx[l], cl[m], fog[n] );
                        VK_SET_OBJECT_NAME( vk.shaders.frag.gen[j][k][l][m][n], s, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
                    }
                }
            }
        }
    }
#endif

    vk.shaders.frag.gen0_df = SHADER_MODULE(frag_tx0_df);
    VK_SET_OBJECT_NAME(vk.shaders.frag.gen0_df, "single-texture df fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    vk.shaders.vert.gen0_ident = SHADER_MODULE(vert_tx0_ident);
    vk.shaders.frag.gen0_ident = SHADER_MODULE(frag_tx0_ident);
    VK_SET_OBJECT_NAME(vk.shaders.vert.gen0_ident, "single-texture ident.color vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.frag.gen0_ident, "single-texture ident.color fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    vk.shaders.vert.light[0] = SHADER_MODULE(vert_light);
    vk.shaders.vert.light[1] = SHADER_MODULE(vert_light_fog);
    VK_SET_OBJECT_NAME(vk.shaders.vert.light[0], "light vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.vert.light[1], "light fog vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    vk.shaders.frag.light[0][0] = SHADER_MODULE(frag_light);
    vk.shaders.frag.light[0][1] = SHADER_MODULE(frag_light_fog);
    vk.shaders.frag.light[1][0] = SHADER_MODULE(frag_light_line);
    vk.shaders.frag.light[1][1] = SHADER_MODULE(frag_light_line_fog);
    VK_SET_OBJECT_NAME(vk.shaders.frag.light[0][0], "light fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.frag.light[0][1], "light fog fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.frag.light[1][0], "linear light fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.frag.light[1][1], "linear light fog fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    
    vk.shaders.color_vs[0] = SHADER_MODULE(color_vert_spv);
    vk.shaders.color_vs[1] = SHADER_MODULE(color_ghoul2_vert_spv);
    vk.shaders.color_fs = SHADER_MODULE(color_frag_spv);
    VK_SET_OBJECT_NAME(vk.shaders.color_vs[0], "single-color vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.color_vs[1], "single-color ghoul2 vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.color_fs, "single-color fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    vk.shaders.fog_vs[0] = SHADER_MODULE(fog_vert_spv);
    vk.shaders.fog_vs[1] = SHADER_MODULE(fog_ghoul2_vert_spv);
    vk.shaders.fog_fs = SHADER_MODULE(fog_frag_spv);
    VK_SET_OBJECT_NAME(vk.shaders.fog_vs[0], "fog-only vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.fog_vs[1], "fog-only ghoul2 vbo vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.fog_fs, "fog-only fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    


    vk.shaders.dot_vs = SHADER_MODULE(dot_vert_spv);
    vk.shaders.dot_fs = SHADER_MODULE(dot_frag_spv);
    VK_SET_OBJECT_NAME(vk.shaders.dot_vs, "dot vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.dot_fs, "dot fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    vk.shaders.bloom_fs = SHADER_MODULE(bloom_frag_spv);
    vk.shaders.blur_fs = SHADER_MODULE(blur_frag_spv);
    vk.shaders.blend_fs = SHADER_MODULE(blend_frag_spv);
    VK_SET_OBJECT_NAME(vk.shaders.bloom_fs, "bloom extraction fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.blur_fs, "gaussian blur fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.blend_fs, "final bloom blend fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    vk.shaders.gamma_fs = SHADER_MODULE(gamma_frag_spv);
    vk.shaders.gamma_vs = SHADER_MODULE(gamma_vert_spv);
    VK_SET_OBJECT_NAME(vk.shaders.gamma_fs, "gamma post-processing fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    VK_SET_OBJECT_NAME(vk.shaders.gamma_vs, "gamma post-processing vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

#ifdef VK_PBR_BRDFLUT
    vk.shaders.brdflut_fs = SHADER_MODULE(brdflut_frag_spv);
    VK_SET_OBJECT_NAME(vk.shaders.brdflut_fs, "brdf LUT fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
#endif

    vk.shaders.filtercube_vs = SHADER_MODULE(filtercube_vert_spv);
    VK_SET_OBJECT_NAME(vk.shaders.filtercube_vs, "filter cube vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    vk.shaders.prefilterenvmap_fs = SHADER_MODULE(prefilterenvmap_frag_spv);
    VK_SET_OBJECT_NAME(vk.shaders.prefilterenvmap_fs, "prefilter env map fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    vk.shaders.irradiancecube_fs = SHADER_MODULE(irradiancecube_frag_spv);
    VK_SET_OBJECT_NAME(vk.shaders.irradiancecube_fs, "irradiance cube fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);

    vk.shaders.filtercube_gm = SHADER_MODULE(filtercube_geom_spv);
    VK_SET_OBJECT_NAME(vk.shaders.filtercube_gm, "filter cube geometry shader", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
}

void vk_destroy_shader_modules( void )
{
    int i, j, k, l, m, n, o;

    for ( i = 0; i < 3; i++ ) {
        for ( j = 0; j < 2; j++ ) {
            for ( k = 0; k < 4; k++ ) {
                for ( l = 0; l < 3; l++ ) {
                    for ( m = 0; m < 2; m++ ) {
                        for ( n = 0; n < 2; n++ ) {
                            for ( o = 0; o < 2; o++ ) {
                                if ( vk.shaders.vert.gen[i][j][k][l][m][n][o] != VK_NULL_HANDLE ) {
                                    qvkDestroyShaderModule( vk.device, vk.shaders.vert.gen[i][j][k][l][m][n][o], NULL );
                                    vk.shaders.vert.gen[i][j][k][l][m][n][o] = VK_NULL_HANDLE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    for ( j = 0; j < 2; j++ ) {
        for ( k = 0; k < 4; k++ ) {
            for ( l = 0; l < 3; l++ ) {
                for ( m = 0; m < 2; m++ ) {
                    for ( n = 0; n < 2; n++ ) {
                        if ( vk.shaders.frag.gen[j][k][l][m][n] != VK_NULL_HANDLE ) {
                            qvkDestroyShaderModule( vk.device, vk.shaders.frag.gen[j][k][l][m][n], NULL );
                            vk.shaders.frag.gen[j][k][l][m][n] = VK_NULL_HANDLE;
                        }
                    }
                }
            }
        }
    }
    

    for (i = 0; i < 2; i++) {
        if (vk.shaders.vert.light[i] != VK_NULL_HANDLE) {
            qvkDestroyShaderModule(vk.device, vk.shaders.vert.light[i], NULL);
            vk.shaders.vert.light[i] = VK_NULL_HANDLE;
        }
        for (j = 0; j < 2; j++) {
            if (vk.shaders.frag.light[i][j] != VK_NULL_HANDLE) {
                qvkDestroyShaderModule(vk.device, vk.shaders.frag.light[i][j], NULL);
                vk.shaders.frag.light[i][j] = VK_NULL_HANDLE;
            }
        }
    }
    qvkDestroyShaderModule(vk.device, vk.shaders.vert.gen0_ident, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.frag.gen0_ident, NULL);

    qvkDestroyShaderModule(vk.device, vk.shaders.frag.gen0_df, NULL);

    qvkDestroyShaderModule(vk.device, vk.shaders.color_fs, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.color_vs[0], NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.color_vs[1], NULL);

    qvkDestroyShaderModule(vk.device, vk.shaders.fog_vs[0], NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.fog_fs, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.fog_vs[1], NULL);

    qvkDestroyShaderModule(vk.device, vk.shaders.dot_vs, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.dot_fs, NULL);

    qvkDestroyShaderModule(vk.device, vk.shaders.bloom_fs, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.blur_fs, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.blend_fs, NULL);

    qvkDestroyShaderModule(vk.device, vk.shaders.gamma_vs, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.gamma_fs, NULL);

#ifdef VK_PBR_BRDFLUT
    qvkDestroyShaderModule(vk.device, vk.shaders.brdflut_fs, NULL);
#endif

    qvkDestroyShaderModule(vk.device, vk.shaders.filtercube_vs, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.filtercube_gm, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.prefilterenvmap_fs, NULL);
    qvkDestroyShaderModule(vk.device, vk.shaders.irradiancecube_fs, NULL);
}