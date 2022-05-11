# 说明文档 20220308

郑重声明：本说明只提供教学参考作用，如果用本说明中的例子代码进行生产，后果自负。



## 系统环境搭建：硬件平台：rk3568

debian10-rootfs:

链接：https://pan.baidu.com/s/11_T__18yPFmPY2TFq7xT8g 
提取码：zrt3 

gst硬解码包：

链接：https://pan.baidu.com/s/1k95vIjbxqYsbNhGawRQt4Q 
提取码：fg4u 
（注意，网盘文件有效期只有30天，只能下载一次）



烧入rootfs后

1.先卸载原来的gst:

```
sudo apt autoremove gir1.2-gstreamer-1.0 gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-bad  gstreamer1.0-plugins-good
```

2.覆盖安装：

```
sudo apt install gir1.2-gstreamer-1.0 gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-bad  gstreamer1.0-plugins-good
```

3.解压出gst硬解码包，拷贝到板子上，安装：

```
cd release

dpkg -i ./*.deb
```



## 编译环境搭建：

```
sudo apt install build-essential gstreamer1.0-rtsp libgstrtspserver-1.0-dev  libunwind-dev
```

直接在板子上编译demo,免去搭建交叉编译环境的麻烦。



## 编译demo:

demo包：

链接：https://pan.baidu.com/s/1h4x0yf4uaOv5l8x4uNDEYA 
提取码：aghg  

（注意，网盘文件有效期只有30天，只能下载一次）

拷贝源代码到板子上，直接在板子上编译：

```
cd rtsp_demo/build

cmake .. && make
```

会生成两个可执行文件：rtsp_gst和rtsp_gst_xv

rtsp_gst是解码h26x码流的，是比较完整的例子，不带显示，最后解码出来的buf 指针会暴露出来给客户使用，后面代码讲解会说到，使用方法是：

```
rtsp_gst -u 你的rtsp流
```

rtsp_gst_xv是解码h26x码流的，带显示的，可以用来验证和debug硬件解码的通路的。它的使用方法是：

```
rtsp_gst_xv  -u 你的rtsp流
```



## 概念说明：

我们需要做的硬解码大概需要下面这样的流程：

rtsp流 （gst完成）-> 解封装 （gst完成）-> 分析包头信息（gst完成） -> 解码（gst-mpp完成） ->处理帧数据（用户完成）

我们可以用gst 的命令简单验证下：

```
sudo gst-launch-1.0 rtspsrc location=rtsp://172.16.21.6/live/2 ! rtph265depay ! h265parse ! mppvideodec ! xvimagesink
```

rtph265depay 就是解封装器

h265parse 就是包头信息分析器

mppvideodec 就是mpp硬件解码器

xvimagesink 是显示服务

## 代码讲解：



rtsp_gst:

```
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/time.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>


class userdata
{
	private:

	public:
		void* buf;
};//这里是我们定义的给客户使用的类，用来传递给处理解码后的线程

class get_args
{
  private:
  	void usage_tip(FILE *fp, int argc, char **argv);
  public:
  	char* RtspPath;
    get_args(int argc, char **argv);
    ~get_args();
};

void get_args::usage_tip(FILE *fp, int argc, char **argv) {
	fprintf(fp,
	        "Usage: %s [options]\n"
	        "Version %s\n"
	        "Options:\n"
	        "-u | --url  The url  rtsp The url.\n"
	        "-h | --help        for help \n\n"
	        "\n",
	        argv[0], "V1.0.0");
}

get_args::get_args(int argc, char *argv[]) {
	const char short_options[] = "u:h:";
	const struct option long_options[] = {{"url", required_argument, NULL, 'u'},
                                             {"help", no_argument, NULL, 'h'},
                                             {0, 0}};
    RtspPath = NULL;
	for (;;) {
		int idx;
		int c;
		c = getopt_long(argc, argv, short_options, long_options, &idx);
		if (-1 == c)
			break;
		switch (c) {
		case 0: /* getopt_long() flag */
			break;
		case 'u':
			RtspPath = optarg;
			break;
		case 'h':
			usage_tip(stdout, argc, argv);
			exit(EXIT_SUCCESS);
		default:
			usage_tip(stderr, argc, argv);
			exit(EXIT_FAILURE);
		}
	}
	printf("RtspPath=%s \n", RtspPath);
}

get_args::~get_args() {
	printf("relese get_args\n");
}
// 上面的代码是获取参数的标准化实现，并不重要

static void on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
    GstPad *sinkpad;
    GstElement *decoder = (GstElement *) data;
    /* We can now link this pad with the rtsp-decoder sink pad */
    g_print ("Dynamic pad created, linking source/demuxer\n");
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
}


static void cb_new_rtspsrc_pad(GstElement *element, GstPad*pad, gpointer  data)
{
    gchar *name;
    GstCaps * p_caps;
    gchar * description;
    GstElement *p_rtp_depay;

    name = gst_pad_get_name(pad);
    g_print("A new pad %s was created\n", name);
    p_caps = gst_pad_get_pad_template_caps (pad);

    description = gst_caps_to_string(p_caps);
    printf("%s\n",p_caps,", ",description,"\n");
    g_free(description);

    p_rtp_depay = GST_ELEMENT(data);

    // try to link the pads then ...
    if(!gst_element_link_pads(element, name, p_rtp_depay, "sink"))
    {
        printf("Failed to link elements 3\n");
    }

    g_free(name);
}

static GstFlowReturn your_thread (GstElement *sink, userdata *instance) {
  GstSample *sample;
  GstBuffer *buffer;
  GstMapInfo map;
  g_signal_emit_by_name (sink, "pull-sample", &sample);
  buffer = gst_sample_get_buffer (sample);
  if (buffer) {
    gst_buffer_map (buffer, &map, GST_MAP_READ);
    //在这里去对图片数据进行处理
    //举个例子：user_run(map.data)；map.data就是buffer指针
    gst_buffer_unmap (buffer, &map);
    gst_sample_unref (sample);
    return GST_FLOW_OK;
  }

  return GST_FLOW_ERROR;
}

int main (int argc, char *argv[])
{
	userdata data_instance;
	GMainLoop* loop;
	char *rtsp_path;
	get_args args_instance(argc, argv);
  	if(args_instance.RtspPath == NULL) {
    	printf("rtsp uri is NULL\n");
    	return -1;
  	}else {
    	rtsp_path = args_instance.RtspPath;
  	}
	gst_init (&argc, &argv); //初始化gst
	
	GstStateChangeReturn sret;
	
	//生成我们需要的节点
	GstElement *pipeline, *src, *rtph_depay, *parse, *videodec, *appsink;
	pipeline = gst_pipeline_new ("pipeline");
	src = gst_element_factory_make ("rtspsrc", "rtspsrc");
	g_object_set(G_OBJECT(src),"location", rtsp_path, NULL);
	rtph_depay = gst_element_factory_make ("rtph265depay", "rtph265depay");
	parse = gst_element_factory_make ("h265parse", "h265parse"); //注意，这里我的测试流是h265，所以你要改成你需要的头部信息解析器
	videodec = gst_element_factory_make("mppvideodec", "mppvideodec");
	appsink = gst_element_factory_make ("appsink", "appsink");
  	
  	g_object_set(appsink, "max-buffers", 3, NULL);
  	g_object_set(appsink, "drop", true, NULL);
  	g_object_set (appsink, "emit-signals", true, NULL);
  	gst_bin_add_many (GST_BIN (pipeline), src, rtph_depay, parse, videodec, appsink, NULL);
  	//生成我们需要的节点
  	
  	g_signal_connect(src, "pad-added", G_CALLBACK(cb_new_rtspsrc_pad), rtph_depay);//需要给rtsp解包注册回调

  	
  	gst_element_link_many (rtph_depay, parse, videodec, appsink, NULL);//连接所有元素

	g_signal_connect(rtph_depay, "pad-added", G_CALLBACK(on_pad_added), parse);

  	g_signal_connect (appsink, "new-sample", G_CALLBACK (your_thread), &data_instance);//给解码buf处理线程做回调
  	
  	//后面的都是标准化的代码，在gst资料中都有
  	sret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  	if (sret == GST_STATE_CHANGE_FAILURE) {
		gst_element_set_state (pipeline, GST_STATE_NULL);
    	gst_object_unref (pipeline);
    	printf("gst_element_set_state GST_STATE_PLAYING is fail.\n");
    	return -1;
  	}
  	GstBus* bus = gst_element_get_bus(pipeline);
  	GstMessage* msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
  	if (msg != NULL) {
		gst_message_unref(msg);
  	}
  	gst_object_unref (bus);
 
  	gst_element_set_state(pipeline, GST_STATE_NULL);
  	gst_object_unref(pipeline);

  	
  	return 0;
}
```

