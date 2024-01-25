#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAX_LEN 32

int read_line(char *buf, int max) {
  // return the characters of this line
  int i;
  int cc;
  for (i = 0; i < max - 1; i++) {
    char c;
    cc = read(0, &c, 1);
    if (cc < 1) {
      // read failed
      break;
    }
    buf[i] = c;
    if (c == '\n') {
      buf[i] = 0;
      break;
    }
  }

  return i;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("xargs usage: xargs command (args)...\n");
    exit(0);
  }
  char buffer[MAXARG][MAX_LEN];
  int lines = 0;

  // copy argv first
  for (int i = 1; i < argc; i++) {
    memcpy(buffer[lines++], argv[i], MAX_LEN);
  }

  // read from stdin to get args
  while (read_line(buffer[lines++], MAX_LEN)) {
    if (lines >= MAXARG) {
      printf("xargs: argv over MAXARG\n");
      exit(0);
    }
  }

  if (fork() == 0) {
    // child process
    char *argv[lines+1];
    for (int i = 0; i < lines; i++) {
      argv[i] = (char *)malloc(sizeof(char) * MAX_LEN);
      memcpy(argv[i], buffer[i], MAX_LEN);
    }
    argv[lines] = 0;
    exec(argv[0], argv);
    printf("xargs: failed to execute program\n");
    exit(0);
  } else {
    // parent process
    wait(0);
  }

  exit(0);
}
