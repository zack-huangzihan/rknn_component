cmake_minimum_required(VERSION 2.8.12)

PROJECT(RKNN_AI_DEMO)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(PkgConfig REQUIRED)
pkg_search_module(EGL REQUIRED egl)
pkg_search_module(EGLESV2 REQUIRED glesv2)
#pkg_search_module(MALI REQUIRED mali)
pkg_search_module(X11 REQUIRED x11)

set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/opencv/opencv-linux-aarch64/share/OpenCV)
find_package(OpenCV REQUIRED)

set(RockX_DIR ${CMAKE_SOURCE_DIR}/3rdparty/rockx-rk3588-Linux)
find_package(RockX REQUIRED)
include_directories(${RockX_INCLUDE_DIRS})

include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(${EGL_INCLUDE_DIRS})
include_directories(${EGLESV2_INCLUDE_DIRS})
#include_directories(${MALI_INCLUDE_DIRS})
include_directories(${X11_INCLUDE_DIRS})
#include_directories(${CMAKE_SOURCE_DIR}/3rdparty/opencv/opencv-linux-aarch64/include)

set(SRC
      src/x11_background_blur.cpp
      src/v4l_achieve.cpp
      src/v4l2_camera.cpp
	)

add_executable(background_blur ${SRC})

target_link_libraries(background_blur 
	${EGL_LIBRARIES}
	${EGLESV2_LIBRARIES}
	${X11_LIBRARIES}
	${OpenCV_LIBS}
	${RockX_LIBS}
)