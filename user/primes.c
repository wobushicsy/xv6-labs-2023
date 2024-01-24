#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, const char *argv[]) {
  close(0);
  close(2);

  int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
  int size = sizeof(primes) / sizeof(int);
  int p[2];

  pipe(p);

  if (fork() == 0) {
    // child process
    // generate pipeline
    for (int i = 0; i < size; i++) {
      int read_fd = p[0];
      int write_fd = p[1];
      int prime = primes[i];
      if (pipe(p) != 0) {
        printf("pipe error\n");
        exit(1);
      }

      int pid = fork();
      if (pid < 0) {
        printf("fork error\n");
        exit(1);
      } else if (pid == 0) {
        // child process
        close(write_fd);
      } else {
        // parent process
        close(write_fd);
        write_fd = p[1];
        int num;
        while (read(read_fd, &num, sizeof(int))) {
          if (num == prime) {
            printf("prime %d\n", num);
          }
          if (num % prime == 0) {
            continue;
          }
          write(write_fd, &num, sizeof(int));
        }
        close(write_fd);
        close(read_fd);
        wait(0);
        exit(0);
      }
    }
  } else {
    // parent process
    // generate nums
    int read_fd = p[0];
    int write_fd = p[1];
    close(read_fd);
    for (int i = 2; i <= 35; i++) {
      write(write_fd, &i, sizeof(int));
    }
    close(write_fd);
    wait(0);
  }

  exit(0);
}
