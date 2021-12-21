cmake_minimum_required(VERSION 2.8.12)

PROJECT(RKNN_AI_DEMO)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
#set(GCC_COMPILER_PATH /home/huangzihan/linux-release/youdao_1109/buildroot/output/rockchip_rv1126_rv1109/host)
#set(CMAKE_C_COMPILER ${GCC_COMPILER_PATH}/bin/arm-linux-gnueabihf-gcc)
#set(CMAKE_CXX_COMPILER ${GCC_COMPILER_PATH}/bin/arm-linux-gnueabihf-g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#find gtk+-2.0
find_package(PkgConfig REQUIRED)
#pkg_search_module(GTK2 REQUIRED gtk+-2.0)
pkg_search_module(GTK3 REQUIRED gtk+-3.0)
pkg_search_module(GST_VIDEO REQUIRED gstreamer-video-1.0) #use gstreamer-video-1.0,It will contain gstreamer-1.0
pkg_search_module(GST_APP REQUIRED gstreamer-app-1.0)

include_directories(${GTK3_INCLUDE_DIRS})
include_directories(${GST_VIDEO_INCLUDE_DIRS})
include_directories(${GST_APP_INCLUDE_DIRS})

set(RKNN_AI_DEMO_SRC
      src/main.c
	)

# RKNN_AI_DEMO
add_executable(rknn_ai_demo ${RKNN_AI_DEMO_SRC})
#add_executable(test src/test.c)

target_link_libraries(rknn_ai_demo 
	#${GTK2_LIBRARIES}
	${GTK3_LIBRARIES}
	${GST_VIDEO_LIBRARIES}
	${GST_APP_LIBRARIES}
	)

#target_link_libraries(test 
	#${GTK2_LIBRARIES}
#	${GTK3_LIBRARIES}
#	${GST_VIDEO_LIBRARIES}
#	${GST_APP_LIBRARIES}
#	)