@echo off

set "VSCMD_START_DIR=%CD%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"

set tools_dir=tools
set bh=%tools_dir%\bin2hex.exe
set bh_cpp=%tools_dir%\bin2hex.c
if not exist %bh% (
    cl.exe /EHsc /nologo /Fe%tools_dir%\ /Fo%tools_dir%\ %bh_cpp%
)

set PATH=%tools_dir%;%PATH%

set glsl=glsl\
set cl=%VULKAN_SDK%\glslangValidator.exe
set tmpf=spirv\data.spv
set outf=+spirv\shader_data.c

echo %bh%

mkdir spirv

del /Q spirv\shader_data.c
del /Q "%tmpf%"

@rem compile individual shaders

for %%f in (%glsl%*.vert) do (
    "%cl%" -S vert -V -o "%tmpf%" "%%f"
    "%bh%" "%tmpf%" %outf% %%~nf_vert_spv
    del /Q "%tmpf%"
)

for %%f in (%glsl%*.frag) do (
    "%cl%" -S frag -V -o "%tmpf%" "%%f"
    "%bh%" "%tmpf%" %outf% %%~nf_frag_spv
    del /Q "%tmpf%"
)

for %%f in (%glsl%*.geom) do (
    "%cl%" -S geom -V -o "%tmpf%" "%%f"
    "%bh%" "%tmpf%" %outf% %%~nf_geom_spv
    del /Q "%tmpf%"
)

"%cl%" -S vert -V -o "%tmpf%" %glsl%gen_vert.tmpl -DUSE_CL0_IDENT
"%bh%" "%tmpf%" %outf% vert_tx0_ident

"%cl%" -S frag -V -o "%tmpf%" %glsl%gen_frag.tmpl -DUSE_CL0_IDENT
"%bh%" "%tmpf%" %outf% frag_tx0_ident

"%cl%" -S frag -V -o "%tmpf%" %glsl%gen_frag.tmpl -DUSE_ATEST -DUSE_DF
"%bh%" "%tmpf%" %outf% frag_tx0_df

@rem compile lighting shader variations from templates

"%cl%" -S vert -V -o "%tmpf%" %glsl%light_vert.tmpl
"%bh%" "%tmpf%" %outf% vert_light

"%cl%" -S vert -V -o "%tmpf%" %glsl%light_vert.tmpl -DUSE_FOG
"%bh%" "%tmpf%" %outf% vert_light_fog

"%cl%" -S frag -V -o "%tmpf%" %glsl%light_frag.tmpl
"%bh%" "%tmpf%" %outf% frag_light

"%cl%" -S frag -V -o "%tmpf%" %glsl%light_frag.tmpl -DUSE_FOG 
"%bh%" "%tmpf%" %outf% frag_light_fog

"%cl%" -S frag -V -o "%tmpf%" %glsl%light_frag.tmpl -DUSE_LINE
"%bh%" "%tmpf%" %outf% frag_light_line

"%cl%" -S frag -V -o "%tmpf%" %glsl%light_frag.tmpl -DUSE_LINE -DUSE_FOG
"%bh%" "%tmpf%" %outf% frag_light_line_fog

@rem template shader identifiers and flags
set "vbo[0]="
set "vbo[1]=-DUSE_VBO_GHOUL2"
set "vbo[2]=-DUSE_VBO_MDV"
set "vbo_id[0]=cpu_"
set "vbo_id[1]=gpu_ghoul2_"
set "vbo_id[2]=gpu_mdv_"

set "pbr[0]="
set "pbr[1]=-DUSE_VK_PBR"
set "pbr_id[0]="
set "pbr_id[1]=pbr_"

set "tx[0]="
set "tx[1]=-DUSE_TX1"
set "tx[2]=-DUSE_TX2"
set "tx_id[0]=tx0"
set "tx_id[1]=tx1"
set "tx_id[2]=tx2"

set "cl[1]=-DUSE_CL1"
set "cl[2]=-DUSE_CL2"
set "cl_id[1]=cl"
set "cl_id[2]=cl"

set "env[0]="
set "env[1]=-DUSE_ENV"
set "env_id[0]="
set "env_id[1]=_env"

set "fog[0]="
set "fog[1]=-DUSE_FOG"
set "fog_id[0]="
set "fog_id[1]=_fog"

SETLOCAL EnableDelayedExpansion

@rem compile generic shader variations from templates
@rem vertex shader
for /L %%i in ( 0,1,2 ) do (                	@rem vbo 
	for /L %%j in ( 0,1,1 ) do (                @rem pbr 
		for /L %%k in ( 0,1,2 ) do (            @rem tx   
			for /L %%l in ( 0,1,1 ) do (        @rem +env
				for /L %%m in ( 0,1,1 ) do (    @rem +fog
					call :compile_vertex_shader %%i, %%j, %%k, %%l, %%m
				)
			)
		)
	)
)

@rem fragment shader
for /L %%i in ( 0,1,2 ) do (                	@rem vbo
	for /L %%j in ( 0,1,1 ) do (                @rem pbr
		for /L %%k in ( 0,1,2 ) do (            @rem tx 
			for /L %%l in ( 0,1,1 ) do (        @rem +fog
				call :compile_fragment_shader %%i, %%j, %%k, %%l
			)
		)
	)
)

del /Q "%tmpf%"

pause

:compile_fragment_shader
    set "flags=!vbo[%1]! !pbr[%2]! !tx[%3]! !fog[%4]!"
    set "name=!vbo_id[%1]!!pbr_id[%2]!!tx_id[%3]!!fog_id[%4]!"
    if %3 equ 0 ( set "flags=%flags% -DUSE_ATEST" )

    "%cl%" -S frag -V -o "%tmpf%" %glsl%gen_frag.tmpl %flags%
    "%bh%" "%tmpf%" %outf% frag_%name%

    @rem +cl
    if %3 equ 0 goto continue
        "%cl%" -S frag -V -o "%tmpf%" %glsl%gen_frag.tmpl !vbo[%1]! !pbr[%2]! !tx[%3]! !cl[%3]! !fog[%4]!
        "%bh%" "%tmpf%" %outf% frag_!vbo_id[%1]!!pbr_id[%2]!!tx_id[%3]!_!cl_id[%3]!!fog_id[%4]!
    :continue
exit /B

:compile_vertex_shader
    "%cl%" -S vert -V -o "%tmpf%" %glsl%gen_vert.tmpl !vbo[%1]! !pbr[%2]! !tx[%3]! !env[%4]! !fog[%5]!
    "%bh%" "%tmpf%" %outf% vert_!vbo_id[%1]!!pbr_id[%2]!!tx_id[%3]!!env_id[%4]!!fog_id[%5]!

    @rem +cl
    if %3 equ 0 goto continue
        "%cl%" -S vert -V -o "%tmpf%" %glsl%gen_vert.tmpl !vbo[%1]! !pbr[%2]! !tx[%3]! !cl[%3]! !env[%4]! !fog[%5]!
        "%bh%" "%tmpf%" %outf% vert_!vbo_id[%1]!!pbr_id[%2]!!tx_id[%3]!_!cl_id[%3]!!env_id[%4]!!fog_id[%5]!
    :continue
exit /B