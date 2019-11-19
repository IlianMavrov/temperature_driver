#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int8_t read_buf[4];
int main()
{
	int fd;
	char option;
	printf("*********************************\n");

	fd = open("/dev/tmp100_device", O_RDWR);
	if(fd < 0) {
		printf("Cannot open device file...\n");
		return 0;
	}

	while(1) {
		printf("****Please Enter the Option******\n");
		printf("        1. Read                 \n");
		printf("        2. Exit                 \n");
		printf("*********************************\n");
		scanf(" %c", &option);
		printf("Your Option = %c\n", option);

	switch(option) {
		case '1':
			printf("Data Reading ...");
			read(fd, read_buf, 4);
			printf("Done!\n\n");
			printf("Data = %s\n\n", read_buf);
			break;
		case '2':
			close(fd);
			exit(1);
			break;
		default:
			printf("Enter Valid option = %c\n",option);
			break;
		}
	}
close(fd);
}

