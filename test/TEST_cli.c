
#include "cli.h"

#include <stdio.h>

static size_t my_read(void *ptr, size_t size) {
  size_t n = fread(ptr, size, 1, stdin);
  return n;
}

static size_t my_write(const void *ptr, size_t size) {
  return fwrite(ptr, size, 1, stdout);
}

static int my_flush(void) { return fflush(stdout); }

int TEST_cli(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;

  (void)my_read;
  (void)my_write;
  (void)my_flush;

  return 0;
}
