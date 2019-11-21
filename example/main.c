#include "cli.h"
#include "uart.h"

#include <string.h>

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h> // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

static cli_t cli;

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

static const cli_cmd_group_t cli_cmd_group[] = {
    {.name = "mcu",
     .desc = "MCU group",
     .cmds = cli_cmd_mcu_list,
     .length = ARRAY_SIZE(cli_cmd_mcu_list)},
    {.name = "gpio",
     .desc = "Gpio group",
     .cmds = cli_cmd_gpio_list,
     .length = ARRAY_SIZE(cli_cmd_gpio_list)},
    {.name = "adc",
     .desc = "ADC group",
     .cmds = cli_cmd_adc_list,
     .length = ARRAY_SIZE(cli_cmd_adc_list)},
};

static const cli_cmd_list_t cli_list = {
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
void sleep_ms(int milliseconds) // cross-platform sleep function
{
#ifdef WIN32
  Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
#else
  usleep(milliseconds * 1000);
#endif
}

void uart_rx_callback(char ch) {
  if (cli_putchar(&cli, ch) != ch) {
    ; // could not write. buffer full
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  uart_init();

  uart_register_rx_callback(uart_rx_callback);

  cli_init(&cli, &cli_list);

  while (1) {

    cli_mainloop(&cli);

    sleep_ms(10);
  }
  return 0;
}