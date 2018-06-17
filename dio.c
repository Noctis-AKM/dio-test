#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __USE_GNU 1
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define W_FLAG (O_RDWR | O_CREAT | O_TRUNC)

#define PAGE_SIZE 4096
#define ROUND_UP(x, y) ((x + y - 1) / y * y)
#define FILE_SIZE (1024 * 1024 * 100)

int main(int argc, char *argv[])
{
	char *buf, *p, *endp;
	int ret  = 0;
	int i, fd;
	long buf_size = PAGE_SIZE;
	long long file_size = FILE_SIZE;
	int flag = W_FLAG;
	int ch, count;

	while ((ch = getopt(argc, argv, "b:ds:")) != -1) {
		switch (ch) {
			case 'b':
				buf_size = strtol(optarg, &endp, 10);
				if (*endp == 'k')
					buf_size *= 1024;
				else if (*endp == 'm')
					buf_size *= 1024 * 1024;
				break;
			case 'd':
				flag |= O_DIRECT;
				break;
			case 's':
				file_size = strtol(optarg, &endp, 10);
				if (*endp == 'm')
					file_size *= 1024 * 1024;
				else if (*endp == 'g')
					file_size *= 1024 * 1024 * 1024;
				break;
			case '?':
		   		printf("Unknown option: %c\n",(char)optopt);
		   		break;
		}
	}

	printf("buf_size %ld, file size %lld, flag %o, test file %s\n", buf_size, file_size,flag, argv[optind]);

	if (optind >= argc) {
		ret = -1;
		printf("invalid argument.\n");
		goto out;
	}

	fd = open(argv[optind], flag, 0644);
	if (fd < 0) {
		perror("open:");
		return -1;
	}

	buf = malloc(2 * buf_size - 1);
	if (!buf) {
		ret = -1;
		perror("malloc:");
		goto out;
	}

	p = buf;
	buf = (char *)ROUND_UP((unsigned long)buf, buf_size);

	count = file_size / buf_size;
	for (i = 0; i < count; i++) {
		memset(buf, 0xa5, buf_size);
		ret = write(fd, buf, buf_size);
		if (ret != buf_size) {
			perror("write: ");
			ret = -1;
			goto out1;
		}
	}

out1:
	free(p);
out:
	close(fd);
	return ret;
}
