@echo off

set "VSCMD_START_DIR=%CD%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"

set tools_dir=tools
set bh=%tools_dir%\bin2hex.exe
set bh_cpp=%tools_dir%\bin2hex.c
if not exist %bh% (
    cl.exe /EHsc /nologo /Fe%tools_dir%\ /Fo%tools_dir%\ %bh_cpp%
)

set bs=%tools_dir%\bindshader.exe
set bs_cpp=%tools_dir%\bindshader.c
if not exist %bs% (
    cl.exe /EHsc /nologo /Fe%tools_dir%\ /Fo%tools_dir%\ %bs_cpp%
)

set PATH=%tools_dir%;%PATH%

set glsl_rt=glsl\rtx\raytracer\
set glsl_comp=glsl\rtx\compute\
set glsl_vert=glsl\rtx\vert\
set cl=%VULKAN_SDK%\Bin\glslangValidator.exe
set tmpf=spirv\data_rtx.spv
set outf=+spirv\shader_data_rtx.c
set outfb=+spirv\shader_binding_rtx.c

echo %bh%

mkdir spirv

del /Q spirv\shader_data_rtx.c
del /Q spirv\shader_binding_rtx.c
del /Q "%tmpf%"

@rem compile individual shaders

set "flags=-DGLSL -DUSE_VK_IMGUI"

for %%f in (%glsl_vert%*.vert) do (
    "%cl%" -S vert --target-env vulkan1.2 -V -o "%tmpf%" "%%f" %flags%
    "%bh%" "%tmpf%" %outf% %%~nf_vert
    del /Q "%tmpf%"
)

for %%f in (%glsl_vert%*.frag) do (
    "%cl%" -S frag --target-env vulkan1.2 -V -o "%tmpf%" "%%f" %flags%
    "%bh%" "%tmpf%" %outf% %%~nf_frag
    del /Q "%tmpf%"
)

for %%f in (%glsl_comp%*.comp) do (
    "%cl%" -S comp --target-env vulkan1.2 -V -o "%tmpf%" "%%f" %flags%
    "%bh%" "%tmpf%" %outf% %%~nf_comp
    del /Q "%tmpf%"
)

for %%f in (%glsl_rt%*.rgen) do (
    echo %%f
    "%cl%" -S rgen --target-env vulkan1.2 -V -o "%tmpf%" "%%f" %flags%
    "%bh%" "%tmpf%" %outf% %%~nf_rgen
    del /Q "%tmpf%"
)

for %%f in (%glsl_rt%*.rmiss) do (
    "%cl%" -S rmiss --target-env vulkan1.2 -V -o "%tmpf%" "%%f" %flags%
    "%bh%" "%tmpf%" %outf% %%~nf_rmiss
    del /Q "%tmpf%"
)

for %%f in (%glsl_rt%*.rchit) do (
    "%cl%" -S rchit --target-env vulkan1.2 -V -o "%tmpf%" "%%f" %flags%
    "%bh%" "%tmpf%" %outf% %%~nf_rchit
    del /Q "%tmpf%"
)

for %%f in (%glsl_rt%*.rahit) do (
    "%cl%" -S rahit --target-env vulkan1.2 -V -o "%tmpf%" "%%f" %flags%
    "%bh%" "%tmpf%" %outf% %%~nf_rahit
    del /Q "%tmpf%"
)

pause