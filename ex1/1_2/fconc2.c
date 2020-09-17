#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void doWrite(int fd, const char *buff, int len) {
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
  if (argc < 3) {
    fprintf(stderr, "Usage: ./fconc infile1 ... infile(n) [outfile (default:fconc.out)]\n");
    exit(1);
  }
  fd = open(argv[argc-1], oflags, mode);
  if (fd  == -1) {
      perror(argv[argc-1]);
      exit(1);
  }
  int i;
  for (i = 1; i < argc-1; i++) {
      if (argv[i] == argv[argc-1]) {
	fprintf(stderr, "[-] Please believe me you don't want to write to file you read from (: %s)\n", argv[argc-1]);
	exit(1);
	}
  }
  for (i = 1; i < argc - 1; i++) {   
      write_file(fd, argv[i]);
  }
  close(fd);
  return 0;
}

