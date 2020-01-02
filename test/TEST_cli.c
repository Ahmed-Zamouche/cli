/**
 * @file TEST_cli.c
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
#include "TEST_cmd_list.h"
#include "cli.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static volatile int cmd_quit_flag = 0;

static struct TEST_cmd_output_s {
  char data[1024];
  size_t offset;
} TEST_cmd_output;

static void TEST_cmd_output_clear(struct TEST_cmd_output_s *output) {
  memset(output->data, 0, sizeof(output->data));
  output->offset = 0;
}

static void TEST_cmd_quit_handler(void) { cmd_quit_flag = 1; }

static size_t TEST_read(void *ptr, size_t size) {
  size_t n = fread(ptr, size, 1, stdin);
  return n;
}

static size_t TEST_write(const void *ptr, size_t size) {

  char *s = (char *)ptr;

  for (size_t i = 0; i < size; i++) {
    TEST_cmd_output.data[TEST_cmd_output.offset + i] = s[i];
  }
  TEST_cmd_output.offset += size;
  TEST_cmd_output.offset %= sizeof(TEST_cmd_output.data);

  return size;
}

static int TEST_flush(void) { return 0; }

void TEST_quit_cb(void) {}

int TEST_cli(int argc, char const *argv[]) {

  (void)argc;
  (void)argv;

  (void)TEST_read;

  cli_t cli;

  cli_init(&cli, NULL);

  cli.write = TEST_write;
  cli.flush = TEST_flush;

  cli_register_quit_callback(&cli, TEST_cmd_quit_handler);

  // 1. TEST build-in commands
  // 1.1 Test build-in quit command
  cmd_quit_flag = 0;
  cli_puts(&cli, "quit\n");
  cli_mainloop(&cli);
  assert(cmd_quit_flag == 1);

  // 1.2 Test build-in echo on/off command
  TEST_cmd_output_clear(&TEST_cmd_output);
  cli_puts(&cli, "echo off\r\n");
  cli_mainloop(&cli);
  assert(!strcmp(TEST_cmd_output.data, CLI_PROMPT ">echo off\r\n"));

  TEST_cmd_output_clear(&TEST_cmd_output);
  cli_puts(&cli, "echo on\r\n");
  cli_mainloop(&cli);
  assert(!strcmp(TEST_cmd_output.data, CLI_PROMPT ">\r\n"));

  // 1.3 Test build-in help command
  TEST_cmd_output_clear(&TEST_cmd_output);
  cli_puts(&cli, "help\r\n");
  cli_mainloop(&cli);
  assert(!memcmp(TEST_cmd_output.data, CLI_PROMPT ">help\r\nhelp\t",
                 strlen(CLI_PROMPT) + 12));

  // 2. TEST user defined commands
  cli_cmd_list_t *cmd_list = &TEST_cli_cmd_list;

  // 2.1 Test null groups
  cli.cmd_list = cmd_list;
  TEST_cmd_handler_flag = 0;
  cli_puts(&cli, "mcu reset\r\n");
  cli_mainloop(&cli);
  assert(TEST_cmd_handler_flag == 0);

  cli_cmd_group_t cli_cmd_mcu_group = {0};
  cli_cmd_group_t cli_cmd_gpio_group = {0};
  cli_cmd_group_t cli_cmd_adc_group = {0};
  // 2.2 Attach the first group
  cli_cmd_group_t *cli_cmd_group[ARRAY_SIZE(TEST_cli_cmd_group)] = {
      &cli_cmd_mcu_group, &cli_cmd_gpio_group, &cli_cmd_adc_group};
  cmd_list->groups = (const cli_cmd_group_t **)cli_cmd_group;
  cmd_list->length = 1;

  // name and description can not be null
  cli_cmd_group[0]->name = TEST_cli_cmd_group[0]->name;
  cli_cmd_group[0]->desc = TEST_cli_cmd_group[0]->desc;

  // 2.1.1 Test null cmds;
  cli_puts(&cli, "mcu reset\r\n");
  cli_mainloop(&cli);
  assert(TEST_cmd_handler_flag == 0);

  // 2.1.1 Attach the first cmd;
  cli_cmd_t cli_cmd_mcu_list[ARRAY_SIZE(TEST_cli_cmd_mcu_list)] = {0};
  cli_cmd_group[0]->cmds = cli_cmd_mcu_list;
  cli_cmd_group[0]->length = 1;

  // name and description can not be null
  cli_cmd_mcu_list->name = TEST_cli_cmd_mcu_list->name;
  cli_cmd_mcu_list->desc = TEST_cli_cmd_mcu_list->desc;

  // 2.1.1 Test default handler
  cli_cmd_mcu_list->handler = NULL;

  cli_puts(&cli, "mcu reset\r\n");
  cli_mainloop(&cli);
  assert(TEST_cmd_handler_flag == 0);

  // 2.1.1 Test handler
  cli_cmd_mcu_list->handler = TEST_cli_cmd_mcu_list->handler;
  cli_puts(&cli, "mcu reset\r\n");
  cli_mainloop(&cli);
  assert(TEST_cmd_handler_flag == 1);

  // 3. Test limits
  // 3.1 Test empty line
  TEST_cmd_handler_flag = 0;
  cli_puts(&cli, "\r\n");
  cli_mainloop(&cli);
  assert(TEST_cmd_handler_flag == 0);

  // 3.2 Test argv max limit
  char buf[CLI_LINE_MAX * 2] = {0};

  {
    int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "mcu reset");
    for (size_t i = 0; i < (CLI_ARGV_NUM - 2) + 1; i++) {
      n += snprintf(buf + n, sizeof(buf) - n, " arg%ld", i);
    }
    n += snprintf(buf + n, sizeof(buf) - n, "\r\n");
  }

  TEST_cmd_handler_flag = 0;
  cli_puts(&cli, buf);
  TEST_cmd_output_clear(&TEST_cmd_output);
  cli_mainloop(&cli);
  assert(TEST_cmd_handler_flag == 0);
  assert(!strcmp(
      TEST_cmd_output.data + strlen(CLI_PROMPT) + 1 + strlen(buf),
      "Error: The number of arguments exceeds maximum of CLI_ARGV_NUM\r\n"));
  {
    int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "mcu reset");
    for (size_t i = 0; i < (CLI_ARGV_NUM - 2); i++) {
      n += snprintf(buf + n, sizeof(buf) - n, " arg%ld", i);
    }
    n += snprintf(buf + n, sizeof(buf) - n, "\r\n");
  }
  cli_puts(&cli, buf);
  cli_mainloop(&cli);
  assert(TEST_cmd_handler_flag == 1);

  // 3.2 Test line max limit
  int n = 0;
  n += snprintf(
      buf + n, sizeof(buf) - n,
      "mcu reset "
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\r\n");
  TEST_cmd_handler_flag = 0;
  TEST_cmd_output_clear(&TEST_cmd_output);
  cli_puts(&cli, buf);
  cli_mainloop(&cli);
  assert(TEST_cmd_handler_flag == 0);
  // anything over \link CLI_LINE_MAX \endlink will be truncated
  assert(!strcmp(cli.argv[2],
                 "0123456789abcdef0123456789abcdef0123456789abcdef01234"));

  return 0;
}
