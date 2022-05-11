/****************************************************************************
*
*    Copyright (c) 2018 - 2021 by Rockchip Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Rockchip Corporation. This is proprietary information owned by
*    Rockchip Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Rockchip Corporation.
*
*****************************************************************************/

#ifndef _ROCKX_BACKGROUND_MATTING_API_H
#define _ROCKX_BACKGROUND_MATTING_API_H

#include "rockx_type.h"

#ifdef __cplusplus
extern "C" {
#endif

extern rockx_module_t ROCKX_MODULE_BG_MATTING;                   ///< Background Matting

/**
 * @brief Background Matting Output Array
 */
typedef struct rockx_bg_matting_array_t {
	uint8_t *matting_img;	/// [out] output matting_img, size = width * height * 4(RGBA format)
	size_t width;			/// [in] matting_img width
	size_t height;			/// [in] matting_img height
} rockx_bg_matting_array_t;

rockx_ret_t rockx_bg_matting(rockx_handle_t handle, rockx_image_t *in_img, rockx_bg_matting_array_t *bg_matting_array,
		rockx_async_callback *callback);

#ifdef __cplusplus
} //extern "C"
#endif

#endif // _ROCKX_BACKGROUND_MATTING_API_H
