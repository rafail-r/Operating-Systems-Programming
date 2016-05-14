#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void doWrite(int, const char*, int);
void write_file(int, const char*);

int main(int argc, char **argv) {
	if ((argc<3) || (argc>4)) {	/*not valid input*/
                printf("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)] \n");
		return -1; 
	}
	int fd;
	int oflags = O_WRONLY | O_CREAT | O_TRUNC;
        int mode = S_IRUSR | S_IWUSR;
	if (argc==4) {	/*if there is file for output*/
		fd=open(argv[3], oflags, mode);
		if (fd == -1){
			perror(argv[3]);
			return -1;
		}
	}
	else{
		fd=open("fconc.out", oflags, mode);
		if (fd == -1){
                        perror("fconc.out");
                        return -1; 
		}	/*default output file*/
	}
	write_file(fd,argv[1]);	/*write infile 1 to output file*/
	write_file(fd,argv[2]);	/*write infile 1 to output file*/
	close(fd);
	return 0;
}
