#include "uart.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios termios_orig;
static void (*uart_rx_callback)(char ch);
static pthread_spinlock_t spinlock;

void termios_enable_raw_mode(void) {
  tcgetattr(STDIN_FILENO, &termios_orig);
  struct termios raw = termios_orig;
  cfmakeraw(&raw);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void termios_disable_raw_mode(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &termios_orig);
}

void uart_default_rx_callback(char ch) { (void)ch; }

static void *uart_rx_thread(void *arg) {

  (void)arg;

  int ch;

  while (ch = uart_getchar(), ch != EOF) {
    pthread_spin_lock(&spinlock);
    void (*cb)(char) = uart_rx_callback;
    pthread_spin_unlock(&spinlock);
    cb(ch);
  }

  pthread_exit(&ch);

  return NULL;
}

int uart_putchar(int ch) {
  size_t n = fwrite(&ch, 1, 1, stdout);
  if (n != 1) {
    return EOF;
  }
  return ch;
}

int uart_getchar(void) {
  char ch;
  size_t n = fread(&ch, 1, 1, stdin);
  if (n != 1) {
    return EOF;
  }
  return ch;
}

int uart_flush(void) { return fflush(stdout); }

void uart_register_rx_callback(void (*cb)(char)) {

  pthread_spin_lock(&spinlock);
  uart_rx_callback = cb ? cb : uart_default_rx_callback;
  pthread_spin_unlock(&spinlock);
}

void uart_init(void) {

  termios_enable_raw_mode();
  atexit(termios_disable_raw_mode);

  pthread_spin_init(&spinlock, 0);

  uart_rx_callback = uart_default_rx_callback;

  pthread_t thread_id;
  pthread_create(&thread_id, NULL, uart_rx_thread, NULL);
}