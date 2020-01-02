/**
 * @file cmd_list.c
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
#include "cmd_list.h"

#include <string.h>

static int cli_cmd_handler(cli_t *cli, int argc, char **argv);

static const cli_cmd_t cli_cmd_mcu_list[] = {

    {
        .name = "reset",
        .desc = "[NUM]. Reset the mcu after NUM seconds",
        .handler = cli_cmd_handler,
    },
    {
        .name = "sleep",
        .desc = "[NUM]. Put mcu in sleep mode for NUM seconds",
        .handler = cli_cmd_handler,
    },

};

static const cli_cmd_group_t cli_cmd_mcu_group = {
    .name = "mcu",
    .desc = "MCU group",
    .cmds = cli_cmd_mcu_list,
    .length = ARRAY_SIZE(cli_cmd_mcu_list)};

static const cli_cmd_t cli_cmd_gpio_list[] = {
    {
        .name = "input-get",
        .desc = "NAME. Get gpio input NAME value",
        .handler = cli_cmd_handler,
    },
    {
        .name = "output-get",
        .desc = "NAME. Get gpio output NAME value",
        .handler = cli_cmd_handler,
    },
    {
        .name = "output-set",
        .desc = "NAME (0|1). Set gpio output NAME",
        .handler = cli_cmd_handler,
    },
};

static const cli_cmd_group_t cli_cmd_gpio_group = {
    .name = "gpio",
    .desc = "Gpio group",
    .cmds = cli_cmd_gpio_list,
    .length = ARRAY_SIZE(cli_cmd_gpio_list)};

static const cli_cmd_t cli_cmd_adc_list[] = {
    {
        .name = "get",
        .desc = "NAME. Get adc NAME value",
        .handler = cli_cmd_handler,
    },
    {
        .name = "start-conv",
        .desc = "NAME. Start acd NAME conversion",
        .handler = cli_cmd_handler,
    },
};

static const cli_cmd_group_t cli_cmd_adc_group = {
    .name = "adc",
    .desc = "ADC group",
    .cmds = cli_cmd_adc_list,
    .length = ARRAY_SIZE(cli_cmd_adc_list)};

static const cli_cmd_group_t *cli_cmd_group[] = {
    &cli_cmd_mcu_group,
    &cli_cmd_gpio_group,
    &cli_cmd_adc_group,
};

const cli_cmd_list_t cli_cmd_list = {
    .groups = cli_cmd_group,
    .length = ARRAY_SIZE(cli_cmd_group),
};

static int cli_cmd_handler(cli_t *cli, int argc, char **argv) {
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

  return 0;
}