#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

void doWrite(int fd, const char *buff, int len){
	ssize_t wcnt;
	wcnt = write(fd, buff, len);	/*write file*/
	if (wcnt == -1){	/* error message*/
		perror("write");
		close(fd);
		exit(1);
	}
}

void write_file(int fd, const char *infile){
	int fd2;
	char buff[1024];	/*Make a buffer for reading file*/
	fd2 = open(infile, O_RDONLY);
	if (fd2 == -1){	/*error message and close file*/
	       	perror(infile);
        	exit(1);
	}
	int small=1023;
	while (small > 0){
		small=read(fd2,buff, small);	/*read file*/
		if (small == -1){	/* error */
			perror(infile);
			close(fd2);
			exit(1);
		}
		doWrite(fd, buff, small);	 /*write file*/
	}
	close(fd2);	/*close file*/
}
