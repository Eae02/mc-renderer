cmake_minimum_required(VERSION 3.1)
project(MCRenderer CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

add_definitions(-DDGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_RADIANS -DZLIB_CONST -DVK_NO_PROTOTYPES)

message(${CMAKE_CXX_COMPILER})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMCR_DEBUG")

file(GLOB_RECURSE SOURCE_FILES
		${CMAKE_SOURCE_DIR}/src/*.cpp
		${CMAKE_SOURCE_DIR}/src/*.h
		${CMAKE_SOURCE_DIR}/src/*.inl
)

#Finds libraries
find_package(SDL2 REQUIRED)
find_package(GLM REQUIRED)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
find_package(Libnoise REQUIRED)

find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h PATHS ${HEADER_SEARCH_PATH} "$ENV{VK_SDK_PATH}/Include")

#Compiler specific flags
if (CMAKE_COMPILER_IS_GNUCXX)
	add_compile_options(-Wall -Wextra -Wshadow -pedantic -Wno-unused-parameter -Wno-missing-field-initializers
	                    $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor> $<$<COMPILE_LANGUAGE:CXX>:-std=c++1z>)
	link_libraries(stdc++fs)
	
	set(CMAKE_INSTALL_RPATH "$ORIGIN")
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

if (${MSVC})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4250 /wd4251 /wd4307 /D_CRT_SECURE_NO_WARNINGS /D_SCL_SECURE_NO_WARNINGS")
	
	set(ASSIMP_LIBRARIES "${_VCPKG_INSTALLED_DIR}/x64-windows/lib/assimp-vc140-mt.lib")
endif()

#Sets link libraries
link_libraries(
	${SDL2_LIBRARY}
	${FREETYPE_LIBRARIES}
	${ZLIB_LIBRARIES}
	${NOISE_LIBRARY}
	${CMAKE_THREAD_LIBS_INIT}
)

#Sets include directories
include_directories(SYSTEM
	${SDL2_INCLUDE_DIRS}
	${VULKAN_INCLUDE_DIR}
	${GLM_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${NOISE_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/inc
)

add_executable(mcrenderer ${SOURCE_FILES})
