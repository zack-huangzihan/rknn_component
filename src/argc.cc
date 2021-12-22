#include "argc.h"

void rknn_component_args::usage_tip(FILE *fp, int argc, char **argv) {
	fprintf(fp,
	        "Usage: %s [options]\n"
	        "Version %s\n"
	        "Options:\n"
	        "-d | --device  device  /dev/video8, default node.\n"
	        "-m | --model_path  default is model/mobilenet_v1_rk3588.rknn\n"
	        "-h | --help        for help \n\n"
	        "\n",
	        argv[0], "V1.0.0");
}

rknn_component_args::rknn_component_args(int argc, char *argv[]) {
	const char short_options[] = "d:m:h:";
	const struct option long_options[] = {{"device", required_argument, NULL, 'd'},
                                             {"model_path", no_argument, NULL, 'm'},
                                             {"help", no_argument, NULL, 'h'},
                                             {0, 0, 0}};
    dev_path = NULL;
    model_path = NULL;
	for (;;) {
		int idx;
		int c;
		c = getopt_long(argc, argv, short_options, long_options, &idx);
		if (-1 == c)
			break;
		switch (c) {
		case 0: /* getopt_long() flag */
			break;
		case 'd':
			dev_path = optarg;
			break;
		case 'm':
			model_path = optarg;
			break;
		case 'h':
			usage_tip(stdout, argc, argv);
			exit(EXIT_SUCCESS);
		default:
			usage_tip(stderr, argc, argv);
			exit(EXIT_FAILURE);
		}
	}
	printf("dev_path=%s; model_path=%s.\n", dev_path, model_path);
}

rknn_component_args::~rknn_component_args() {
	printf("relese rknn_component_args\n");
}