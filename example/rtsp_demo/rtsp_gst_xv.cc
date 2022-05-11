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

#define  URI "rtsp://172.16.21.6/live/2"

GMainLoop* loop;

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

    // here, you would setup a new pad link for the newly created pad
    // sooo, now find that rtph264depay is needed and link them?
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

int main (int argc, char *argv[])
{
	char *rtsp_path;
	get_args args_instance(argc, argv);
  if(args_instance.RtspPath == NULL) {
    printf("rtsp uri is NULL\n");
    return -1;
  }else {
    rtsp_path = args_instance.RtspPath;
  }
	gst_init (&argc, &argv);
	GstStateChangeReturn sret;
	GstElement *pipeline, *src, *rtph_depay, *parse, *videodec, *xvsink;
	pipeline = gst_pipeline_new ("pipeline");
	src = gst_element_factory_make ("rtspsrc", "rtspsrc");
	g_object_set(G_OBJECT(src), "location", rtsp_path, NULL);
	rtph_depay = gst_element_factory_make ("rtph265depay", "rtph_depay");
	parse = gst_element_factory_make ("h265parse", "parse");
	videodec = gst_element_factory_make("mppvideodec", "videodec");
	xvsink = gst_element_factory_make ("xvimagesink", "xvsink");
	if(!pipeline || !src || !rtph_depay || !parse || !videodec || !xvsink){
		g_printerr("Not all elements could be created !\n");
		return -1;
	}


  gst_bin_add_many (GST_BIN (pipeline), src, rtph_depay, NULL);

	g_signal_connect(src, "pad-added", G_CALLBACK(cb_new_rtspsrc_pad), rtph_depay);
	
	gst_bin_add_many (GST_BIN (pipeline), parse, NULL);

	if (!gst_element_link(rtph_depay, parse)) {
		printf("Failed to link rtph_depay to parse\n");
		return -1;
	}

	gst_bin_add_many (GST_BIN (pipeline), videodec, xvsink, NULL);
	
	if (!gst_element_link(parse, videodec)) {
		printf("Failed to link parse to videodec\n");
		return -1;
	}

	if (!gst_element_link_many(videodec, xvsink, NULL)) {
		printf("Failed to link videodec to xvsink\n");
		return -1;
	}
	g_signal_connect(rtph_depay, "pad-added", G_CALLBACK(on_pad_added), parse);

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