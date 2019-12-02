/**
 * @file TEST_cmd_list.h
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