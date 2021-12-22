#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

using namespace std;

class rknn_component_args
{
  private:
  	void usage_tip(FILE *fp, int argc, char **argv);
  public:
  	char* model_path;
  	char* dev_path;
    rknn_component_args(int argc, char **argv);
    ~rknn_component_args();
};

