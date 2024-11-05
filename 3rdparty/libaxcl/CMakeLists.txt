# Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
#
# This source file is the property of Axera Semiconductor Co., Ltd. and
# may not be copied or distributed in any isomorphic form without the prior
# written consent of Axera Semiconductor Co., Ltd.
#
# Author: liujunda@axera-tech.com
#

axcl_add_library (libaxclrt INTERFACE
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/bsp
    LINK_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
    LINK_LIBRARIES
        axcl_rt
        axcl_pkg
        axcl_comm
        spdlog
        axcl_token
        axcl_pcie_msg
        axcl_pcie_dma
    ALIAS
        axcl::rt
)
