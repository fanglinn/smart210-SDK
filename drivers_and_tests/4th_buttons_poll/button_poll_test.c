#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

int main(int argc, char **argv)
{
	int fd ,ret ;

	unsigned char key_value;
	int cnt = 0;
	struct pollfd fdsa[1];


	fd = open("/dev/mykey", O_RDWR);
	if(fd < 0)
	{
		printf("open err ! \n");
		return -1;
	}

	fdsa[0].fd      = fd; 
	fdsa[0].events  = POLLIN; 

	while(1)
	{
		ret = poll( &fdsa[0], 1, 5000);
		if(!ret)
		{
			printf("timeout\n");
		}
		else
		{
			read(fd, &key_value, 1);
        	        printf("key_value = 0x%x \n", key_value);
		}
	}
	
	return 0;
}

