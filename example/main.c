#include "cli.h"
#include "cmd_list.h"

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

  cli_init(&cli, &cli_cmd_list);

  while (1) {

    cli_mainloop(&cli);

    sleep_ms(10);
  }
  return 0;
}