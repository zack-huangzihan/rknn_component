
set(RockX_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)

if (CMAKE_SYSTEM_NAME MATCHES "Android")
    file(GLOB RockX_LIBS ${CMAKE_CURRENT_LIST_DIR}/${CMAKE_ANDROID_ARCH_ABI}/*.so)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(RockX_LIBS
        ${CMAKE_CURRENT_LIST_DIR}/lib64/librknn_api.dylib
        ${CMAKE_CURRENT_LIST_DIR}/lib64/librockx.dylib
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(RockX_LIBS
        ${CMAKE_CURRENT_LIST_DIR}/lib64/librknn_api.dll
        ${CMAKE_CURRENT_LIST_DIR}/lib64/librockx.dll
    )
else ()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set (TARGET_LIB_ARCH lib64)
    else()
        set (TARGET_LIB_ARCH lib)
    endif()
    file(GLOB RockX_LIBS ${CMAKE_CURRENT_LIST_DIR}/${TARGET_LIB_ARCH}/*.so)
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/lib/npu_transfer_proxy")
    file(GLOB NPU_TRANSFER_PROXY_FILES "${CMAKE_CURRENT_LIST_DIR}/lib/npu_transfer_proxy/*")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../rockx-data-rk3588")
    file(GLOB ROCKX_DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/../rockx-data-rk3588/*")
endif()

set(RockX_BINS ${NPU_TRANSFER_PROXY_FILES})
set(RockX_DATA ${ROCKX_DATA_FILES})