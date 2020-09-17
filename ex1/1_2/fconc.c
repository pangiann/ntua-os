#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void doWrite(int fd, const char *buff, int len){
  size_t idx;
  ssize_t wcnt;
  idx = 0;
  do {
    wcnt = write(fd, buff + idx, len - idx);
    if (wcnt == -1) {
       perror("write");
       exit(1);
    }
    idx += wcnt;
  } while(idx < len);
}

void write_file(int fd, const char *infile){
  char buff[1024];
  ssize_t rcnt;
  int fdin = open(infile, O_RDONLY);
  if (fdin == -1){
    perror(infile);
    exit(1);
  }
  while ((rcnt = read(fdin, buff, sizeof(buff) - 1)) > 0) {
        if (rcnt == -1) { /* error */
            perror("read");
            exit(1);
        }
  	buff[rcnt] = '\0';
  	doWrite(fd, buff, strlen(buff));
  }
  close(fdin);
}

int main(int argc, char **argv)
{
  int fd, oflags = O_CREAT | O_WRONLY | O_TRUNC, mode = S_IRUSR | S_IWUSR;
  if (argc < 3 || argc > 4) {
    fprintf(stderr, "Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n");
    exit(1);
  }
  if (argc == 3) {
      fd = open("fconc.out", oflags, mode);
      if (fd == -1) {
          perror("fconc.out");
	        exit(1);
      }
  }
  else {
    printf("agv = %s", argv[3]);
    printf("agv = %s", argv[1]);

    if (!strcmp(argv[1], argv[3]) || !strcmp(argv[2], argv[3])) {
  	   perror("You shouldn't write where you read\n");
  	   exit(1);
    }
    fd = open(argv[3], oflags, mode);
    if (fd  == -1) {
	      perror(argv[3]);
        exit(1);
    }
  }

  write_file(fd, argv[1]);
  write_file(fd, argv[2]);
  close(fd);

  return 0;
}
