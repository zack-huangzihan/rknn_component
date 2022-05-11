#include "v4l2_camera.h"
#include "v4l_achieve.h"
#include "log.h"

typedef struct {
    int videoid;
    int width;
    int height;
    int refCount;
    V4LAchieve *v4LAchieve;
} _v4l2_camera_context_t; 

const int MAX_CAMERA_NUM = 32;

_v4l2_camera_context_t camera_context[MAX_CAMERA_NUM];

int v4l2_camera_open(int videoId, int width, int height) {
    if (videoId < 0) {
        LOGE("Error videoId(%d)", videoId);
        return -1;
    }
    if (videoId >= MAX_CAMERA_NUM) {
        LOGE("Error videoId(%d) >= MAX_CAMERA_NUM(%d)", videoId, MAX_CAMERA_NUM);
        return -1;
    }

    _v4l2_camera_context_t *context = &camera_context[videoId];

    if (context->v4LAchieve != NULL) {
        LOGE("Error videoId(%d) already has been opened", videoId);
        return -1;
    }

    LOGI("prepareUsbCamera");
    V4LAchieve *oV4LAchieve = new V4LAchieve(videoId, width, height);
    if (!oV4LAchieve->OpenCamera()) {
        LOGE("V4LAchieve OpenCamera ERROR");
        return -1;
    }
    context->v4LAchieve = oV4LAchieve;
    context->width = width;
    context->height = height;
    LOGD("videoId=%d context=%p v4l=%p", videoId, context, context->v4LAchieve);
    return 0;
}

int v4l2_camera_get_frame(int videoId, unsigned char *data, int size) {
    if (videoId < 0) {
        LOGE("Error videoId(%d)", videoId);
        return -1;
    }
    if (videoId >= MAX_CAMERA_NUM) {
        LOGE("Error videoId(%d) >= MAX_CAMERA_NUM(%d)", videoId, MAX_CAMERA_NUM);
        return -1;
    }

    _v4l2_camera_context_t *context = &camera_context[videoId];
    if (context->v4LAchieve == NULL) {
        LOGE("Error Camera videoId %d has not been opened", videoId);
        return -1;
    }

    LOGD("videoId=%d context=%p v4l=%p", videoId, context, context->v4LAchieve);

    V4LAchieve *oV4LAchieve = context->v4LAchieve;

    oV4LAchieve->CameraVideoGetLoop();
    unsigned char *frame_data = oV4LAchieve->GetpYUYV422();
    if (frame_data == NULL) {
        LOGE("Error frame_data is NULL!");
        return -1;
    }
    memcpy(data, frame_data, context->width*context->height*2);
    return 0;
}

int v4l2_camera_close(int videoId) {
    if (videoId < 0) {
        LOGE("Error videoId(%d)", videoId);
        return -1;
    }
    if (videoId >= MAX_CAMERA_NUM) {
        LOGE("Error videoId(%d) >= MAX_CAMERA_NUM(%d)", videoId, MAX_CAMERA_NUM);
        return -1;
    }

    _v4l2_camera_context_t *context = &camera_context[videoId];
    if (context->v4LAchieve == NULL) {
        LOGE("Error Camera videoId %d has not been opened", videoId);
        return -1;
    }

    LOGD("videoId=%d context=%p v4l=%p", videoId, context, context->v4LAchieve);

    V4LAchieve *oV4LAchieve = context->v4LAchieve;
    oV4LAchieve->CloseCamera();
    delete(oV4LAchieve);

    context->v4LAchieve = NULL;
    return 0;
}