#ifndef UART_H
#define UART_H


int uart_putchar(int);

int uart_getchar(void);

int uart_flush(void);

void uart_register_rx_callback(void (*)(char));

void uart_init(void);

#endif /*UART_H*/
