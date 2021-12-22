/*
 * Copyright 2021 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define GDK_VERSION_MIN_REQUIRED (GDK_VERSION_3_0)

#include <glib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rknn_mobilenet.h"
#include "argc.h"

int cou = 0;
GtkWidget *info_widget;

static void window_closed (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
  GstElement *pipeline = (GstElement*)user_data;

  gtk_widget_hide (widget);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gtk_main_quit ();
}

/* slightly convoluted way to find a working video sink that's not a bin,
 * one could use autovideosink from gst-plugins-good instead
 */
static GstElement *find_video_sink (void)
{
  GstStateChangeReturn sret;
  GstElement *sink;

  if ((sink = gst_element_factory_make ("xvimagesink", "xvimagesink"))) {
    sret = gst_element_set_state (sink, GST_STATE_READY);
    if (sret == GST_STATE_CHANGE_SUCCESS)
      return sink;

    gst_element_set_state (sink, GST_STATE_NULL);
    gst_object_unref (sink);
  }

  if ((sink = gst_element_factory_make ("ximagesink", "ximagesink"))) {
    sret = gst_element_set_state (sink, GST_STATE_READY);
    if (sret == GST_STATE_CHANGE_SUCCESS)
      return sink;

    gst_element_set_state (sink, GST_STATE_NULL);
    gst_object_unref (sink);
  }

  return NULL;
}

static void update_text_ui (int label, float score, long timecost) {
  GtkTextBuffer *info_text_ui;
  gchar *str, *total_str_ui;
  gdk_threads_enter(); 
  info_text_ui = gtk_text_view_get_buffer (GTK_TEXT_VIEW (info_widget));
  gtk_text_buffer_set_text (info_text_ui, "", -1);
  total_str_ui = g_strdup_printf ("Reasoning label:%d   Score：%8.6f time_cost: %ld ms.\n", label, score, timecost);
  gtk_text_buffer_insert_at_cursor (info_text_ui, total_str_ui, -1);
  g_free (total_str_ui);
  gdk_threads_leave();
}

/* The appsink has received a buffer */
static GstFlowReturn rknn_thread (GstElement *sink , rknn_mobilenet *rknn_instance) {
  GstSample *sample;
  GstBuffer *buffer;
  GstMapInfo map;
  // gint sample_width;
  // gint sample_height;
  // GstStructure *info;
  // GstCaps *caps;
  // FILE *fp;
  g_signal_emit_by_name (sink, "pull-sample", &sample);
  buffer = gst_sample_get_buffer (sample);
  if (buffer) {
    gst_buffer_map (buffer, &map, GST_MAP_READ);
    rknn_instance->run(map.data);
    //printf("run_results = %d.\n", rknn_instance->run_results);
    update_text_ui(rknn_instance->run_results, rknn_instance->run_results_score, rknn_instance->time_cost);
    // caps = gst_sample_get_caps (sample);
    // info = gst_caps_get_structure (caps, 0);
    // gst_structure_get_int (info, "width", &sample_width);
    // gst_structure_get_int (info, "height", &sample_height);
    // printf ("width = %d. height = %d. size = %d.\n", sample_width, sample_height, map.size);
    // //printf("strlen(map.data) = %ld.\n",strlen((char*)map.data));
    // if (cou == 3) {
    //   fp = fopen("dump.yuv","w+");
    //   fwrite(map.data, 1, map.size, fp);
    //   fclose(fp);
    //   printf ("buffer dump success!\n");
    // }
    // cou++;
    gst_buffer_unmap (buffer, &map);
    gst_sample_unref (sample);
    return GST_FLOW_OK;
  }

  return GST_FLOW_ERROR;
}

int main (int argc, char *argv[])
{
  rknn_component_args args_instance(argc, argv);
  int ret = 0;
  //gtk+ 
  GdkWindow *video_window_xwindow;
  GtkWidget *window, *video_window, *box;
  GtkTextBuffer *info_text;
  gchar *str, *total_str;
  gulong uret;

  //gst
  char *device_path;
  if(args_instance.dev_path == NULL) {
    device_path = (char*)"/dev/video8";
  }else {
    device_path = args_instance.dev_path;
  }
  GstElement *pipeline, *src, *sink, *appsink, *tee, 
             *queue_video, *queue_rknn, *convert, *scale_video;
  GstStateChangeReturn sret;
  GstCaps *caps_scale, *caps_convert, *caps_format;
  GstSample *sample;
  GstPadTemplate *tee_src_pad_template;
  GstPad *tee_rknn_pad, *tee_video_pad;
  GstPad *queue_rknn_pad, *queue_video_pad;

  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);

  //rknn
  int rknn_w = 224;
  int rknn_h = 224;
  int rknn_c = 3;
  char *model_path;
  if(args_instance.model_path == NULL) {
    model_path = (char*)"model/mobilenet_v1_rk3588.rknn";
  }else {
    model_path = args_instance.model_path;
  }
  rknn_mobilenet rknn_instance(rknn_w, rknn_h, rknn_c);
  ret = rknn_instance.init(model_path);
  if(ret < 0 ) {
    printf("rknn init fail\n");
    return -1;
  }
  /* prepare the pipeline */

  pipeline = gst_pipeline_new ("xvoverlay");
  src = gst_element_factory_make ("v4l2src", "v4l2src");
  g_object_set(G_OBJECT(src),"device", device_path, NULL);
  convert = gst_element_factory_make ("videoconvert", "videoconvert_in");
  scale_video = gst_element_factory_make ("videoscale", "videoscale_video");
  queue_video = gst_element_factory_make ("queue", "queue_video");
  queue_rknn = gst_element_factory_make ("queue", "queue_rknn");
  g_object_set(queue_video, "max-size-buffers", 3, NULL);
  g_object_set(queue_rknn, "max-size-buffers", 3, NULL);
  appsink = gst_element_factory_make ("appsink", "appsink");
  g_object_set(appsink, "max-buffers", 3, NULL);
  g_object_set(appsink, "drop", True, NULL);
  g_object_set (appsink, "emit-signals", TRUE, NULL);
  sink = find_video_sink ();
  if (sink == NULL) {
    g_error ("Couldn't find a working video sink.");
  }
  tee = gst_element_factory_make ("tee", "tee");
  caps_scale = gst_caps_from_string("video/x-raw, format=NV12, width=224, height=224, pixel-aspect-ratio=1/1");
  caps_convert = gst_caps_from_string("video/x-raw,format=RGB,width=224, height=224, framerate=30/1");
  caps_format = gst_caps_from_string("video/x-raw,format=NV12,width=1920, height=1080, framerate=30/1");
  //caps_debug = gst_caps_from_string("video/x-raw,format=RGB,width=224, height=224, framerate=30/1, pixel-aspect-ratio=1/1");
  gst_bin_add_many (GST_BIN (pipeline), src, tee, queue_video, queue_rknn, scale_video, sink, convert, appsink, NULL);
  gst_element_link_filtered (src, scale_video, caps_format);
  gst_element_link_filtered (scale_video, tee, caps_scale);
  gst_element_link_filtered (convert, appsink, caps_convert);

  gst_element_link_many (src, scale_video, tee, NULL);
  gst_element_link_many (tee, queue_video, sink, NULL);
  gst_element_link_many (tee, queue_rknn, convert, appsink, NULL);

//debug
  // gst_bin_add_many (GST_BIN (pipeline), src, appsink, scale, convert, NULL);
  // gst_element_link_filtered (src, scale, caps_format);
  // gst_element_link_filtered (scale, convert, caps_scale);
  // gst_element_link_filtered (convert, appsink, caps_convert);
  // gst_element_link_many (src, scale, convert, appsink, NULL);
//debug

  gst_caps_unref (caps_convert);
  gst_caps_unref (caps_scale);
  gst_caps_unref (caps_format);
  // gst_caps_unref (caps_debug);

  /* prepare the ui */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (window_closed), (gpointer) pipeline);
  g_signal_connect (appsink, "new-sample", G_CALLBACK (rknn_thread), &rknn_instance);
  gtk_window_set_title (GTK_WINDOW (window), "rknn_component");

  info_widget = gtk_text_view_new ();
  info_text = gtk_text_view_get_buffer (GTK_TEXT_VIEW (info_widget));
  total_str = g_strdup_printf ("Reasoning label:   Score： \n");
  gtk_text_buffer_insert_at_cursor (info_text, total_str, -1);
  g_free (total_str);

  video_window = gtk_drawing_area_new ();

  box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), video_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), info_widget, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show_all (window);

  video_window_xwindow = gtk_widget_get_window (video_window);
  uret = GDK_WINDOW_XID (video_window_xwindow);
  gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink), uret);

  /* run the pipeline */
  sret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (sret == GST_STATE_CHANGE_FAILURE) {
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    rknn_instance.release();
    printf("gst_element_set_state GST_STATE_PLAYING is fail.\n");
    return -1;
  }
  gdk_threads_init(); 
  gdk_threads_enter(); 
  gtk_main ();
  gdk_threads_leave();  
  // gst_object_unref (pipeline);
  // rknn_instance.release();
  return 0;
}

