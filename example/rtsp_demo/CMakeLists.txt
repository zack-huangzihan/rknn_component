cmake_minimum_required(VERSION 2.8.12)

PROJECT(RTSP_DEMO)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
#set(GCC_COMPILER_PATH /Compile link path)
#set(CMAKE_C_COMPILER ${GCC_COMPILER_PATH}/bin/arm-linux-gnueabihf-gcc)
#set(CMAKE_CXX_COMPILER ${GCC_COMPILER_PATH}/bin/arm-linux-gnueabihf-g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(PkgConfig REQUIRED)
pkg_search_module(GST_VIDEO REQUIRED gstreamer-video-1.0) #use gstreamer-video-1.0,It will contain gstreamer-1.0
pkg_search_module(GST_APP REQUIRED gstreamer-app-1.0)

include_directories(${GST_VIDEO_INCLUDE_DIRS})
include_directories(${GST_APP_INCLUDE_DIRS})

set(RTSP_GST_SRC
	rtsp_gst.cc
)

set(RTSP_GST_XV_SRC
	rtsp_gst_xv.cc
)

set(RTSP_GST_APP_XV_SRC
	rtsp_gst_app_xv.cc
)
# RTSP_DEMO
add_executable(rtsp_gst ${RTSP_GST_SRC})
add_executable(rtsp_gst_xv ${RTSP_GST_XV_SRC})
add_executable(rtsp_gst_appsink_xv ${RTSP_GST_APP_XV_SRC})

target_link_libraries(rtsp_gst 
	${GST_VIDEO_LIBRARIES}
	${GST_APP_LIBRARIES}
)

target_link_libraries(rtsp_gst_xv 
	${GST_VIDEO_LIBRARIES}
	${GST_APP_LIBRARIES}
)

target_link_libraries(rtsp_gst_appsink_xv 
	${GST_VIDEO_LIBRARIES}
	${GST_APP_LIBRARIES}
)