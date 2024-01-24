#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PARENT_SND_BYTE 5
#define CHILD_SND_BYTE 6

int main(int argc, const char *argv[]) {
  int p[2];
  pipe(p);

  if (fork() == 0) {
    // child process
    char byte = 254;
    read(p[0], &byte, 1);
    if (byte != PARENT_SND_BYTE) {
      printf("child failed to receive byte\n");
      printf("expected: %d, but got: %d\n", PARENT_SND_BYTE, byte);
      exit(0);
    }

    int pid = getpid();
    printf("%d: received ping\n", pid);
    byte = CHILD_SND_BYTE;
    write(p[1], &byte, 1);
    exit(0);
  }

  // parent process
  char byte = PARENT_SND_BYTE;
  write(p[1], &byte, 1);
  wait((int*)0);
  read(p[0], &byte, 1);
  if (byte != CHILD_SND_BYTE) {
    printf("parent failed to receive byte\n");
    printf("expected: %d, but got: %d\n", CHILD_SND_BYTE, byte);
    exit(0);
  }
  
  int pid = getpid();
  printf("%d: received pong\n", pid);
  exit(0);
}
