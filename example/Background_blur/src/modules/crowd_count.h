/****************************************************************************
 *
 *     Copyright (c) 2018 - 2021 by Rockchip Corp.  All rights reserved.
 *
 *    The material in this file is confidential and contains trade secrets
 *    of Rockchip Corporation. This is proprietary information owned by
 *    Rockchip Corporation. No part of this work may be disclosed,
 *    reproduced, copied, transmitted, or used in any way for any purpose,
 *    without the express written permission of Rockchip Corporation.
 *
 *****************************************************************************/

#ifndef _ROCKX_CROWD_COUNT_API_H
#define _ROCKX_CROWD_COUNT_API_H

#include "../rockx_type.h"

#ifdef __cplusplus
extern "C" {
#endif

extern rockx_module_t ROCKX_MODULE_CROWD_COUNT;
extern rockx_module_t ROCKX_MODULE_CROWD_COUNT_FASTER;  ///< Crowd Count

/**
 * @brief CrowdCount Array
 */
typedef struct rockx_crowdcount_t
{
    unsigned char* pred;  /// predict density map
    int width;
    int height;
    int predcount;  /// predict count
} rockx_crowdcount_t;

/**
 * Get People Number of image
 * @param handle [in] Handle of a created ROCKX_MODULE_CROWD_COUNT module(created by @ref rockx_create)
 * @param in_img [in] Input image
 * @param crowdcount_array [in] [out] Array of crowd count
 * @param callback [in] Async callback function pointer
 * @return @ref rockx_ret_t
 */
rockx_ret_t rockx_crowd_count(rockx_handle_t handle, rockx_image_t* in_img, rockx_crowdcount_t* crowdcnt_array);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _ROCKX_CROWD_COUNT_API_H
