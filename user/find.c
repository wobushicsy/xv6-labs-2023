#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char *fmtname(char *path) {
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), 0, DIRSIZ-strlen(p));
  return buf;
}

void find(char *path, char *name) {
  int fd;
  struct stat st;
  struct dirent de;
  
  char buf[512];
  char *p;

  // open path as a file
  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  // get file status
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
    case T_DEVICE:
    case T_FILE:
    // if the type of this file is device or file, we compare it with target file
    if (strcmp(fmtname(path), name) == 0) {
      printf("%s/%s", path, name);
    }
    break;

    // if the type of this file is a directory, we may recursively search the directory
    case T_DIR:
    // recursively search
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }

    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      // eliminate . and .. dir cases
      if (strcmp(p, ".") == 0) {
        continue;
      }
      if (strcmp(p, "..") == 0) {
        continue;
      }

      if (stat(buf, &st) < 0) {
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      switch (st.type) {
        case T_DEVICE:
        case T_FILE:
          if (strcmp(fmtname(buf), name) == 0) {
            printf("%s/%s\n", path, name);
          }
          break;

        case T_DIR:
          find(buf, name);
          break;
      }
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("usage: find path name...\n");
    exit(0);
  }

  find(argv[1], argv[2]);
  exit(0);
}
