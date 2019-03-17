#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
        int fd;

        fd = open("/dev/chrdev", O_RDWR);
        if(fd < 0)
        {
                printf("open /dev/chrdev fail . errcode : %d \n", fd);
                return -1;
        }

        close(fd);
        return 0;
}

