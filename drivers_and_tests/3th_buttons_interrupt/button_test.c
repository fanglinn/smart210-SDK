#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char **argv)
{
	int fd ;

	unsigned char key_value;
	int cnt = 0;

	fd = open("/dev/mykey", O_RDWR);
	if(fd < 0)
	{
		printf("open err ! \n");
		return -1;
	}

	while(1)
	{
		read(fd, &key_value, 1);
		printf("key_value = 0x%x \n", key_value);
	}
	
	return 0;
}
