#ifndef _TEST_CMD_LIST_H
#define _TEST_CMD_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cli.h"

extern volatile int TEST_cmd_handler_flag;

extern const cli_cmd_t TEST_cli_cmd_mcu_list[2];
extern const cli_cmd_t TEST_cli_cmd_gpio_list[3];
extern const cli_cmd_t TEST_cli_cmd_adc_list[2];
extern const cli_cmd_group_t TEST_cli_cmd_group[3];

extern cli_cmd_list_t TEST_cli_cmd_list;

#ifdef __cplusplus
}
#endif

#endif /* _TEST_CMD_LIST_H */