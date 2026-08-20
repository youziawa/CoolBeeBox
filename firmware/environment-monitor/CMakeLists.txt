# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(CoolBeeBox)

# Work around occasional GCC ICE in esp_lcd/rgb_panel_draw_bitmap.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	idf_component_get_property(esp_lcd_lib esp_lcd COMPONENT_LIB)
	if(esp_lcd_lib)
		target_compile_options(${esp_lcd_lib} PRIVATE -O1 -fno-tree-sra)
	endif()
endif()
