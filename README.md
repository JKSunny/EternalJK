# EternalJK
[![build](https://github.com/JKSunny/EternalJK/actions/workflows/build.yml/badge.svg)](https://github.com/JKSunny/EternalJK/actions/workflows/build.yml)

## License

[![License](https://img.shields.io/github/license/eternalcodes/EternalJK.svg)](https://github.com/eternalcodes/EternalJK/blob/master/LICENSE.txt)

OpenJK is licensed under GPLv2 as free software. You are free to use, modify and redistribute OpenJK following the terms in LICENSE.txt.

## For Developers

### Contributing to EternalJK
* [Fork](https://github.com/JKSunny/EternalJK/fork) the EternalJK project on GitHub
* Create a new branch on your fork and make your changes
* Send a [pull request](https://help.github.com/articles/creating-a-pull-request) to upstream (eternaljk/master)

### Vulkan support
Support Initially started by porting to [Quake-III-Arena-Kenny-Edition](https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition).<br />
After that, I found [vkQuake3](https://github.com/suijingfeng/vkQuake3/tree/master/code), hence the file structure.

Lastly, I stumbled across [Quake3e](https://github.com/ec-/Quake3e).<br />
Which is highly maintained, and is packed with many additions compared to the other repositories.

Therefore the vulkan renderer is now based on Quake3e. <br />A list of the additions can be found on [here](https://github.com/ec-/Quake3e#user-content-vulkan-renderer).

Many thanks to all the contributors that worked & are still working hard on improving the Q3 engine!

### Raytracing support
> **NOTE** This is Work-in-progress, its lacking features and is unstable. \
> E.g. switching map requires a full restart because buffer clearing is not implemented yet. \
> - To-do list will follow.

I wanted to learn more on the topic of RayTracing. \
The current implementation is not yet complete, but the essential functionality should be there. \
It needs more TLC, due to time constraints I have decided to put this project on hold.

I believe its a waste to let this codebase collect dust, and decided to share it. \
Because the Jedi Knight community remains highly active, with further development, this project <ins>could</ins> become stable and playable, either through my efforts or with your involvement. 

I used [Quake-III-Arena-R](https://github.com/fknfilewalker/Quake-III-Arena-R) as a starting point, however transitioned to [Q2RTX](https://github.com/NVIDIA/Q2RTX) relativly quickly. \
[Q2RTX](https://github.com/NVIDIA/Q2RTX) is active and receives frequent updates. \
Also the code base is very clear plus I like the RayTraced result!
I can highly recommend you to give their great effort a try if you have not done so already.

**Main differences in this codebase:**

> Q2RTX implemented their own material system. \
> However I wanted to use the native .shader and support the .mtr material system from **Rend2** \
> I did a quick mockup of that, I have not added multistage support yet.

> Ghoul2 support is added *(No MD3 yet)*, and uses the same IBO's and VBO's used for **Beta** and **PBR** branch with minor tweaks. \
> Q2RTX uses a single buffer for the IBO and VBO and does have Ghoul2 support. \
> I did port this method, but it is define guarded and can safely be removed.

> BSP level loading and buffers differ. \
> This should however be looked at and transitioned to Q2RTX method of tlas and blas creation *(top/bottom level accel structure)*

> **GETTING STARTED** \
> Set cvar r_normalMapping 1, r_specularMapping 1, r_hdr 0, r_fullscreen 1 and r_vertexLight 2 
> Everything is define guarded using **USE_RTX** in the codebase

> **IMPORTANT** Requires assets from [Q2RTX on Steam](https://store.steampowered.com/app/1089130/Quake_II_RTX/) \
> Download and locate the install folder and open baseq2 folder \
> Locate and open blue_noise.pkz and copy the folder "blue_noise" to the base folder of your Jedi Academy install folder. \
> Do the same for the folder "env" in q2rtx_media.pkz. \
> You should now have the following file structure: GameData/base/blue_noise and GameData/base/env \
> *~ Give Q2RTX a try as well*

If you made it this far reading, thank you for your time and hopefully this may be of any use in the future.

Sources: \
[Quake-III-Arena-R](https://github.com/fknfilewalker/Quake-III-Arena-R) \
[Q2RTX](https://github.com/NVIDIA/Q2RTX) - [License](https://github.com/NVIDIA/Q2RTX/blob/master/license.txt)

References: \
[Vulkan Specification](https://registry.khronos.org/vulkan/specs/1.2-extensions/html/vkspec.html) \
[SaschaWillems - Vulkan](https://github.com/SaschaWillems/Vulkan)

### ImGui support 
Press F1 to toggle input

Dear ImGui: Bloat-free Graphical User interface for C++ with minimal dependencies<br />
[Repository](https://github.com/ocornut/imgui) - 
[License](https://github.com/ocornut/imgui/blob/master/LICENSE.txt)

cimgui: Thin c-api wrapper for c++ Dear ImGui<br />
[Repository](https://github.com/cimgui/cimgui) - 
[License](https://github.com/cimgui/cimgui/blob/docking_inter/LICENSE)

## Maintainers

* [eternal](https://github.com/eternalcodes)

## Contributors 
* [bucky](https://github.com/Bucky21659)
* [loda](https://github.com/videoP)