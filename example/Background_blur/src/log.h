#ifndef _LOG_H
#define _LOG_H

#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define DEBUG_ON 1

#ifndef LOG_TAG
#define LOG_TAG "v4l2cam"
#endif

#ifdef ANDROID

#include <android/log.h>
#define LOG(level, ...) ((void)__android_log_print(level, LOG_TAG, __VA_ARGS__))

#else

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static size_t getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#define LOG(level, _str, ...) \
        do {        \
            fprintf(stdout, "%ld " LOG_TAG " %s(%d): " _str "\n", getCurrentTime(), __FUNCTION__, __LINE__, ## __VA_ARGS__);    \
        } while(0)

#endif // #ifdef ANDROID


#if (DEBUG_ON==1)

#define LOGD(_str, ...) LOG(ANDROID_LOG_DEBUG, _str , ## __VA_ARGS__)
#define LOGI(_str, ...) LOG(ANDROID_LOG_INFO, _str , ## __VA_ARGS__)

#else

#define LOGD(_str, ...)
#define LOGI(_str, ...)

#endif // #if (DEBUG_ON==1)

#define LOGW(_str, ...) LOG(ANDROID_LOG_WARN, _str , ## __VA_ARGS__)
#define LOGE(_str, ...) LOG(ANDROID_LOG_ERROR, _str , ## __VA_ARGS__)


#endif //_LOG_H
