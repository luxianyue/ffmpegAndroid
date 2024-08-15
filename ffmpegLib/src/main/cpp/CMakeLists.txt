# 设置构建本机库文件所需的 CMake的最小版本
cmake_minimum_required(VERSION 3.18.1)

project("useffmpeg")

#找到所有的源文件，存放在全局变量中
add_library(libavcodec IMPORTED SHARED)
add_library(libavdevice IMPORTED SHARED)
add_library(libavfilter IMPORTED SHARED)
add_library(libavformat IMPORTED SHARED)
add_library(libavutil IMPORTED SHARED)
add_library(libswresample IMPORTED SHARED)
add_library(libswscale IMPORTED SHARED)
set(libs_dir "${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/output/lib/${CMAKE_ANDROID_ARCH_ABI}")
set_target_properties(libavcodec PROPERTIES IMPORTED_LOCATION "${libs_dir}/libavcodec.so")
set_target_properties(libavdevice PROPERTIES IMPORTED_LOCATION "${libs_dir}/libavdevice.so")
set_target_properties(libavfilter PROPERTIES IMPORTED_LOCATION "${libs_dir}/libavfilter.so")
set_target_properties(libavformat PROPERTIES IMPORTED_LOCATION "${libs_dir}/libavformat.so")
set_target_properties(libavutil PROPERTIES IMPORTED_LOCATION "${libs_dir}/libavutil.so")
set_target_properties(libswresample PROPERTIES IMPORTED_LOCATION "${libs_dir}/libswresample.so")
set_target_properties(libswscale PROPERTIES IMPORTED_LOCATION "${libs_dir}/libswscale.so")


include_directories(include ffmpeg/output/include ffmpeg/output/include/${CMAKE_ANDROID_ARCH_ABI})
file(GLOB CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/output/include/*.c)
file(GLOB YUV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libyuv/*.cc)

# 添加自己写的 C/C++源文件
add_library(
        my_native #so名称
        SHARED #动态库
        ${CPP_DIR}
        ${FFMPEG_DIR}
        ${YUV_DIR}
        )

#  依赖 NDK中自带的log库
find_library(log-lib log)

#  链接库
target_link_libraries(
        my_native
        libavcodec
        libavdevice
        libavfilter
        libavformat
        libavutil
        libswresample
        libswscale
        jnigraphics
        ${log-lib})