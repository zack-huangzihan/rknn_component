/*
 * Copyright 2021 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/time.h>
#include "rknn_api.h"

using namespace std;

class rknn_mobilenet
{
  private:
    int max_top_num;
    int model_in_width;
    int model_in_height;
    int model_in_channels;
    int model_len;
    int ret;
    rknn_context ctx;
    rknn_input_output_num io_num;
    unsigned char *model;
    unsigned char *load_model(const char *filename, int *model_size);
    void printRKNNTensor(rknn_tensor_attr *attr);
    int rknn_GetTop (float *pfProb, float *pfMaxProb, uint32_t *pMaxClass, uint32_t outputCount, uint32_t topNum);
    long getCurrentTimeMsec();

  public:
    int run_results;
    float run_results_score;
    long time_cost;
    rknn_mobilenet(int MODEL_IN_WIDTH, int MODEL_IN_HEIGHT, int MODEL_IN_CHANNELS);
    int init(char *model_path);
    int run(void* buf);
    void release();
    ~rknn_mobilenet();
};