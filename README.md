# EternalJK
[![build](https://github.com/JKSunny/EternalJK/actions/workflows/build.yml/badge.svg)](https://github.com/JKSunny/EternalJK/actions/workflows/build.yml)

This fork focuses on the jaPRO integration and Client Engine modifications.

If you have any suggestions or would like to submit a bug report, please post them in [issues](https://github.com/eternalcodes/EternalJK/issues).

[![Website](https://img.shields.io/badge/website-japro-brightgreen.svg)](http://playja.pro) [![Fork](https://img.shields.io/badge/repository-japro%20game%20library-blue.svg)](https://github.com/videoP/jaPRO)

## License

[![License](https://img.shields.io/github/license/eternalcodes/EternalJK.svg)](https://github.com/eternalcodes/EternalJK/blob/master/LICENSE.txt)

OpenJK is licensed under GPLv2 as free software. You are free to use, modify and redistribute OpenJK following the terms in LICENSE.txt.


## For players

Installing and running EternalJK:

1. [Download the latest release](https://github.com/eternalcodes/EternalJK/releases).
2. Extract the file into the Jedi Academy `GameData` folder. For Steam users, this will be in `<Steam Folder>/steamapps/common/Jedi Academy/GameData/`.
3. Run eternaljk.x86.exe (Rename to jamp.exe for better steam support)

## For Developers

### Contributing to EternalJK
* [Fork](https://github.com/eternalcodes/EternalJK/fork) the EternalJK project on GitHub
* Create a new branch on your fork and make your changes
* Send a [pull request](https://help.github.com/articles/creating-a-pull-request) to upstream (eternaljk/master)

### Vulkan support
Support Initially started by porting to [Quake-III-Arena-Kenny-Edition](https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition).<br />
After that, I found [vkQuake3](https://github.com/suijingfeng/vkQuake3/tree/master/code), hence the file structure.

Lastly, I stumbled across [Quake3e](https://github.com/ec-/Quake3e).<br />
Which is highly maintained, and is packed with many additions compared to the other repositories.

Therefore the vulkan renderer is now based on Quake3e. <br />A list of the additions can be found on [here](https://github.com/ec-/Quake3e#user-content-vulkan-renderer).

Many thanks to all the contributors that worked & are still working hard on improving the Q3 engine!

### ImGui support 
Press F1 to toggle input

**Dear ImGui**: Bloat-free Graphical User interface for C++ with minimal dependencies<br />
[Repository](https://github.com/ocornut/imgui) - 
[License](https://github.com/ocornut/imgui/blob/master/LICENSE.txt)

**cimgui**: Thin c-api wrapper for c++ Dear ImGui<br />
[Repository](https://github.com/cimgui/cimgui) - 
[License](https://github.com/cimgui/cimgui/blob/docking_inter/LICENSE)

**Legit profiler**: ImGui helper class for simple profiler histogram<br />
[Repository](https://github.com/Raikiri/LegitProfiler) - 
[License](https://github.com/Raikiri/LegitProfiler/blob/master/LICENSE.txt)

**ImGuiColorTextEdit**: Syntax highlighting text editor for ImGui<br />
[Repository](https://github.com/BalazsJako/ImGuiColorTextEdit) - 
[License](https://github.com/BalazsJako/ImGuiColorTextEdit/blob/master/LICENSE)

**ImNodeFlow**: Node-based editor/blueprints for ImGui<br />
[Repository](https://github.com/Fattorino/ImNodeFlow) - 
[License](https://github.com/Fattorino/ImNodeFlow/blob/master/LICENSE.txt)

**NetRadiant-custom**: The open-source, cross-platform level editor for id Tech based games.<br />
*- As Inspiration for auto-complete -*<br />
[Commit](https://github.com/Garux/netradiant-custom/commit/9c2fbc9d1dd4029c0f76aec2830f991fcb2c259e) -
[Repository](https://github.com/Garux/netradiant-custom/tree/master) -
[License](https://github.com/Raikiri/LegitProfiler/blob/master/LICENSE.txt)
## Maintainers

* [eternal](https://github.com/eternalcodes)

## Contributors 
* [bucky](https://github.com/Bucky21659)
* [loda](https://github.com/videoP)
