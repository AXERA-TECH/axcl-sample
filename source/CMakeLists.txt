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
    aux_source_directory (${CMAKE_SOURCE_DIR}/include/utils SRCS)
    aux_source_directory (${CMAKE_SOURCE_DIR}/include/ax_model_runner SRCS)
    add_executable (${name})

    target_sources (${name} PRIVATE ${SRCS})
    # add source files
    foreach (file IN LISTS ARGN)
        target_sources (${name} PRIVATE ${file})
    endforeach()

    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/include/ax_model_runner)
    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/include/utils)
    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/cmdline/include)

    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/libaxcl/bsp)
    target_include_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/libaxcl/include)
    target_link_directories (${name} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/libaxcl/lib)
    target_link_libraries (${name} 
    axcl_rt
    axcl_pkg
    axcl_comm
    spdlog
    axcl_token
    axcl_pcie_msg
    axcl_pcie_dma)

    set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/libopencv-4.5.5-aarch64/lib/cmake/opencv4)
    find_package(OpenCV REQUIRED)
    target_include_directories(${name} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${name} ${OpenCV_LIBS})

    set_target_properties(${name}
       PROPERTIES
       LINK_FLAGS "-Wl,-rpath,$ORIGIN/../3rdparty/libaxcl/lib/"
    )
endfunction ()

# set names
set (SAMPLE_CLASSIFICATION sample_axclrt_classification)
set (SAMPLE_DETECTION sample_axclrt_detection)

# add samples
add_sample (${SAMPLE_CLASSIFICATION} sample_axclrt_classification.cc)
add_sample (${SAMPLE_DETECTION} sample_axclrt_yolov5s.cc)

# install targets
list(APPEND BINS 
    ${SAMPLE_CLASSIFICATION}
    ${SAMPLE_DETECTION}
    )
    
install(TARGETS ${BINS}
    RUNTIME
        DESTINATION ./)