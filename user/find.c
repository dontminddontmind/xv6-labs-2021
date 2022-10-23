#include "kernel/types.h"

#include "kernel/stat.h"
#include "user/user.h"

#include "kernel/fs.h"

void find(char *path, char *name) {

  char buf[512];
  int fd;
  struct stat st;
  struct dirent de;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "open %s error\n", path);
    return;
  }
  if ((stat(path, &st)) < 0) {
    fprintf(2, "stat %s error\n", path);
    close(fd);
    return;
  }

  char *filename;
  switch (st.type) {
  case T_FILE:
    for (filename = path + strlen(path) - 1;
         filename >= path && (*filename != '/'); filename--)
      ;
    filename++;
    if (!strcmp(filename, name))
      printf("%s", path);
    break;
  case T_DIR:
    if (strlen(path) + 1 > sizeof buf) {
      fprintf(2, "path too long\n");
      break;
    }
    strcpy(buf, path);
    filename = buf + strlen(path);
    *filename++ = '/';
    *filename = '\0';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      // printf("test %s\n", de.name);
      if ((!strcmp(de.name, ".")) || (!strcmp(de.name, "..")))
        continue;
      if (strlen(de.name) + (filename - buf) + 1 > sizeof(buf)) {
        fprintf(2, "path to long\n");
        continue;
      }
      strcpy(filename, de.name);
      if (stat(buf, &st) < 0) {
        fprintf(2, "cannot stat %s\n", buf);
        continue;
      }
      if (st.type == T_DIR) {
        find(buf, name);
      } else {
        if (!strcmp(de.name, name))
          printf("%s\n", buf);
      }
    }

    break;
  }
}
int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(2, "find path name\n");
    exit(1);
  }
  find(argv[1], argv[2]);

  exit(0);
}
