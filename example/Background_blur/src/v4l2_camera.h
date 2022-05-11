#ifndef _V4L2_CAMERA_H
#define _V4L2_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

int v4l2_camera_open(int videoId, int width, int height);

int v4l2_camera_get_frame(int videoId, unsigned char *data, int size);

int v4l2_camera_close(int videoId);

#ifdef __cplusplus
} //extern "C"
#endif

#endif // _V4L2_CAMERA_H
