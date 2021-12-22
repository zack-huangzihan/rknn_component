# rockchip_rknn_component

This suite is currently designed for RKNN on the RK3588 platform. It is composed of UI based on GDK + and media framework written by GStreamer. For the pre and post processing of different models, RKNN Runtime interface is encapsulated into a plug-in form for better evaluation and demo writing.

Here is the model support:

| model        | The validation set used | Download link                                                |
| ------------ | ----------------------- | ------------------------------------------------------------ |
| mobilenet_v1 | ILSVRC2012_img_val      | [ImageNet (image-net.org)](https://www.image-net.org/challenges/LSVRC/) |

## demo:

### rknn_camera:

This example is a simple example of GTK + GST + RKNN, which can quickly build a RKNN path on debian11.

#### build

rockchip debian11:

```shell
apt install build-essential cmake libgstreamer1.0-dev libgstreamer-plugins-bad1.0-dev libgstreamer-plugins-base1.0-dev libgtk-3-dev libgtk2.0-dev
cmake . && make
```

#### run

RK3588:

```
sudo cp librknn_api/aarch64/librknn* /usr/lib64/
./rknn_camera -d /dev/video8 -m model/mobilenet_v1_rk3588.rknn
```

You will see a camera preview screen appear on your desktop, with a line "Reasoning label" below, where you get the tag number, which is the most likely tag for mobilenet_v1 model inference, and you need to look up the corresponding type in ILSVRC2012_img_val. "score" on behalf of The model's score for reasoning about this set of data. "Time_cost" represents the time it takes to spend a frame reasoning (ms).





