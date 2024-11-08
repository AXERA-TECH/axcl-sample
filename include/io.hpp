/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <cstdio>
#include <cstring>
#include <vector>
#include <utility>

#include <axcl.h>

#define AX_CMM_ALIGN_SIZE 128

inline auto AX_CMM_SESSION_NAME = "axclrt engine";

typedef struct
{
    uint32_t nIndex;
    uint64_t nSize;
    void* pBuf;
} AXCL_IO_BUF_T;

typedef struct
{
    uint32_t nInputSize;
    uint32_t nOutputSize;
    AXCL_IO_BUF_T* pInputs;
    AXCL_IO_BUF_T* pOutputs;
} AXCL_IO_DATA_T;

namespace middleware
{
    inline void free_io_index(AXCL_IO_BUF_T* pBuf, size_t index)
    {
        for (size_t i = 0; i < index; ++i)
        {
            axclrtFree(pBuf[i].pBuf);
        }
    }

    inline void free_io(AXCL_IO_DATA_T* io_data)
    {
        for (size_t j = 0; j < io_data->nInputSize; ++j)
        {
            axclrtFree(io_data->pInputs[j].pBuf);
        }
        for (size_t j = 0; j < io_data->nOutputSize; ++j)
        {
            axclrtFree(io_data->pOutputs[j].pBuf);
        }
        delete[] io_data->pInputs;
        delete[] io_data->pOutputs;
    }

    inline int prepare_io(axclrtEngineIOInfo io_info, axclrtEngineIO io, AXCL_IO_DATA_T *io_data)
    {
        memset(io_data, 0, sizeof(AXCL_IO_DATA_T));

        auto inputNum = axclrtEngineGetNumInputs(io_info);
        auto outputNum = axclrtEngineGetNumOutputs(io_info);
        io_data->nInputSize = inputNum;
        io_data->nOutputSize = outputNum;
        io_data->pInputs = new AXCL_IO_BUF_T[inputNum];
        io_data->pOutputs = new AXCL_IO_BUF_T[outputNum];

        // 1. alloc inputs
        for (int32_t i = 0; i < inputNum; i++)
        {
            auto bufSize = axclrtEngineGetInputSizeByIndex(io_info, 0, i);
            void* devPtr = nullptr;
            axclError ret = axclrtMalloc(&devPtr, bufSize, axclrtMemMallocPolicy{});
            if (ret != 0)
            {
                free_io_index(io_data->pInputs, i);
                fprintf(stderr, "Malloc input(index: %d, size: %d) failed! ret=0x%x\n", i, bufSize, ret);
                return -1;
            }

            io_data->pInputs[i].nIndex = i;
            io_data->pInputs[i].nSize = bufSize;
            io_data->pInputs[i].pBuf = devPtr;
            ret = axclrtEngineSetInputBufferByIndex(io, i,  devPtr, bufSize);
            if (ret != 0)
            {
                free_io_index(io_data->pInputs, i);
                fprintf(stderr, "Set input buffer(index: %d, size: %lu) failed! ret=0x%x\n", i, bufSize, ret);
                return -1;
            }
        }

        // 2. alloc outputs
        for (int32_t i = 0; i < outputNum; i++)
        {
            auto bufSize = axclrtEngineGetOutputSizeByIndex(io_info, 0, i);
            void* devPtr = nullptr;
            axclError ret = axclrtMalloc(&devPtr, bufSize, axclrtMemMallocPolicy{});

            if (ret != 0)
            {
                free_io_index(io_data->pOutputs, i);
                fprintf(stderr, "Malloc output(index: %d, size: %d) failed! ret=0x%x\n", i, bufSize, ret);
                return -1;
            }

            io_data->pOutputs[i].nIndex = i;
            io_data->pOutputs[i].nSize = bufSize;
            io_data->pOutputs[i].pBuf = devPtr;
            ret = axclrtEngineSetOutputBufferByIndex(io, i,  devPtr, bufSize);
            if (ret != 0)
            {
                free_io_index(io_data->pOutputs, i);
                fprintf(stderr, "Set output buffer(index: %d, size: %lu) failed! ret=0x%x\n", i, bufSize, ret);
                return -1;
            }
        }

        return 0;
    }

    static int push_input(const std::vector<uint8_t>& data, AXCL_IO_DATA_T* io_data)
    {
        if (io_data->nInputSize != 1)
        {
            fprintf(stderr, "Only support Input size == 1 current now");
            return -1;
        }

        if (data.size() != io_data->pInputs[0].nSize)
        {
            fprintf(stderr, "The input data size is not matched with tensor {index: %d, size: %d}.\n", io_data->pInputs[0].nIndex, io_data->pInputs[0].nSize);
            return -1;
        }

        axclrtMemcpy(io_data->pInputs[0].pBuf, data.data(), data.size(), AXCL_MEMCPY_HOST_TO_DEVICE);

        return 0;
    }
} // namespace middleware
