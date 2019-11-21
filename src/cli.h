#ifndef _CLI_H
#define _CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLI_INPUT_BUF_SIZE
#define CLI_INPUT_BUF_SIZE (64)
#endif

#include "ringbuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CLI_PROMPT
#define CLI_PROMPT "ucli>"
#endif /*CLI_PROMPT*/

#ifndef CLI_IN_BUF_MAX
#define CLI_IN_BUF_MAX (128)
#endif /*CLI_IN_BUF_MAX*/

#ifndef CLI_LINE_MAX
#define CLI_LINE_MAX (64)
#endif /*CLI_LINE_MAX*/

#ifndef CLI_ARGV_NUM
#define CLI_ARGV_NUM (8)
#endif /*CLI_ARGV_NUM*/

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif /*ARRAY_SIZE*/

typedef struct cli_s cli_t;

typedef int (*cli_cmd_handler_t)(cli_t *cli, int argc, char **argv);

typedef struct cli_cmd_s {
  char *name;
  char *desc;
  cli_cmd_handler_t handler;
} cli_cmd_t;

typedef struct cli_cmd_group_s {
  char *name;
  char *desc;
  const cli_cmd_t *cmds;
  size_t length;
} cli_cmd_group_t;

typedef struct cli_cmd_list_s {
  const cli_cmd_group_t *groups;
  size_t length;
} cli_cmd_list_t;

struct cli_s {
  bool echo;

  char *ptr;

  char inbuf[CLI_IN_BUF_MAX];
  char line[CLI_LINE_MAX];

  int argc;
  char *argv[CLI_ARGV_NUM];

  ringbuffer_t rb_inbuf;

  size_t (*write)(const void *ptr, size_t size);

  int (*flush)(void);

  void (*cmd_quit_cb)(void);

  char const *prompt;

  const cli_cmd_list_t *cmd_list;
};

int cli_putchar(cli_t *cli, int ch);

void cli_register_quit_callback(cli_t *cli, void (*)(void));

void cli_mainloop(cli_t *cli);

void cli_init(cli_t *cli, const cli_cmd_list_t *cmd_list);

#ifdef __cplusplus
}
#endif

#endif /* _CLI_H */