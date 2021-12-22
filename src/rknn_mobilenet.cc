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

#include "rknn_mobilenet.h"
#include "rknn_api.h"

rknn_mobilenet::rknn_mobilenet(int MODEL_IN_WIDTH, int MODEL_IN_HEIGHT, int MODEL_IN_CHANNELS) {
  max_top_num = 20;
  model_len = 0;
  ret = 0;
  model_in_width = MODEL_IN_WIDTH;
  model_in_height = MODEL_IN_HEIGHT;
  model_in_channels = MODEL_IN_CHANNELS;
  run_results = -1;
}
 
rknn_mobilenet::~rknn_mobilenet() {
    if(ctx >= 0) {
        rknn_destroy(ctx);
    }
    if(model) {
        free(model);
    }
  printf("rknn_mobilenet release.\n");
}

long rknn_mobilenet::getCurrentTimeMsec() {
  long msec = 0;
  char str[20] = {0};
  struct timeval stuCurrentTime;
  gettimeofday(&stuCurrentTime, NULL);\
  sprintf(str, "%ld%03ld", stuCurrentTime.tv_sec, (stuCurrentTime.tv_usec)/1000);
  for(size_t i=0; i<strlen(str); i++) {
    msec =msec*10 + (str[i]-'0');
  }
  return msec;
}

unsigned char *rknn_mobilenet::load_model(const char *filename, int *model_size) {
  FILE *fp = fopen(filename, "rb");
  if(fp == nullptr) {
      printf("fopen %s fail!\n", filename);
      return NULL;
  }
  fseek(fp, 0, SEEK_END);
  int model_len = ftell(fp);
  unsigned char *model = (unsigned char*)malloc(model_len);
  fseek(fp, 0, SEEK_SET);
  if(model_len != fread(model, 1, model_len, fp)) {
      printf("fread %s fail!\n", filename);
      free(model);
      return NULL;
  }
  *model_size = model_len;
  if(fp) {
      fclose(fp);
  }
  return model;
}

void rknn_mobilenet::printRKNNTensor(rknn_tensor_attr *attr) {
  printf("index=%d name=%s n_dims=%d dims=[%d %d %d %d] n_elems=%d size=%d fmt=%d type=%d qnt_type=%d fl=%d zp=%d scale=%f\n", 
            attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3], 
            attr->n_elems, attr->size, 0, attr->type, attr->qnt_type, attr->fl, attr->zp, attr->scale);
}

int rknn_mobilenet::rknn_GetTop (float *pfProb, float *pfMaxProb, uint32_t *pMaxClass, uint32_t outputCount, uint32_t topNum) {
    uint32_t i, j;
    if (topNum > max_top_num) return 0;
    memset(pfMaxProb, 0, sizeof(float) * topNum);
    memset(pMaxClass, 0xff, sizeof(float) * topNum);
    for (j = 0; j < topNum; j++) {
        for (i=0; i<outputCount; i++){
            if ((i == *(pMaxClass+0)) || (i == *(pMaxClass+1)) || (i == *(pMaxClass+2)) ||
                (i == *(pMaxClass+3)) || (i == *(pMaxClass+4)))
            {
                continue;
            }

            if (pfProb[i] > *(pfMaxProb+j)) {
                *(pfMaxProb+j) = pfProb[i];
                *(pMaxClass+j) = i;
            }
        }
    }
    return 1;
}

int rknn_mobilenet::init(char *model_path) {
  model = rknn_mobilenet::load_model(model_path, &model_len);
  ret = rknn_init(&ctx, model, model_len, 0, NULL);
  if(ret < 0) {
    printf("rknn_init fail! ret=%d\n", ret);
    return -1;
  }
  ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
  if (ret != RKNN_SUCC) {
    printf("rknn_query fail! ret=%d\n", ret);
    return -1;
  }
  printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);
  printf("input tensors:\n");
  rknn_tensor_attr input_attrs[io_num.n_input];
  memset(input_attrs, 0, sizeof(input_attrs));
  for (int i = 0; i < io_num.n_input; i++) {
    input_attrs[i].index = i;
    ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC) {
      printf("rknn_query fail! ret=%d\n", ret);
      return -1;
    }
      printRKNNTensor(&(input_attrs[i]));
  }

  printf("output tensors:\n");
  rknn_tensor_attr output_attrs[io_num.n_output];
  memset(output_attrs, 0, sizeof(output_attrs));
  for (int i = 0; i < io_num.n_output; i++) {
    output_attrs[i].index = i;
    ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC) {
      printf("rknn_query fail! ret=%d\n", ret);
      return -1;
    }
    printRKNNTensor(&(output_attrs[i]));
  }
  return 0;
}

int rknn_mobilenet::run(void* input_buf) {
  rknn_input inputs[1];
  memset(inputs, 0, sizeof(inputs));
  inputs[0].index = 0;
  inputs[0].type = RKNN_TENSOR_UINT8;
  inputs[0].size = model_in_width*model_in_height*model_in_channels;
  inputs[0].fmt = RKNN_TENSOR_NHWC;
  inputs[0].buf = input_buf;

  ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
  if(ret < 0) {
      printf("rknn_input_set fail! ret=%d\n", ret);
      return -1;
  }
  // Run
  //printf("rknn_run\n");
  long time1 = getCurrentTimeMsec();
  ret = rknn_run(ctx, nullptr);
  if(ret < 0) {
    printf("rknn_run fail! ret=%d\n", ret);
    return -1;
  }
  time_cost = getCurrentTimeMsec() - time1;
  // Get Output
  rknn_output outputs[1];
  memset(outputs, 0, sizeof(outputs));
  outputs[0].want_float = 1;
  ret = rknn_outputs_get(ctx, 1, outputs, NULL);
  if(ret < 0) {
    printf("rknn_outputs_get fail! ret=%d\n", ret);
    return -1;
  }
  // Post Process
  uint32_t MaxClass[5];
  float fMaxProb[5];
  for (int i = 0; i < io_num.n_output; i++) {
    float *buffer = (float *)outputs[i].buf;
    uint32_t sz = outputs[i].size/4;

    rknn_GetTop(buffer, fMaxProb, MaxClass, sz, 5);

    // printf(" --- Top5 ---\n");
    // for(int i=0; i<5; i++) {
    //   printf("%3d: %8.6f\n", MaxClass[i], fMaxProb[i]);
    // }
  }
  run_results = MaxClass[0];
  run_results_score = fMaxProb[0];

    // Release rknn_outputs
    rknn_outputs_release(ctx, 1, outputs);
  return 0;
}

void rknn_mobilenet::release() {
    // Release
    if(ctx >= 0) {
        rknn_destroy(ctx);
    }
    if(model) {
        free(model);
    }
}
