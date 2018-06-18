#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#define _GNU_SOURCE
#define __USE_GNU 1
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#define W_FLAG (O_RDWR | O_CREAT | O_TRUNC)

#define PAGE_SIZE 4096
#define ROUND_UP(x, y) ((x + y - 1) / y * y)
#define FILE_SIZE (1024 * 1024 * 100)

#define BUF_WRITE 0
#define DIRECT_WRITE 1
#define MMAP_TRUNC 2
#define MMAP_ALLOC 2

enum {
	TST_BUFFER,
	TST_MMAP_COPY,
	TST_MMAP
};

int buffer_write(int fd, int buf_size, long long file_size)
{
	char *buf, *p;
	int count, i, ret = -1;
	buf = malloc(2 * buf_size - 1);
	if (!buf) {
		perror("malloc:");
		goto out;
	}

	p = buf;
	/* round it for direct io */
	buf = (char *)ROUND_UP((unsigned long)buf, buf_size);
	count = file_size / buf_size;
	for (i = 0; i < count; i++) {
		memset(buf, 0xa5, buf_size);
		ret = write(fd, buf, buf_size);
		if (ret != buf_size) {
			perror("write: ");
			ret = -1;
			goto out;
		}
	}

	ret = 0;
out:
	free(p);
	return ret;
}

int mmap_write_copy(int fd, int buf_size, long long file_size)
{
	char *buf, *maddr;
	int count, i, ret = -1;

	buf = malloc(buf_size);
	if (!buf) {
		perror("malloc:");
		goto out;
	}

	count = file_size / buf_size;
	for (i = 0; i < count; i++) {
		memset(buf, 0xa5, buf_size);
		maddr = mmap(NULL, buf_size, PROT_WRITE, MAP_SHARED, fd, i * buf_size);
		if (maddr == MAP_FAILED) {
			perror("mmap:");
			goto out;
		}

		memcpy(maddr, buf, buf_size);
		munmap(maddr, buf_size);
	}

	ret = 0;
out:
	free(buf);
	return ret;
}

int __mmap_write(int fd, int buf_size, long long file_size)
{
	int i, count;
	char *maddr;
	int ret = -1;

	count = file_size / buf_size;
	for (i = 0; i < count; i++) {
		maddr = mmap(NULL, buf_size, PROT_WRITE, MAP_SHARED, fd, i * buf_size);
		if (maddr == MAP_FAILED) {
			perror("mmap:");
			goto out;
		}
		memset(maddr, 0xa5, buf_size);
		munmap(maddr, buf_size);
	}

	ret = 0;
out:
	return ret;
}

void mmap_write(int fd, int buf_size, long long file_size, int copy)
{
	int ret;
	if (copy)
		ret = mmap_write_copy(fd, buf_size, file_size);
	else
		ret = __mmap_write(fd, buf_size, file_size);

	if (ret != 0)
		printf("%s failed.\n", __func__);
}

int main(int argc, char *argv[])
{
	char *buf, *p, *endp;
	int ret  = 0;
	int fd;
	long buf_size = PAGE_SIZE;
	long long file_size = FILE_SIZE;
	int flag = W_FLAG;
	int ch;
	int ftrun_flag = 0;
	int falloc_flag = 0;

	int test_case = 0;

	while ((ch = getopt(argc, argv, "ab:c:ds:t")) != -1) {
		switch (ch) {
			case 'a':
				falloc_flag = 1;
				break;
			case 'b':
				buf_size = strtol(optarg, &endp, 10);
				if (*endp == 'k')
					buf_size *= 1024;
				else if (*endp == 'm')
					buf_size *= 1024 * 1024;
				break;
			case 'c':
				test_case = strtol(optarg, &endp, 10);
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
			case 't':
				ftrun_flag = 1;
				break;
			case '?':
		   		printf("Unknown option: %c\n",(char)optopt);
		   		break;
		}
	}

	printf("test case %d, buf_size %ld, file size %lld, flag %o, test file %s\n",
		   test_case, buf_size, file_size, flag, argv[optind]);

	if (optind >= argc || (ftrun_flag && falloc_flag)) {
		printf("invalid argument.\n");
		goto out;
	}

	fd = open(argv[optind], flag, 0644);
	if (fd < 0) {
		perror("open:");
		return -1;
	}

	ret = -1;
	if (ftrun_flag == 1) {
		ret = ftruncate(fd, file_size);
		if (ret != 0) {
			perror("ftruncate: \n");
			goto out;
		}
	}

	if (falloc_flag == 1) {
		ret = fallocate(fd, 0, 0, file_size);
		if (ret != 0) {
			perror("fallocate: \n");
			goto out;
		}
	}

	if (test_case == TST_BUFFER)
		buffer_write(fd, buf_size, file_size);
	else if (test_case == TST_MMAP_COPY)
		mmap_write(fd, buf_size, file_size, 1);
	else if (test_case == TST_MMAP)
		mmap_write(fd, buf_size, file_size, 0);
	else {
		printf("wrong test case %d.\n", test_case);
		goto out;
	}

	ret = 0;
out:
	close(fd);
	return ret;
}
