# Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
#
# This source file is the property of Axera Semiconductor Co., Ltd. and
# may not be copied or distributed in any isomorphic form without the prior
# written consent of Axera Semiconductor Co., Ltd.
#
# Author: wanglusheng@axera-tech.com
#

# common function to add samples
function (add_sample name)
    add_executable (${name})

    # add source files
    foreach (file IN LISTS ARGN)
        target_sources (${name} PRIVATE ${file})
    endforeach()

    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/include)

    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/cmdline/include)

    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/axcl/include)
    target_link_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/axcl/lib)
    target_link_libraries (${name} axcl_runtime axcl_mpl ax_sys ax_engine ax_interpreter ax_pcie_msg ax_pcie_dma)

    set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/opencv/lib/cmake/opencv4)
    find_package(OpenCV REQUIRED)
    target_include_directories(${name} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${name} ${OpenCV_LIBS})
#    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/opencv/include/opencv4)
#    target_link_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/opencv/lib ${CMAKE_SOURCE_DIR}/3rdparty/opencv/lib/opencv4/3rdparty)
#    target_link_libraries (${name} opencv_imgcodecs opencv_imgproc opencv_core dl m pthread rt libjpeg-turbo libwebp libpng libtiff libopenjp2 zlib)
endfunction ()

# set names
set (SAMPLE_CLASSIFICATION sample_axclrt_classification)
set (SAMPLE_DETECTION sample_axclrt_detection)

# add samples
add_sample (${SAMPLE_CLASSIFICATION} sample_axclrt_classification.cc)
add_sample (${SAMPLE_DETECTION} sample_axclrt_yolov5s.cc)
