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


#define RTSP_PATH "rtsp://127.0.0.1:8554/test"	//视频流的网址，根据你的实际情况来
#define CAPS "video/x-raw,format=RGB,pixel-aspect-ratio=1/1"	//设置appsink输出的视频格式，这里最好要设置，不设置可能会有一些错误

GST_DEBUG_CATEGORY_STATIC (my_category);

int i;

typedef struct _CustomData {
    GstElement *pipeline;		//gstreamer管道
    GstElement *source;		//输入源，这里用的是rtspsrc
    GstElement *depayloader;	//提取数据，因为我的视频流是h264格式，所以这里用的是rtph264depay
    GstElement *avdec;		//视频解码，因为我提取的是h264数据，所以这里用的是avdec_h264
    GstElement *convert1,*convert2;		//视屏转换，这里用的是videoconvert
    GstElement *scale;		//充定义视屏尺寸，默认是按源的尺寸来定义，这里用的是videoscale
    GstElement *tee,*sink,*appsink;			//视屏输出，由于我们需要获取视频的输出数据，所以这里使用appsink.有些人可能有疑问为什么不用multifilesrc，因为你没法控制每一帧
    GstElement *capture_queue, *video_queue;


    GstMapInfo map_info; //cht  截图
    int Width = 640, Height = 480;  //截图分辨率
    bool capture_flag = true;  //截图标志 默认

    GMainLoop *main_loop;  /* GLib's Main Loop */
} CustomData;

/* 这个是appsink的信号new-sample的回调函数，表示已经接收到一帧数据 */
static GstFlowReturn new_sample (GstElement *sink, CustomData *data){

    /* 我这里控制输出1分钟，如果不需要可以自行修改代码 */
    if(i >= 30*60){
        g_main_loop_quit (data->main_loop);		//结束程序
        return GST_FLOW_OK;
    }

    i++;

    if ( i%(30*5) == 0){   // 每（i/30）秒 保存一张图

        //qDebug() << "i==== " << i;
        GstSample *sample;
        GstBuffer *buffer;
        GstCaps *caps;
        GstStructure *s;
        gint width, height;	//图片的尺寸

        /* Retrieve the buffer */
        g_signal_emit_by_name (sink, "pull-sample", &sample);
        if (sample){
            //g_print ("*");
            caps = gst_sample_get_caps (sample);
            if (!caps) {
                g_print ("gst_sample_get_caps fail\n");
                gst_sample_unref (sample);
                return GST_FLOW_ERROR;
            }
            s = gst_caps_get_structure (caps, 0);
            gboolean res;
            res = gst_structure_get_int (s, "width", &width);		//获取图片的宽
            res |= gst_structure_get_int (s, "height", &height);	//获取图片的高
            if (!res) {
                g_print ("gst_structure_get_int fail\n");
                gst_sample_unref (sample);
                return GST_FLOW_ERROR;
            }
            buffer = gst_sample_get_buffer (sample);		//获取视频的一帧buffer,注意，这个buffer是无法直接用的，它不是char类型
            if(!buffer){
                g_print ("gst_sample_get_buffer fail\n");
                gst_sample_unref (sample);
                return GST_FLOW_ERROR;
            }

            GstMapInfo map;
            char name[32] = {0};
            sprintf(name,"frame%d.png",i);
            if (gst_buffer_map (buffer, &map, GST_MAP_READ)){		//把buffer映射到map，这样我们就可以通过map.data取到buffer的数据
    //            GError *error = NULL;
                //            GdkPixbuf * pixbuf = gdk_pixbuf_new_from_data (map.data,GDK_COLORSPACE_RGB, FALSE, 8, width, height,GST_ROUND_UP_4 (width * 3), NULL, NULL);	//将数据保存到GdkPixbuf

                //            /* save the pixbuf */
                //            gdk_pixbuf_save (pixbuf, name, "jpeg", &error, NULL);	//把GdkPixbuf保存成jpeg格式
                //            g_object_unref(pixbuf);		//释放掉GdkPixbuf
                //            gst_buffer_unmap (buffer, &map);	//解除映射

                g_print("apply buffer\n");

                // char *jpg = new char[map.size];
                // memcpy(jpg, map.data, map.size);
                // g_print("jpg size = %ld \n", map.size);

                // QImage img(map.data, width, height, width * 3, QImage::Format_RGB888);

                // QString strFile = QString("Capture-%0.jpg").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmssz"));
                // img.save(strFile, "jpg");
                // g_print("save  %s succese\n",strFile.toStdString().c_str());

                gst_buffer_unmap (buffer, &map);	//解除映射
            }
            gst_sample_unref (sample);	//释放资源
            GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(data->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "capture");
            return GST_FLOW_OK;
        }
    }
    return GST_FLOW_OK ;
}

/*获取rgb帧数据 -- 截图功能 */
// static GstPadProbeReturn
// cb_have_data (GstPad          *pad,
//               GstPadProbeInfo *info,
//               CustomData      *date){


//     i++;
//     qDebug() << "i==== " << i;
//     /* 我这里控制输出10张，如果不需要可以自行修改代码 */
//     if(i >= 10){
//         g_main_loop_quit (date->main_loop);		//结束程序
//         return GST_PAD_PROBE_OK;
//     }
//         g_print ("Enter cb_have_data\n");

//         // 使用mapinfo获取图像数据

//         GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);

//         if (!gst_buffer_map(buffer, &date->map_info, GST_MAP_READ)) {
//             g_print("gst_buffer_map() error!");
//             return GST_PAD_PROBE_DROP;
//         }

//                 g_print("apply buffer\n");
//         char *jpg = new char[date->map_info.size];
//         memcpy(jpg, date->map_info.data, date->map_info.size);
//         g_print("jpg size = %ld \n", date->map_info.size);

//         QImage img(date->map_info.data, 640, 480, 640 * 3, QImage::Format_RGB888);
//         QString strFile = QString("Capture-%0.jpg").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmssz"));
//         img.save(strFile, "jpeg");
//         g_print("save  %s succese\n",strFile.toStdString().c_str());

//         gst_buffer_unmap (buffer, &date->map_info);
//   //      date->capture_flag = false;

//         GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(date->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "capture");

//     return GST_PAD_PROBE_OK;
// }


/* 这个是pad-added信号的回调函数 */
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data){

    g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));	//new_pad是data->source的source pad

    GstPad *sink_pad = gst_element_get_static_pad (data->depayloader, "sink");	//获取到data->depayloader的sink pad

    /* 如果depayloader已经连接好了，那就忽略 */
    if (gst_pad_is_linked (sink_pad)) {
        g_print ("We are already linked. Ignoring.\n");
        goto exit;
    }

    GstCaps *new_pad_caps;
    GstStructure *new_pad_struct;
    const gchar *new_pad_type;

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps (new_pad);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    if (!g_str_has_prefix (new_pad_type, "application/x-rtp")) {		//因为depayloader（rtph264depay）要求application/x-rtp格式的输入数据，所以我们找source的pad里面有没有这种格式的数据
        g_print ("It has type '%s' which is not application/x-rtp. Ignoring.\n", new_pad_type);
        goto exit;
    }

    GstPadLinkReturn ret;
    /* Attempt the link */
    ret = gst_pad_link (new_pad, sink_pad);	//把source（rtspsrc）的source pad和depayloader（rtph264depay）的sink pad连起来
    if (GST_PAD_LINK_FAILED (ret)) {
        g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } else {
        g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }

exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
        gst_caps_unref (new_pad_caps);

    /* Unreference the sink pad */
    gst_object_unref (GST_OBJECT(sink_pad));
}

/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus *bus, GstMessage *msg, CustomData *data) {
    GError *err;
    gchar *debug_info;

    /* Print error details on the screen */
    gst_message_parse_error (msg, &err, &debug_info);
    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error (&err);
    g_free (debug_info);

    g_main_loop_quit (data->main_loop);	//结束程序
}

int main(int argc, char *argv[]){

    g_setenv("GST_DEBUG_DUMP_DOT_DIR", "/tmp/test", TRUE);
    GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");

    i = 0;	//这个用于控制输出100张，不要可以去掉
    CustomData data;	//定义所有gstreamer元素
    memset (&data, 0, sizeof (data));

    /* Initialize GStreamer */
    gst_init (&argc, &argv);	//初始化gstreamer，这一步是一定要的


    GstPad *tee_capture_pad, *tee_video_pad;
    GstPad *queue_capture_pad, *queue_video_pad;


    data.source = gst_element_factory_make ("rtspsrc", "source"); //rtspsrc
    data.depayloader = gst_element_factory_make ("rtph264depay", "depayloader");
    data.avdec = gst_element_factory_make ("avdec_h264", "avdec");
    data.convert1 = gst_element_factory_make ("videoconvert", "convert1");
    data.convert2 = gst_element_factory_make ("videoconvert", "convert2");
    data.scale = gst_element_factory_make ("videoscale", "scale");

    data.tee = gst_element_factory_make ("tee", "tee");

    data.video_queue = gst_element_factory_make ("queue", "video_queue");
    data.capture_queue = gst_element_factory_make ("queue", "capture_queue");

    data.appsink = gst_element_factory_make ("appsink", "sink");
    data.sink = gst_element_factory_make ("ximagesink", "videosink");

    data.pipeline = gst_pipeline_new ("test-pipeline");

    if (!data.pipeline || !data.source ||  !data.depayloader || !data.avdec || !data.convert1 || !data.scale || !data.sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

//    g_object_set (data.source, "location", "rtsp://192.168.31.36:8554/test", NULL);	//设置rtspsrc的路径
    g_object_set (data.source, "location", RTSP_PATH, NULL);	//设置rtspsrc的路径

    /* Configure appsink */
    GstCaps *video_caps;
    gchar *video_caps_text;
    video_caps_text = g_strdup_printf (CAPS);
    video_caps = gst_caps_from_string (video_caps_text);
    if(!video_caps){
        g_printerr("gst_caps_from_string fail\n");
        return -1;
    }

    g_object_set (data.appsink, "emit-signals", TRUE, "caps", video_caps, NULL);	//我们必须把appsink的emit-signals属性打开，我们才能收到new-sample信号，同时还设置了输出视频格式
    g_signal_connect (data.appsink, "new-sample", G_CALLBACK (new_sample), &data);	//设置监听new-sample和回调函数new_sample，&data是传给回调函数的参数
    gst_caps_unref (video_caps);
    g_free (video_caps_text);


    // /* 因为data.source和data.depayloader还没有连接，所有必须设置pad-added信号监听，在管道开始工作后，确定了数据格式，再把它们连接起来 */
    g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler), &data);

    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many (GST_BIN (data.pipeline), data.source,data.depayloader,data.avdec,
                      data.tee,data.capture_queue,data.video_queue,data.convert1,data.convert2,
                      data.scale,data.sink,data.appsink, NULL);	//把所有元素都放到管道里面

    /* 连接元素，注意顺序不能错，data.source是没法在这里和data.depayloader连接的，因为还没有确定数据的格式 */

    if (gst_element_link_many (data.depayloader, data.avdec, data.tee, NULL) != TRUE ||
        gst_element_link_many (data.capture_queue, data.convert1, data.appsink, NULL) != TRUE ||
        gst_element_link_many (data.video_queue, data.convert2,data.scale, data.sink, NULL) != TRUE) {
      g_printerr ("Elements could not be linked.\n");
      gst_object_unref (data.pipeline);
      return -1;
    }

    tee_capture_pad = gst_element_get_request_pad (data.tee, "src_%u");
    g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_capture_pad));
    queue_capture_pad = gst_element_get_static_pad (data.capture_queue, "sink");
    tee_video_pad = gst_element_get_request_pad(data.tee, "src_%u");
    g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
    queue_video_pad = gst_element_get_static_pad (data.video_queue, "sink");
    if (gst_pad_link (tee_capture_pad, queue_capture_pad) != GST_PAD_LINK_OK ||
        gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
        g_printerr ("Tee could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
      }
    gst_object_unref (queue_capture_pad);
    gst_object_unref (queue_video_pad);


/*
    //date probe 获取帧数据 拍照功能
     GstPad *pad = gst_element_get_static_pad (data.convert1, "src");
     gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER,
         (GstPadProbeCallback) cb_have_data, &data, NULL);
     gst_object_unref (pad);
*/
    //    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    GstBus *bus;
    bus = gst_element_get_bus (data.pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, &data);	//这里监听是否运行出错，出错就结束程序

    /* Start playing the pipeline */
    if(gst_element_set_state (data.pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {//启动管道开始工作
        //qDebug() << "Unable to set the pipeline to playing state!";
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(data.pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");

    /* Create a GLib Main Loop and set it to run */
    data.main_loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (data.main_loop);	//程序开始循环跑，不设置这个的话，程序就直接退出了

    /* Free resources */
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT(data.pipeline));

    return 0;
}
