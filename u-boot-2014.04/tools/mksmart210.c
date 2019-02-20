#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define IMG_SIZE        16 * 1024
#define HEADER_SIZE     16

int main (int argc, char *argv[])
{
        FILE                    *fp;
        unsigned                char *buffer;
        int                             bufferLen;
        int                             nbytes, fileLen;
        unsigned int    checksum, count;
        int                             i;

        if (argc != 3)
        {
                printf("Usage: %s <source file> <destination file>\n", argv[0]);
                return -1;
        }

        /* 分配16KByte的buffer,BL1最大为16KByte,并初始化为0 */
        buffer = calloc(1, IMG_SIZE);
        if (!buffer)
        {
                perror("Alloc buffer failed!");
                return -1;
        }

        /* 打开源bin文件 */
        fp = fopen(argv[1], "rb");
        if( fp == NULL)
        {
                perror("source file open error");
                free(buffer);
                return -1;
        }
        /* 获取源bin文件的长度 */
        fseek(fp, 0L, SEEK_END);
        fileLen = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        /* 源bin文件不得超过(16K-16)Byte */
        if (fileLen > (IMG_SIZE - HEADER_SIZE))
        {
                fprintf(stderr, "Source file is too big(> 16KByte)\n");
                free(buffer);
                fclose(fp);
        }

        /* 计算校验和 */
        i = 0;
        checksum = 0;
        while (fread(buffer + HEADER_SIZE + i, 1, 1, fp))
        {
                checksum += buffer[HEADER_SIZE + i++];
        }
        fclose(fp);


        /* 计算BL1的大小(BL1的大小包括BL1的头信息),并保存到buffer[0~3]中 */
        fileLen += HEADER_SIZE;
        memcpy(buffer, &fileLen, 4);

        // 将校验和保存在buffer[8~15]
        memcpy(buffer + 8, &checksum, 4);

        /* 打开目标文件 */
        fp = fopen(argv[2], "wb");
        if (fp == NULL)
        {
                perror("destination file open error");
                free(buffer);
                return -1;
        }
		// 将buffer拷贝到目标bin文件中
        nbytes  = fwrite(buffer, 1, fileLen, fp);
        if (nbytes != fileLen)
        {
                perror("destination file write error");
                free(buffer);
                fclose(fp);
                return -1;
        }

        free(buffer);
        fclose(fp);

        return 0;
}
