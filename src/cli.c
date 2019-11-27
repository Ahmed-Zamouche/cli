#include "cli.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const cli_default_prompt = CLI_PROMPT;

static const char *const CLI_MSG_CMD_OK = "Ok\r\n";
static const char *const CLI_MSG_CMD_ERROR = "Error\r\n";
static const char *const CLI_MSG_NUM_ARG_ERR =
    "Error: The number of arguments exceeds maximum of CLI_ARGV_NUM\r\n";
static const char *const CLI_MSG_LINE_LENGTH_ERR =
    "Error: The line length exceeds maximum of CLI_LINE_MAX\r\n";
static const char *const CLI_MSG_CMD_UNKNOWN = "Unknown command\r\n";

static size_t cli_default_write(const void *ptr, size_t size) {
  return fwrite(ptr, size, 1, stdout);
}

static int cli_default_flush(void) { return fflush(stdout); }

static void cli_cmd_quit_default_cb(void) { exit(0); }

static int cli_cmd_echo(cli_t *cli, int argc, char **argv);
static int cli_cmd_help(cli_t *cli, int argc, char **argv);
static int cli_cmd_quit(cli_t *cli, int argc, char **argv);

static const cli_cmd_t cli_default_cmd_list[] = {
    {.name = "help", .desc = "Print this help", .handler = cli_cmd_help},
    {.name = "echo",
     .desc = "(on|off). Turn echoing On or Off",
     .handler = cli_cmd_echo},
    {.name = "quit",
     .desc = "Quit command line interpreter",
     .handler = cli_cmd_quit},
};

#define CLI_CMD_LIST_TRV_NEXT (0)
#define CLI_CMD_LIST_TRV_SKIP (1)
#define CLI_CMD_LIST_TRV_QUIT (2)

static int cli_cmd_default_handler(cli_t *cli, int argc, char **argv) {
  (void)cli;
  (void)argc;
  (void)argv;
  return 0;
}

static int cli_cmd_list_traverser(cli_t *cli,
                                  int (*cb)(cli_t *, const cli_cmd_group_t *,
                                            const cli_cmd_t *)) {

  if (cli->cmd_list == NULL || cli->cmd_list->groups == NULL) {
    return 0;
  }

  for (size_t i = 0; i < cli->cmd_list->length; i++) {

    const cli_cmd_group_t *group = &cli->cmd_list->groups[i];

    int ret = cb(cli, group, NULL);

    if (ret == CLI_CMD_LIST_TRV_SKIP) {
      continue;
    } else if (ret == CLI_CMD_LIST_TRV_QUIT) {
      return 1;
    } else {
      ;
    }

    if (group->cmds == NULL) {
      continue;
    }
    for (size_t j = 0; j < group->length; j++) {

      const cli_cmd_t *cmd = &group->cmds[j];

      int ret = cb(cli, group, cmd);
      if (ret == CLI_CMD_LIST_TRV_SKIP) {
        continue;
      } else if (ret == CLI_CMD_LIST_TRV_QUIT) {
        return 2;
      } else {
        ;
      }
    }
  }
  return 0;
}

static int cli_cmd_help_traverser_cb(cli_t *cli, const cli_cmd_group_t *group,
                                     const cli_cmd_t *cmd) {
  const char *name;
  const char *desc;
  if (group && !cmd) {
    name = group->name;
    desc = group->desc;
    cli->write("\r\n", 2);
  } else {
    name = cmd->name;
    desc = cmd->desc;
    cli->write(" ", 1);
  }

  cli->write(name, strlen(name));
  cli->write("\t", 1);
  cli->write(desc, strlen(desc));
  cli->write("\r\n", 2);

  return CLI_CMD_LIST_TRV_NEXT;
}

static int cli_cmd_help(cli_t *cli, int argc, char **argv) {

  (void)argv;

  if (argc > 3) {
    return -1;
  }

  for (size_t i = 0; i < ARRAY_SIZE(cli_default_cmd_list); i++) {
    cli->write(cli_default_cmd_list[i].name,
               strlen(cli_default_cmd_list[i].name));
    cli->write("\t", 1);
    cli->write(cli_default_cmd_list[i].desc,
               strlen(cli_default_cmd_list[i].desc));
    cli->write("\r\n", 2);
  }

  cli_cmd_list_traverser(cli, cli_cmd_help_traverser_cb);

  return 0;
}

static int cli_cmd_echo(cli_t *cli, int argc, char **argv) {

  if (argc != 2) {
    return -1;
  }

  if (!strcmp("on", argv[1])) {
    cli->echo = true;
  } else if (!strcmp("off", argv[1])) {
    cli->echo = false;
  } else {
    return -2;
  }

  return 0;
}

static int cli_cmd_quit(cli_t *cli, int argc, char **argv) {

  (void)cli;
  (void)argv;

  if (argc != 1) {
    return -1;
  }

  cli->cmd_quit_cb();

  return 0;
}

static void cli_echo(cli_t *cli, const void *ptr, size_t size) {
  if (cli->echo) {
    cli->write(ptr, size);
    cli->flush();
  }
}

static int cli_tokenize(cli_t *cli) {

  char *token;
  char *saveptr = cli->line;

  cli->argc = 0;

  while ((token = strtok_r(NULL, " \t", &saveptr))) {

    if ((size_t)cli->argc >= ARRAY_SIZE(cli->argv)) {
      return -1;
    }

    cli->argv[cli->argc++] = token;
  }
  return cli->argc;
}

static size_t cli_getline(cli_t *cli) {

  if (cli->ptr == NULL) {
    cli->write(cli->prompt, strlen(cli->prompt));
    cli->write(">", 1);
    cli->flush();
    cli->ptr = cli->line;
    *cli->ptr = '\0';
  }

  char ch;
  while (!ringbuffer_get(&cli->rb_inbuf, (uint8_t *)&ch)) {

    switch (ch) {
    case '\r':
    case '\n':
      while (!ringbuffer_peek(&cli->rb_inbuf, (uint8_t *)&ch)) {
        if (ch != '\r' && ch != '\n') {
          break;
        }
        ringbuffer_get(&cli->rb_inbuf, (uint8_t *)&ch);
      }
      cli->write("\r\n", 2);
      cli->flush();
      cli->ptr = NULL;
      return strlen(cli->line);
    case 0x15: // CTRL-U
      while (cli->ptr != cli->line) {
        cli_echo(cli, "\b \b", 3);
        --cli->ptr;
      }
      *cli->ptr = '\0';
      break;
    case '\e': // ESC
      cli_echo(cli, "^[ \r\n", 5);
      cli->ptr = cli->line;
      *cli->ptr = '\0';
      break;
    case 0x7f: // <-
      if (cli->ptr > cli->line) {
        *--cli->ptr = '\0';
        cli_echo(cli, "\b \b", 3);
      }
      break;
    default:
      if (isprint(ch)) {
        if (cli->ptr < (cli->line + sizeof(cli->line) - 1)) {
          *cli->ptr++ = tolower(ch);
          *cli->ptr = '\0';
        } else {
          cli->write("\r\n", 2);
          cli->write(CLI_MSG_LINE_LENGTH_ERR, strlen(CLI_MSG_LINE_LENGTH_ERR));
          cli->ptr = NULL;
          return 0;
        }
      }
      cli_echo(cli, &ch, 1);
      break;
    }
  }

  return 0;
}

int cli_putchar(cli_t *cli, int ch) {
  return ringbuffer_put(&cli->rb_inbuf, ch) ? -1 : ch;
}

int cli_puts(cli_t *cli, const char *str) {
  const char *p = str;

  while (*p) {
    if (cli_putchar(cli, *p) < 0) {
      return -1;
    }
    p++;
  }

  return 0;
}

static int cli_cmd_run_traverser_cb(cli_t *cli, const cli_cmd_group_t *group,
                                    const cli_cmd_t *cmd) {

  if (group && !cmd) {
    if (strcmp(cli->argv[0], group->name)) {
      return CLI_CMD_LIST_TRV_SKIP;
    }
  } else {
    if (strcmp(cli->argv[1], cmd->name)) {
      return CLI_CMD_LIST_TRV_SKIP;
    } else {

      cli_cmd_handler_t handler =
          cmd->handler ? cmd->handler : cli_cmd_default_handler;

      if (handler(cli, cli->argc, cli->argv) == 0) {
        cli->write(CLI_MSG_CMD_OK, strlen(CLI_MSG_CMD_OK));
      } else {
        cli->write(CLI_MSG_CMD_ERROR, strlen(CLI_MSG_CMD_ERROR));
      }

      return CLI_CMD_LIST_TRV_QUIT;
    }
  }
  return CLI_CMD_LIST_TRV_NEXT;
}

void cli_register_quit_callback(cli_t *cli, void(cmd_quit_cb)(void)) {
  cli->cmd_quit_cb = cmd_quit_cb ? cmd_quit_cb : cli_cmd_quit_default_cb;
}

void cli_mainloop(cli_t *cli) {

  if (cli_getline(cli) == 0) {
    return;
  }

  if (cli_tokenize(cli) < 0) {
    cli->write(CLI_MSG_NUM_ARG_ERR, strlen(CLI_MSG_NUM_ARG_ERR));
    return;
  }

  if (cli->argc == 0) {
    return;
  }

  for (size_t i = 0; i < ARRAY_SIZE(cli_default_cmd_list); i++) {

    if (!strcmp(cli->argv[0], cli_default_cmd_list[i].name)) {

      cli_default_cmd_list[i].handler(cli, cli->argc, cli->argv);
      return;
    }
  }

  if (cli->argc < 2 || !cli_cmd_list_traverser(cli, cli_cmd_run_traverser_cb)) {
    cli->write(CLI_MSG_CMD_UNKNOWN, strlen(CLI_MSG_CMD_UNKNOWN));
  }
  return;
}

void cli_init(cli_t *cli, const cli_cmd_list_t *cmd_list) {

  ringbuffer_wrap(&cli->rb_inbuf, (uint8_t *)cli->inbuf, sizeof(cli->inbuf));

  cli->echo = true;
  cli->ptr = NULL;

  cli->prompt = cli_default_prompt;
  cli->write = cli_default_write;
  cli->flush = cli_default_flush;

  cli->cmd_quit_cb = cli_cmd_quit_default_cb;

  cli->cmd_list = cmd_list;
}
