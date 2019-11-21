#include "cli.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const cli_default_prompt = CLI_PROMPT;

static const char *const CLI_MSG_CMD_OK = "ok\r\n";
static const char *const CLI_MSG_CMD_ERROR = "error\r\n";
static const char *const CLI_MSG_CMD_UNKNOWN = "unknown cmd\r\n";

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

static int cli_cmd_list_traverser(cli_t *cli,
                                  int (*cb)(cli_t *, const cli_cmd_group_t *,
                                            const cli_cmd_t *)) {

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

  if (argc > 1) {
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

  if (argc > 1) {
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

  { // make line lower case
    char *p = cli->line;

    for (; *p; ++p)
      *p = tolower(*p);
  }

  while ((token = strtok_r(NULL, " \t", &saveptr))) {

    // It is safe to cast to get rid of compiler warning.
    if (cli->argc >= (int)sizeof(cli->argv)) {
      return -1;
    }

    cli->argv[cli->argc++] = token;
  }
  cli->argv[cli->argc] = NULL;

  return cli->argc;
}

static size_t cli_getline(cli_t *cli) {

  if (cli->ptr == NULL) {
    cli->write(cli->prompt, strlen(cli->prompt));
    cli->flush();
    cli->ptr = cli->line;
    *cli->ptr = '\0';
  }

  while (!ringbuffer_is_empty(&cli->rb_inbuf)) {
    char ch;
    ringbuffer_get(&cli->rb_inbuf, (uint8_t *)&ch);
    switch (ch) {
    case '\n':
    case '\r':
      cli->write("\r\n", 2);
      cli->flush();
      cli->ptr = NULL;
      return strlen(cli->line);
      break;
    case 0x15: // CTRL-U
      while (cli->ptr != cli->line) {
        cli_echo(cli, "\b \b", 3);
        --cli->ptr;
      }
      *cli->ptr = '\0';
      break;
    case '\e': // ESC
      cli_echo(cli, "^[\r\n", 4);
      cli->ptr = NULL;
      return 0;
      break;
    case 0x7f:
      if (cli->ptr > cli->line) {
        *--cli->ptr = '\0';
        cli_echo(cli, "\b \b", 3);
      }
      break;
    default:
      if (cli->ptr < (cli->line + sizeof(cli->line) - 1) && isprint(ch)) {
        *cli->ptr++ = ch;
        *cli->ptr = '\0';
      }
      cli_echo(cli, &ch, 1);
      break;
    }
  }

  return 0;
}

int cli_putchar(cli_t *cli, int ch) {
  if (ringbuffer_put(&cli->rb_inbuf, ch)) {
    return -1;
  }
  return ch;
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
      if (cmd->handler(cli, cli->argc, cli->argv) == 0) {
        cli->write(CLI_MSG_CMD_OK, strlen(CLI_MSG_CMD_OK));
      } else {
        cli->write(CLI_MSG_CMD_ERROR, strlen(CLI_MSG_CMD_ERROR));
      }
      return CLI_CMD_LIST_TRV_QUIT;
    }
  }
  return CLI_CMD_LIST_TRV_NEXT;
}

void cli_register_cmd_quit_callback(cli_t *cli, void (*cmd_quit_cb)(void)) {
  cli->cmd_quit_cb = cmd_quit_cb;
}
void cli_mainloop(cli_t *cli) {

  if (!cli_getline(cli)) {
    return;
  }

  if (!cli_tokenize(cli)) {
    return;
  }

  for (size_t i = 0; i < ARRAY_SIZE(cli_default_cmd_list); i++) {

    if (!strcmp(cli->argv[0], cli_default_cmd_list[i].name)) {

      cli_default_cmd_list[i].handler(cli, cli->argc, cli->argv);
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

#undef CLI_CMD_LIST_TRV_NEXT
#undef CLI_CMD_LIST_TRV_SKIP
#undef CLI_CMD_LIST_TRV_QUIT