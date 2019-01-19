cmake_minimum_required(VERSION 3.1)
project(MCRenderer CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

add_definitions(-DDGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_CTOR_INIT -DGLM_ENABLE_EXPERIMENTAL -DGLM_FORCE_RADIANS -DZLIB_CONST -DVK_NO_PROTOTYPES -DVMA_STATIC_VULKAN_FUNCTIONS=0)

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
find_package(LibZip REQUIRED)

find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h PATHS ${HEADER_SEARCH_PATH} "$ENV{VK_SDK_PATH}/Include")

if (${MSVC})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4250 /wd4251 /wd4307 /wd4244 /wd4267 /D_CRT_SECURE_NO_WARNINGS /D_SCL_SECURE_NO_WARNINGS /std:c++latest")
endif()

add_executable(mcrenderer ${SOURCE_FILES})

#Compiler specific flags
if (CMAKE_COMPILER_IS_GNUCXX)
	target_compile_options(mcrenderer BEFORE PUBLIC -Wall -Wextra -Wshadow -pedantic -Wno-unused-parameter
		-Wno-missing-field-initializers -Wnon-virtual-dtor -std=c++17)
	set(EXTRA_LIBS stdc++fs sndio X11-xcb)
endif()

#Sets link libraries
target_link_libraries(mcrenderer
	${SDL2_LIBRARY}
	${FREETYPE_LIBRARIES}
	${ZLIB_LIBRARIES}
	${NOISE_LIBRARY}
	${LIBZIP_LIBRARY}
	${CMAKE_THREAD_LIBS_INIT}
	${CMAKE_DL_LIBS}
	${EXTRA_LIBS}
)

#Sets include directories
target_include_directories(mcrenderer SYSTEM PUBLIC
	${SDL2_INCLUDE_DIR}
	${VULKAN_INCLUDE_DIR}
	${GLM_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
	${LIBZIP_INCLUDE_DIR_ZIP}
	${ZLIB_INCLUDE_DIRS}
	${NOISE_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/inc
)


