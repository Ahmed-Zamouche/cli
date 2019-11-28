#include "TEST_cmd_list.h"

#include <string.h>

volatile int TEST_cmd_handler_flag = 0;

static int TEST_cli_cmd_handler(cli_t *cli, int argc, char **argv);

const cli_cmd_t TEST_cli_cmd_mcu_list[] = {

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

const cli_cmd_t TEST_cli_cmd_gpio_list[] = {
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

const cli_cmd_t TEST_cli_cmd_adc_list[] = {
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

const cli_cmd_group_t TEST_cli_cmd_group[] = {
    {.name = "mcu", .desc = "MCU group", .cmds = NULL, .length = 0},
    {.name = "gpio", .desc = "Gpio group", .cmds = NULL, .length = 0},
    {.name = "adc", .desc = "ADC group", .cmds = NULL, .length = 0},
};

cli_cmd_list_t TEST_cli_cmd_list = {
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

  TEST_cmd_handler_flag = 1;

  return 0;
}