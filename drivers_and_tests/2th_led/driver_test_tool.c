#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>


static char device_name[32] = "/dev/xxx";

#define MAX_ARGS			8

static int cmd_args[MAX_ARGS];

static char *short_options = "c:o:hv";
static struct option long_options[] =
{
	{ "open",  		required_argument, 	NULL, 'o' },
	{ "close", 		required_argument, 	NULL, 'c' },
	{ "Help", 		no_argument, 		NULL, 'H' },
	{ "version", 	no_argument, 		NULL, 'V' },
	{ 0, 0, 0, 0 }
};


static int g_help_flag = 0;
static int g_version_flag = 0;

static int major = 0;
static int minor = 1;
static void version()
{
	 printf("version-%d.%d \r\n", major, minor);
}


static void usage()
{
  fprintf(stderr,
	"Usage: driver_test_tool  [Device] [cmd] [data] ...\n\n"
	"Options:\n"
	"  -o  --open               open the device of offset, \n"
	"  -c  --close              close   		       \n"

    "  -h, --help               help information \n"
    "  -v, --version            version information \n"
    "example: \n"
    " driver_test_tool  %s  -o 0 1   (cmd 0, args : 1)  \n\n" , device_name);
}


static int drv_parse_cls(int argc , char **argv)
{
	char *dev_name = device_name;
	while(1)
	{
		int c;

    	c = getopt_long(argc,argv,short_options,long_options,NULL);
		if(c == -1)
      		break;
		switch(c)
		{
			case 'h':
			case 'H':
			case '?':
				g_help_flag = 1;
				return 0;

			case 'v':
				g_version_flag = 1;
				return 0;

			case 'o':
				sscanf(optarg,"%x",&cmd_args[0]);
				break;

			case 'c':
				sscanf(optarg,"%x",&cmd_args[0]);
				break;			
		}
	}

	if(optind >= argc)
 	{
    	printf("No device found \n");
    	return -1;
	}

	memcpy(dev_name, argv[optind], strlen(argv[optind]));
	++optind;

	int count = 1;
	while(optind < argc)
	{
		char *opt = argv[optind++];

		char *endptr;
		int cmd = strtol(opt,&endptr,16);

		if(*opt == '\0')
		{
			printf("Invalid command byte '%s'\n",opt);
			return -1;
		}

		if(count >= MAX_ARGS)
		{
			break;
		}

    	cmd_args[count] = cmd;
    	++count;
  }

  return 0;
}

int main(int argc , char **argv)
{
	int ret ;
	int fd = -1;

	ret = drv_parse_cls(argc, argv);
	if(ret != 0)
	{
		usage();
		return -1;
	}

	if(1 == g_help_flag)
	{
		usage();
		return 0;
	}
	else if(1 == g_version_flag)
	{
		version();
		return 0;
	}

	fd = open(device_name, O_RDWR);
	if(fd < 0)
	{
		printf("open fail . \n");
		return -1;
	}

	write(fd, cmd_args , MAX_ARGS);

	close(fd);
	
	return 0;
}


