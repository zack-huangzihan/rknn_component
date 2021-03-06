/*
 * Vedio.cpp
 *
 *  Created on: 2014-6-1
 *      Author: sfe1012
 */
#include "v4l_achieve.h"

#include <time.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "log.h"

static int get_fourcc_str(unsigned int fmt, unsigned char *str) {
    memcpy(str, &fmt, 4);
    return 0;
}

V4LAchieve::V4LAchieve() : miCameraIndex(-1), miOpenedCameraFd(-1),
                           miBufferCount(-1), mpstV4LBuffers(NULL) {}

V4LAchieve::V4LAchieve(const int &iCameraIndex, const int &iWidth, const int &iHeigth)
        : miCameraIndex(-1), miOpenedCameraFd(-1),
          miBufferCount(-1), mpstV4LBuffers(NULL) {
    this->miCameraIndex = iCameraIndex;
    m_pYUYV422 = new unsigned char[iWidth * iHeigth * 2];
    m_iYUYV422Len = 0;
    m_iWidth = iWidth;
    m_iHeight = iHeigth;
}

V4LAchieve::~V4LAchieve() {
    delete[] m_pYUYV422;
}

void V4LAchieve::Init(const int &iCameraIndex, const int &iWidth, const int &iHeigth) {
    this->miCameraIndex = iCameraIndex;
    m_pYUYV422 = new unsigned char[iWidth * iHeigth * 2];
    m_iYUYV422Len = 0;
    m_iWidth = iWidth;
    m_iHeight = iHeigth;
}

bool V4LAchieve::OpenCamera() {
    const int iMAXPATH = 50;
    char *chPCameraDevicename = new char[iMAXPATH];
    std::memset(chPCameraDevicename, 0, iMAXPATH);
    std::sprintf(chPCameraDevicename, "/dev/video%d", miCameraIndex);
    LOGI("Open Camera Device : %s", chPCameraDevicename);
    //打开video设备文件
    miOpenedCameraFd = open(chPCameraDevicename, O_RDWR /* required */ | O_NONBLOCK, 0);
    if (miOpenedCameraFd < 0) {
        LOGI(" Open Camera Device : %s Failed %d", chPCameraDevicename, errno);
        return false;
    }
    delete[] chPCameraDevicename;
    chPCameraDevicename = NULL;

    //查询视频设备参数
    if (!GetCameraParameters()) {
        LOGE("GetCameraParameters Fail");
        return false;
    }
    //查询视频采集参数
    if (!GetCameraVideoFormat()) {
        LOGE("GetCameraVideoFormat Fail");
        return false;
    }
    //设置视频采集参数
    if (!SetCameraVideoFormat()) {
        LOGE("SetCameraVideoFormat Fail");
        return false;
    }
    //开始视频的采集
    if (!StartCameraCapture()) {
        LOGI("StartCameraCapture Fail");
        return false;
    }
    return true;
}

//Get camera parameters
bool V4LAchieve::GetCameraParameters() {
    if (miOpenedCameraFd < 0) {
        LOGE("Invalid Camera File Descriptor");
        return false;
    }
    struct v4l2_capability stV4l2Capability;
    std::memset(&stV4l2Capability, 0, sizeof(struct v4l2_capability));
    //查询视频设备参数
    if (ioctl(miOpenedCameraFd, VIDIOC_QUERYCAP, &stV4l2Capability) < 0) {
        LOGE("Get Camera Parameters Failed!");
        return false;
    }
    LOGI("Camera Capability as :");
    LOGI("Camera Bus info: %s", stV4l2Capability.bus_info);
    LOGI("Camera Name: %s", stV4l2Capability.card);
    LOGI("Camera Kernel Version: %d", stV4l2Capability.version);
    LOGI("Camera Driver Info: %s", stV4l2Capability.driver);
    LOGI("Camera Capabilities: 0x%x", stV4l2Capability.capabilities);
    if((stV4l2Capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE){
        LOGI("Camera device support capture");
    }
    if((stV4l2Capability.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING){
        LOGI("Camera device support streaming");
    }
    return true;
}

//get camera capture property
bool V4LAchieve::GetCameraVideoFormat() {
    if (miOpenedCameraFd < 0) {
        LOGE("Invalid Camera File Descriptor");
        return false;
    }
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_fmtdesc fmt_1;
    struct v4l2_frmsizeenum frmsize;
    fmt_1.index = 0;
    fmt_1.type = type;
    while (ioctl(miOpenedCameraFd, VIDIOC_ENUM_FMT, &fmt_1) >= 0) {
        frmsize.pixel_format = fmt_1.pixelformat;
        frmsize.index = 0;
        while (ioctl(miOpenedCameraFd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
            LOGI("camera Capture format: %s, resolution: %dx%d", fmt_1.description,
                     frmsize.discrete.width,
                     frmsize.discrete.height);
            frmsize.index++;
        }
        fmt_1.index++;
    }
    return true;
}

// set camera capture property
bool V4LAchieve::SetCameraVideoFormat() {
    printf("huangzihan log 1 \n");
    if (miOpenedCameraFd < 0) {
        LOGE("Invalid Camera File Descriptor");
        return false;
    }
    printf("huangzihan log 2 \n");
    struct v4l2_format stV4l2Format;
    std::memset(&stV4l2Format, 0, sizeof(struct v4l2_format));
    //stV4l2Format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    stV4l2Format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("width = %d height = %d\n",m_iWidth,m_iHeight);
    stV4l2Format.fmt.pix.width = m_iWidth;
    stV4l2Format.fmt.pix.height = m_iHeight;
    stV4l2Format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    //stV4l2Format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    //stV4l2Format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    stV4l2Format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    //设置视频帧格式,包括宽度高度,帧的点阵格式(YUYV/MJPEG)
    printf("huangzihan log 3 \n");
    if (ioctl(miOpenedCameraFd, VIDIOC_S_FMT, &stV4l2Format) < 0) {
        LOGE("set camera Capture format error! ");
        return false;
    }
    LOGI("set camera capture format is ok !");

    struct v4l2_format stV4l2Format1;
    memcpy(&stV4l2Format1, &stV4l2Format, sizeof(v4l2_format));
	if (ioctl(miOpenedCameraFd, VIDIOC_G_FMT, &stV4l2Format1) == -1) {
		LOGE("unable to get format\n");
		return false;
	}
    unsigned char set_pixelformat_str[5] = {0};
    get_fourcc_str(stV4l2Format.fmt.pix.pixelformat, set_pixelformat_str);
    unsigned char get_pixelformat_str[5] = {0};
    get_fourcc_str(stV4l2Format1.fmt.pix.pixelformat, get_pixelformat_str);
    LOGI("Camera pixelformat set=%s get=%s", set_pixelformat_str, get_pixelformat_str);

    struct v4l2_requestbuffers stV4l2RequestBuffers;
    std::memset(&stV4l2RequestBuffers, 0, sizeof(struct v4l2_requestbuffers));
    stV4l2RequestBuffers.count = 4;
    stV4l2RequestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    stV4l2RequestBuffers.memory = V4L2_MEMORY_MMAP;

    //申请缓冲，count是申请的数量,一般不少于3个
    if (ioctl(miOpenedCameraFd, VIDIOC_REQBUFS, &stV4l2RequestBuffers) < 0) {
        LOGE("申请缓存失败! ");
        return false;
    }
    if (stV4l2RequestBuffers.count < 2) {
        LOGE("Insufficient buffer memory");
    }
    LOGI("The Camera Apply Cache Success");;
    LOGI("Cache Num = %d", stV4l2RequestBuffers.count);;
    LOGI("Cache  Size = %d", stV4l2RequestBuffers.memory);
    LOGI("Cache  Type = %d", stV4l2RequestBuffers.type);
    //保存缓存的帧数
    miBufferCount = stV4l2RequestBuffers.count;
    //开始分配缓存
    //内存中建立对应空间
    mpstV4LBuffers = (struct st_V4LBuffer *) calloc(stV4l2RequestBuffers.count,
                                                    sizeof(struct st_V4LBuffer));
    unsigned int iReqBuffersNum = 0;
    for (iReqBuffersNum = 0; iReqBuffersNum < stV4l2RequestBuffers.count; ++iReqBuffersNum) {
        //驱动中的一帧
        struct v4l2_buffer stV4l2Buffer;
        std::memset(&stV4l2Buffer, 0, sizeof(struct v4l2_buffer));

        stV4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        stV4l2Buffer.memory = V4L2_MEMORY_MMAP;
        stV4l2Buffer.index = iReqBuffersNum;
        //查询帧缓冲区在内核空间中的长度和偏移量
        if (-1 == ioctl(miOpenedCameraFd, VIDIOC_QUERYBUF, &stV4l2Buffer)) {
            LOGE("VIDIOC_QUERYBUF error");
            break;
        }
        //将帧缓冲区的地址映射到用户内存空间中
        mpstV4LBuffers[iReqBuffersNum].iLength = stV4l2Buffer.length;
        mpstV4LBuffers[iReqBuffersNum].pStart =
                //通过mmap建立映射关系
                mmap(NULL /* start anywhere */,
                     stV4l2Buffer.length,
                     PROT_READ | PROT_WRITE /* required */,
                     MAP_SHARED /* recommended */,
                     miOpenedCameraFd, stV4l2Buffer.m.offset);
        if (MAP_FAILED == mpstV4LBuffers[iReqBuffersNum].pStart) {
            LOGE("mmap failed\n");
            break;
        }
        ::memset(mpstV4LBuffers[iReqBuffersNum].pStart, 0, mpstV4LBuffers[iReqBuffersNum].iLength);
        LOGI("Buffer %d start=%p size=%d", iReqBuffersNum, mpstV4LBuffers[iReqBuffersNum].pStart, mpstV4LBuffers[iReqBuffersNum].iLength);
    }
    if (iReqBuffersNum < stV4l2RequestBuffers.count) {
        LOGE(" error in request v4l2 buffer ");
        return false;
    }
    LOGI(" request v4l2 buffer finsihed");
    unsigned int index = 0;
    for (index = 0; index < iReqBuffersNum; ++index) {
        struct v4l2_buffer buf;
        std::memset(&buf, 0, sizeof(struct v4l2_buffer));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = index;

        //将申请到的帧缓冲全部放入视频采集输出列队
        if (-1 == ioctl(miOpenedCameraFd, VIDIOC_QBUF, &buf)) {
            LOGE("VIDIOC_QBUF failed with index= %d", index);
            break;
        }
    }
    if (index < iReqBuffersNum) {
        LOGE(" error in  v4l2 buffer queue ");
        return false;
    }
    return true;
}

bool V4LAchieve::StartCameraCapture() {
    if (miOpenedCameraFd < 0) return false;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    //开始捕捉图像数据
    if (-1 == ioctl(miOpenedCameraFd, VIDIOC_STREAMON, &type)) {
        LOGI(" Start VIDIOC_STREAMON failed \n");
        return false;
    }
    LOGI("VIDIOC_STREAMON Start collecting capture graph ok");
    return true;
}

//双摄像头将不会加入 IO 监控处理
bool V4LAchieve::CameraVideoGetLoop() {
    fd_set fds;
    struct timeval tv;
    //将指定的文件描述符集清空
    FD_ZERO (&fds);
    //在文件描述符集合中增加一个新的文件描述符
    FD_SET (miOpenedCameraFd, &fds);
    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    //判断是否可读（即摄像头是否准备好），tv是定时
    int r = ::select(miOpenedCameraFd + 1, &fds, NULL, NULL, &tv);
    if (-1 == r) {
        if (EINTR == errno)
            return 0;
        LOGE("select err");
        return false;
    }
    // start to dequeue image systemCameraFrame
    struct v4l2_buffer buf;
    std::memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    //从视频采集输出队列中取出已含有采集数据的帧缓冲区
    if (ioctl(miOpenedCameraFd, VIDIOC_DQBUF, &buf) < 0) {
        LOGE("获取数据失败! ");
        return false;
    }
    assert (buf.index < (unsigned long) miBufferCount);

    m_iYUYV422Len = mpstV4LBuffers[buf.index].iLength;
    //从帧缓冲区中获取一帧数据
    ::memcpy(m_pYUYV422, mpstV4LBuffers[buf.index].pStart, mpstV4LBuffers[buf.index].iLength);
    //将帧缓冲区重新入列
    ioctl(miOpenedCameraFd, VIDIOC_QBUF, &buf);
    return true;
}

bool V4LAchieve::StopCameraCapture() {
    if (miOpenedCameraFd < 0) return false;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    //停止捕捉图像数据
    if (-1 == ioctl(miOpenedCameraFd, VIDIOC_STREAMOFF, &type)) {
        LOGI(" Stop VIDIOC_STREAMOFF failed \n");
        return false;
    }
    LOGI("VIDIOC_STREAMOFF Stop collecting capture graph ok");
    return true;
}

bool V4LAchieve::CloseCamera() {
    //停止视频的采集
    StopCameraCapture();
    //释放申请的视频帧缓冲区
    for (int index = 0; index < (mpstV4LBuffers ? miBufferCount : 0); index++)
        ::munmap(mpstV4LBuffers[index].pStart, mpstV4LBuffers[index].iLength);
    //释放用户帧缓冲区内存
    if (mpstV4LBuffers) {
        free(mpstV4LBuffers);
        mpstV4LBuffers = NULL;
    }
    //关闭设备文件
    if (miOpenedCameraFd != -1) {
        ::close(miOpenedCameraFd);
        miOpenedCameraFd = -1;
    }
    return true;
}

void V4LAchieve::DeInit() {
    delete[] m_pYUYV422;
}
