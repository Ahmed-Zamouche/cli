/**
 * @file cli.c
 * @author Ahmed Zamouche (ahmed.zamouche@gmail.com)
 * @brief
 * @version 0.1
 * @date 2019-12-01
 *
 *  @copyright Copyright (c) 2019
 *
 * MIT License
 *
 * Copyright (c) 2019 Ahmed Zamouche
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "cli.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLI_CMD_LIST_TRV_NEXT (0)
#define CLI_CMD_LIST_TRV_SKIP (1)
#define CLI_CMD_LIST_TRV_END (2)

static int cli_cmd_echo(cli_t *cli, int argc, char **argv);
static int cli_cmd_help(cli_t *cli, int argc, char **argv);
static int cli_cmd_quit(cli_t *cli, int argc, char **argv);

static const char *const cli_default_prompt = CLI_PROMPT;
static const char *const CLI_MSG_CMD_OK = "Ok\r\n";
static const char *const CLI_MSG_CMD_ERROR = "Error\r\n";
static const char *const CLI_MSG_NUM_ARG_ERR =
    "Error: The number of arguments exceeds maximum of CLI_ARGV_NUM\r\n";
static const char *const CLI_MSG_LINE_LENGTH_ERR =
    "Error: The line length exceeds maximum of CLI_LINE_MAX\r\n";
static const char *const CLI_MSG_CMD_UNKNOWN = "Unknown command\r\n";

/**
 * @brief default command line interpreter write function
 *
 * @param ptr pinter to the buffer to be written
 * @param size number of bytes to write
 * @return size_t On success, return the number of items read or written.
 * Otherwise, less than size or 0 is returned
 */
static size_t cli_default_write(const void *ptr, size_t size) {
  return fwrite(ptr, size, 1, stdout);
}

/**
 * @brief default quit callback function if none is registred
 *
 * @return int  Upon successful completion 0 is returned.  Otherwise, -1 is
 * returned
 */
static int cli_default_flush(void) { return fflush(stdout); }

/**
 * @brief default quit callback function if none is registred
 *
 */
static void cli_cmd_quit_default_cb(void) { exit(0); }

/**
 * @brief build-in default command list
 *
 */
static const cli_cmd_t cli_default_cmd_list[] = {
    {.name = "help", .desc = "Print this help", .handler = cli_cmd_help},
    {.name = "echo",
     .desc = "(on|off). Turn echoing On or Off",
     .handler = cli_cmd_echo},
    {.name = "quit",
     .desc = "Quit command line interpreter",
     .handler = cli_cmd_quit},
};

/**
 * @brief default command handler if none is attached
 *
 * @param cli the command line interpreter struct
 * @param argc arguments count
 * @param argv arguments vector
 * @return int On success 0 is return. Otherwise non zero value
 */
static int cli_cmd_default_handler(cli_t *cli, int argc, char **argv) {
  (void)cli;
  (void)argc;
  (void)argv;
  return 0;
}

/**
 * @brief Traverse the commands list and callback the caller for every group and
 * command using cb. cb function takes 3 parameters: Pointer to the command
 * line interpreter struct, pointer to command group struct and pointer to the
 * command struct. for every new group found the command struct argument is set
 * to NULL. cb return \link CLI_CMD_LIST_TRV_NEXT \endlink to indicated to
 * traverser to continue traversing,  \link CLI_CMD_LIST_TRV_SKIP \endlink to
 * skip the current group \link CLI_CMD_LIST_TRV_END \endlink to end traversing.
 * @param cli the command line interpreter struct
 * @param cb callback function
 * @return int 0 if the whole list was traversed, 1 if the traversing ended at a
 * group or 2  if the traversing ended at a command
 */
static int cli_cmd_list_traverser(cli_t *cli,
                                  int (*cb)(cli_t *, const cli_cmd_group_t *,
                                            const cli_cmd_t *)) {

  if (cli->cmd_list == NULL || cli->cmd_list->groups == NULL) {
    return 0;
  }

  for (size_t i = 0; i < cli->cmd_list->length; i++) {

    const cli_cmd_group_t *group = cli->cmd_list->groups[i];

    int ret = cb(cli, group, NULL);

    if (ret == CLI_CMD_LIST_TRV_SKIP) {
      continue;
    } else if (ret == CLI_CMD_LIST_TRV_END) {
      return 1;
    } else {
      ;
    }

    if (group->cmds == NULL) {
      continue;
    }
    for (size_t j = 0; j < group->length; j++) {

      const cli_cmd_t *cmd = &group->cmds[j];

      ret = cb(cli, group, cmd);
      if (ret == CLI_CMD_LIST_TRV_SKIP) {
        continue;
      } else if (ret == CLI_CMD_LIST_TRV_END) {
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

/**
 * @brief build-in help command handler
 *
 * @param cli the command line interpreter struct
 * @param argc arguments count
 * @param argv arguments vector
 * @return int On success 0 is return. Otherwise non zero value
 */
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

/**
 * @brief build-in echo command handler
 *
 * @param cli the command line interpreter struct
 * @param argc arguments count
 * @param argv arguments vector
 * @return int On success 0 is return. Otherwise non zero value
 */
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

/**
 * @brief build-in quit command handler
 *
 * @param cli the command line interpreter struct
 * @param argc arguments count
 * @param argv arguments vector
 * @return int On success 0 is return. Otherwise non zero value
 */
static int cli_cmd_quit(cli_t *cli, int argc, char **argv) {

  (void)cli;
  (void)argv;

  if (argc != 1) {
    return -1;
  }

  cli->cmd_quit_cb();

  return 0;
}

/**
 * @brief write buffer pointed to by ptr of size size to the command line
 * interpreter write to output function. If the echoing is not enable nothing is
 * written
 * @param cli the command line interpreter struct
 * @param ptr pointer to buffer to be written
 * @param size number of byte to be written
 */
static void cli_echo(cli_t *cli, const void *ptr, size_t size) {
  if (cli->echo) {
    cli->write(ptr, size);
    cli->flush();
  }
}

/**
 * @brief tokenise the command line interpreter line using space and tab as
 * token delimiter
 *
 * @param cli the command line interpreter struct
 * @return int number of tokens found. -1 if number of token exceeded  \link
 * CLI_ARGV_NUM \endlink
 */
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

void cli_print_prompt(cli_t *cli) {
  cli->write(cli->prompt, strlen(cli->prompt));
  cli->write(">", 1);
  cli->flush();
}

/**
 * @brief read bytes from the receive buffer and add them to the line buffer.
 * Only printing character are added, If newline delimiter is found the the
 * function return the strlen of line
 * @param cli the command line interpreter struct
 * @return size_t strlen of the line. 0 if the receive buffer was emptied before
 * detecting a new line
 */
static size_t cli_getline(cli_t *cli) {

  if (cli->ptr == NULL) {
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
      size_t len = strlen(cli->line);
      if (len == 0) {
        cli_print_prompt(cli);
      }
      return len;
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
    case '\b': // <-
    case 0x7f:
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
          cli_print_prompt(cli);
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

      return CLI_CMD_LIST_TRV_END;
    }
  }
  return CLI_CMD_LIST_TRV_NEXT;
}

void cli_register_quit_callback(cli_t *cli, void (*cmd_quit_cb)(void)) {
  cli->cmd_quit_cb = cmd_quit_cb ? cmd_quit_cb : cli_cmd_quit_default_cb;
}

void cli_mainloop(cli_t *cli) {

  if (cli_getline(cli) == 0) {
    return;
  }

  if (cli_tokenize(cli) < 0) {
    cli->write(CLI_MSG_NUM_ARG_ERR, strlen(CLI_MSG_NUM_ARG_ERR));
    goto cli_mainloop_exit;
  }

  if (cli->argc == 0) {
    goto cli_mainloop_exit;
  }

  for (size_t i = 0; i < ARRAY_SIZE(cli_default_cmd_list); i++) {

    if (!strcmp(cli->argv[0], cli_default_cmd_list[i].name)) {
      if (cli_default_cmd_list[i].handler(cli, cli->argc, cli->argv) == 0) {
        cli->write(CLI_MSG_CMD_OK, strlen(CLI_MSG_CMD_OK));
      } else {
        cli->write(CLI_MSG_CMD_ERROR, strlen(CLI_MSG_CMD_ERROR));
      }
      goto cli_mainloop_exit;
    }
  }

  if (cli->argc < 2 || !cli_cmd_list_traverser(cli, cli_cmd_run_traverser_cb)) {
    cli->write(CLI_MSG_CMD_UNKNOWN, strlen(CLI_MSG_CMD_UNKNOWN));
  }
cli_mainloop_exit:
  cli_print_prompt(cli);
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
