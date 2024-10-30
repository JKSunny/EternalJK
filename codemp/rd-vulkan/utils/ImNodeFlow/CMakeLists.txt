cmake_minimum_required(VERSION 3.26)

# CREATE PROJECT
project(ImNodeFlow)

add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

# SET SOURCE FILES FOR PROJECT
file(GLOB_RECURSE _HDRS "include/*.h")
file(GLOB_RECURSE _SRCS "src/*.cpp" "src/*.h" "src/*.inl")

# CREATE LIBRARY FROM SOURCE_FILES
add_library(ImNodeFlow ${_SRCS} ${_HDRS})

add_subdirectory(../../../../lib/imgui)

# LINK CONAN LIBS
target_link_libraries(ImNodeFlow bundled_libimgui)

# PREP TO USE "#include <>"
target_include_directories(ImNodeFlow PUBLIC include)
