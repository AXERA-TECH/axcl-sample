/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <cstdio>
#include <cstring>
#include <numeric>

#include <opencv2/opencv.hpp>

#include "common.hpp"
#include "detection.hpp"
#include "io.hpp"

#include "cmdline.h"
#include "args.hpp"
#include "file.hpp"
#include "timer.hpp"

#include <axcl/native/ax_sys_api.h>
#include <axcl/axcl.h>
#include <axcl/rt/axcl_rt_engine.h>

const int DEFAULT_IMG_H = 640;
const int DEFAULT_IMG_W = 640;

const char* CLASS_NAMES[] = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"};
const float ANCHORS[18] = {10, 13, 16, 30, 33, 23, 30, 61, 62, 45, 59, 119, 116, 90, 156, 198, 373, 326};

const int DEFAULT_LOOP_COUNT = 1;

const float PROB_THRESHOLD = 0.45f;
const float NMS_THRESHOLD = 0.45f;
namespace ax
{
    bool read_model(const std::string& model, std::vector<char>& model_buffer)
    {
        std::fstream fs(model, std::ios::in | std::ios::binary);

        if (!fs.is_open()) {
            return false;
        }

        fs.seekg(std::ios::end);
        auto fs_end = fs.tellg();
        fs.seekg(std::ios::beg);
        auto fs_beg = fs.tellg();

        auto file_size = static_cast<size_t>(fs_end - fs_beg);
        auto vector_size = model_buffer.size();

        model_buffer.reserve(vector_size + file_size);
        model_buffer.insert(model_buffer.end(), std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());

        fs.close();

        return true;
    }

    void post_process(AXCL_IO_DATA_T* io_data, INPUT_OUTPUT_ALLOC_STRATEGY strategy, const cv::Mat& mat, int input_w, int input_h, const std::vector<float>& time_costs)
    {
        std::vector<detection::Object> proposals;
        std::vector<detection::Object> objects;
        float prob_threshold_u_sigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
        timer timer_postprocess;

        for (uint32_t i = 0; i < io_data->nOutputSize; ++i)
        {
            auto& output = io_data->pOutputs[i];
            if (AX_ENGINE_ABST_CACHED == strategy.second)
            {
                axclrtMemFlush(output.pBuf, output.nSize);
            }

            uchar *buf = new uchar[output.nSize];
            axclrtMemcpy(buf, output.nSize, output.pBuf, output.nSize, AXCL_MEMCPY_DEVICE_TO_HOST);

            // auto& info = io_info->pOutputs[i];
            int32_t stride = (1 << i) * 8;
            detection::generate_proposals_255(stride, (float*)buf, PROB_THRESHOLD, proposals, input_w, input_h, ANCHORS, prob_threshold_u_sigmoid);

            delete[] buf;
        }

        detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, input_h, input_w, mat.rows, mat.cols);
        fprintf(stdout, "post process cost time:%.2f ms \n", timer_postprocess.cost());
        fprintf(stdout, "--------------------------------------\n");
        auto total_time = std::accumulate(time_costs.begin(), time_costs.end(), 0.f);
        auto min_max_time = std::minmax_element(time_costs.begin(), time_costs.end());
        fprintf(stdout,
                "Repeat %d times, avg time %.2f ms, max_time %.2f ms, min_time %.2f ms\n",
                (int)time_costs.size(),
                total_time / (float)time_costs.size(),
                *min_max_time.second,
                *min_max_time.first);
        fprintf(stdout, "--------------------------------------\n");
        fprintf(stdout, "detection num: %zu\n", objects.size());

        detection::draw_objects(mat, objects, CLASS_NAMES, "yolov5s_out");
    }

    bool run_model(const std::string& model, const std::vector<uint8_t>& data, const int& repeat, cv::Mat& mat, int input_h, int input_w)
    {
        // 1. init engine
        axclError ret = axclInit(nullptr);
        if (0 != ret)
        {
            fprintf(stderr, "Init axcl failed. ret=0x%x\n", ret);
            return ret;
        }

        // 2. load model
        std::vector<char> model_buffer;
        if (!read_model(model, model_buffer))
        {
            fprintf(stderr, "Read model(%s) file failed. ret=0x%x\n", model.c_str(), ret);
            return false;
        }

        uint64_t handle = 0;
        ret = axclrtEngineLoadFromMem(model_buffer.data(), model_buffer.size(), &handle);
        if (ret != 0)
        {
            fprintf(stderr, "Read model(%s) file failed. ret=0x%x\n", model.c_str(), ret);
            return false;
        }

        // 3. create context
        uint64_t context = 0;
        ret = axclrtEngineCreateContext(handle, &context);
        if (ret != 0)
        {
            fprintf(stderr, "Create context failed. ret=0x%x\n", ret);
            return false;
        }

        // 3. get io info
        axclrtEngineIOInfo io_info;
        ret = axclrtEngineGetIOInfo(handle, &io_info);
        if (ret != 0)
        {
            fprintf(stderr, "Get io info failed. ret=0x%x\n", ret);
            return false;
        }

        // 4. create io
        axclrtEngineIO io;
        ret = axclrtEngineCreateIO(io_info, &io);
        if (ret != 0)
        {
            axclrtEngineUnload(handle);
            fprintf(stderr, "Create io failed. ret=0x%x\n", ret);
            return false;
        }

        // 6. alloc io
        AXCL_IO_DATA_T io_data;
        auto malloc_strategy = std::make_pair(AX_ENGINE_ABST_DEFAULT, AX_ENGINE_ABST_CACHED);
        ret = middleware::prepare_io(handle, context, io_info, io, &io_data, malloc_strategy);
        if (ret != 0)
        {
            middleware::free_io(&io_data);
            axclrtEngineDestroyIO(io);
            axclrtEngineUnload(handle);

            fprintf(stderr, "prepare_io failed.\n");
            return false;
        }

        // 7. insert input
        ret = middleware::push_input(data, &io_data);
        if (ret != 0)
        {
            middleware::free_io(&io_data);
            axclrtEngineDestroyIO(io);
            axclrtEngineUnload(handle);

            fprintf(stderr, "prepare_io failed.\n");
            return false;
        }
        fprintf(stdout, "Engine push input is done. \n");
        fprintf(stdout, "--------------------------------------\n");

        // 8. warn up
        for (int i = 0; i < 5; ++i)
        {
            axclrtEngineExecute(handle, context);
        }

        // 9. run model
        std::vector<float> time_costs(repeat, 0);
        for (int i = 0; i < repeat; ++i)
        {
            timer tick;
            ret = axclrtEngineExecute(handle, context);
            time_costs[i] = tick.cost();
            if (ret != 0)
            {
                middleware::free_io(&io_data);
                axclrtEngineDestroyIO(io);
                axclrtEngineUnload(handle);

                fprintf(stderr, "prepare_io failed.\n");
                return false;
            }
        }

        // 10. get result
        post_process(&io_data, malloc_strategy, mat, input_w, input_h, time_costs);
        fprintf(stdout, "--------------------------------------\n");

        middleware::free_io(&io_data);
        axclrtEngineDestroyIO(io);
        axclrtEngineUnload(handle);
        return true;
    }
} // namespace ax

int main(int argc, char* argv[])
{
    cmdline::parser cmd;
    cmd.add<std::string>("model", 'm', "joint file(a.k.a. joint model)", true, "");
    cmd.add<std::string>("image", 'i', "image file", true, "");
    cmd.add<std::string>("size", 'g', "input_h, input_w", false, std::to_string(DEFAULT_IMG_H) + "," + std::to_string(DEFAULT_IMG_W));

    cmd.add<int>("repeat", 'r', "repeat count", false, DEFAULT_LOOP_COUNT);
    cmd.parse_check(argc, argv);

    // 0. get app args, can be removed from user's app
    auto model_file = cmd.get<std::string>("model");
    auto image_file = cmd.get<std::string>("image");

    auto model_file_flag = utilities::file_exist(model_file);
    auto image_file_flag = utilities::file_exist(image_file);

    if (!model_file_flag | !image_file_flag)
    {
        auto show_error = [](const std::string& kind, const std::string& value) {
            fprintf(stderr, "Input file %s(%s) is not exist, please check it.\n", kind.c_str(), value.c_str());
        };

        if (!model_file_flag) { show_error("model", model_file); }
        if (!image_file_flag) { show_error("image", image_file); }

        return -1;
    }

    auto input_size_string = cmd.get<std::string>("size");

    std::array<int, 2> input_size = {DEFAULT_IMG_H, DEFAULT_IMG_W};

    auto input_size_flag = utilities::parse_string(input_size_string, input_size);

    if (!input_size_flag)
    {
        auto show_error = [](const std::string& kind, const std::string& value) {
            fprintf(stderr, "Input %s(%s) is not allowed, please check it.\n", kind.c_str(), value.c_str());
        };

        show_error("size", input_size_string);

        return -1;
    }

    auto repeat = cmd.get<int>("repeat");

    // 1. print args
    fprintf(stdout, "--------------------------------------\n");
    fprintf(stdout, "model file : %s\n", model_file.c_str());
    fprintf(stdout, "image file : %s\n", image_file.c_str());
    fprintf(stdout, "img_h, img_w : %d %d\n", input_size[0], input_size[1]);
    fprintf(stdout, "--------------------------------------\n");

    // 2. read image & resize & transpose
    std::vector<uint8_t> image(input_size[0] * input_size[1] * 3, 0);
    cv::Mat mat = cv::imread(image_file);
    if (mat.empty())
    {
        fprintf(stderr, "Read image failed.\n");
        return -1;
    }
    preprocess::get_input_data_letterbox(mat, image, input_size[0], input_size[1]);

    // 3. sys_init
//    AX_SYS_Init();

    // 4. -  engine model  -  can only use AX_ENGINE** inside
    {
        // AX_ENGINE_NPUReset(); // todo ??
        ax::run_model(model_file, image, repeat, mat, input_size[0], input_size[1]);

        // 4.3 engine de init
        axclFinalize();
        // AX_ENGINE_NPUReset();
    }
    // 4. -  engine model  -

//    AX_SYS_Deinit();
    return 0;
}
