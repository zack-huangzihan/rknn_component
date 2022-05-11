部署：
push background_matting.data    /usr/lib/
push background_matting.data    /usr/lib/
push rockx-rk3588-Linux/lib64/* /usr/lib/
push key.lic                    /usr/lib/
如果key过期，或者设备不对，需要重新生成key，请看background_blur/auth/README.md

push background_blur            /usr/bin/

运行：

export ROCKX_DATA_PATH=/usr/lib/
export BACKGROUND_BLUR_VIDEO=0    (这里的你的usb摄像头生成的video节点，目前只支持usb摄像头)
/usr/bin/background_blur  