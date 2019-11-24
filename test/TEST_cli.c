
#include "cli.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static volatile int cmd_quit_flag = 0;
static volatile int cmd_handler_flag = 0;

static struct TEST_cmd_output_s {
  char data[1024];
  size_t offset;
} TEST_cmd_output;

static void TEST_cmd_output_clear(struct TEST_cmd_output_s *output) {
  memset(output->data, 0, sizeof(output->data));
  output->offset = 0;
}

static int TEST_cli_cmd_handler(cli_t *cli, int argc, char **argv);

static const cli_cmd_t TEST_cli_cmd_mcu_list[] = {

    {
        .name = "reset",
        .desc = "[NUM]. Reset the mcu after NUM seconds",
        .handler = TEST_cli_cmd_handler,
    },
    {
        .name = "sleep",
        .desc = "[NUM]. Put mcu in sleep mode for NUM seconds",
        .handler = TEST_cli_cmd_handler,
    },

};

static const cli_cmd_t TEST_cli_cmd_gpio_list[] = {
    {
        .name = "input-get",
        .desc = "NAME. Get gpio input NAME value",
        .handler = TEST_cli_cmd_handler,
    },
    {
        .name = "output-get",
        .desc = "NAME. Get gpio output NAME value",
        .handler = TEST_cli_cmd_handler,
    },
    {
        .name = "output-set",
        .desc = "NAME (0|1). Set gpio output NAME",
        .handler = TEST_cli_cmd_handler,
    },
};

static const cli_cmd_t TEST_cli_cmd_adc_list[] = {
    {
        .name = "get",
        .desc = "NAME. Get adc NAME value",
        .handler = TEST_cli_cmd_handler,
    },
    {
        .name = "start-conv",
        .desc = "NAME. Start acd NAME conversion",
        .handler = TEST_cli_cmd_handler,
    },
};

static const cli_cmd_group_t TEST_cli_cmd_group[] = {
    {.name = "mcu", .desc = "MCU group", .cmds = NULL, .length = 0},
    {.name = "gpio", .desc = "Gpio group", .cmds = NULL, .length = 0},
    {.name = "adc", .desc = "ADC group", .cmds = NULL, .length = 0},
};

static cli_cmd_list_t TEST_cli_list = {
    .groups = NULL,
    .length = 0,
};

static int TEST_cli_cmd_handler(cli_t *cli, int argc, char **argv) {
  (void)cli;
  (void)argc;
  (void)argv;

  cli->write("cmd: ", 5);
  for (int i = 0; i < argc; i++) {
    cli->write("`", 1);
    cli->write(argv[i], strlen(argv[i]));
    cli->write("`, ", 2);
  }
  cli->write("\r\n", 2);

  cmd_handler_flag = 1;

  return 0;
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

  (void)TEST_cli_cmd_mcu_list;
  (void)TEST_cli_cmd_gpio_list;
  (void)TEST_cli_cmd_adc_list;

  (void)TEST_cli_cmd_group;

  (void)TEST_cli_list;

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
  cli_cmd_list_t *cmd_list = &TEST_cli_list;

  // 2.1 Test null groups
  cli.cmd_list = cmd_list;
  cmd_handler_flag = 0;
  cli_puts(&cli, "mcu reset\r\n");
  cli_mainloop(&cli);
  assert(cmd_handler_flag == 0);

  // 2.2 Attach the first group
  cli_cmd_group_t cli_cmd_group[ARRAY_SIZE(TEST_cli_cmd_group)] = {0};
  cmd_list->groups = cli_cmd_group;
  cmd_list->length = 1;

  // name and description can not be null
  cli_cmd_group->name = TEST_cli_cmd_group->name;
  cli_cmd_group->desc = TEST_cli_cmd_group->desc;

  // 2.1.1 Test null cmds;
  cli_puts(&cli, "mcu reset\r\n");
  cli_mainloop(&cli);
  assert(cmd_handler_flag == 0);

  // 2.1.1 Attach the first cmd;
  cli_cmd_t cli_cmd_mcu_list[ARRAY_SIZE(TEST_cli_cmd_mcu_list)] = {0};
  cli_cmd_group->cmds = cli_cmd_mcu_list;
  cli_cmd_group->length = 1;

  // name and description can not be null
  cli_cmd_mcu_list->name = TEST_cli_cmd_mcu_list->name;
  cli_cmd_mcu_list->desc = TEST_cli_cmd_mcu_list->desc;

  // 2.1.1 Test default handler
  cli_cmd_mcu_list->handler = NULL;

  cli_puts(&cli, "mcu reset\r\n");
  cli_mainloop(&cli);
  assert(cmd_handler_flag == 0);

  // 2.1.1 Test handler
  cli_cmd_mcu_list->handler = TEST_cli_cmd_mcu_list->handler;
  cli_puts(&cli, "mcu reset\r\n");
  cli_mainloop(&cli);
  assert(cmd_handler_flag == 1);

  return 0;
}
